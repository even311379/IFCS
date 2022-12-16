#include "Annotation.h"

#include <fstream>

#include "CategoryManagement.h"
#include "DataBrowser.h"
#include "Setting.h"
#include "Style.h"
#include "UUID.h"
#include "Imspinner/imspinner.h"
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include "yaml-cpp/yaml.h"

namespace IFCS
{
    static bool NeedSave = false;
    static bool NeedUpdateCategoryStatics = false;
    static int Tick = 0;
    static int StartFrame;
    static int EndFrame;
    static int BlockSize = 500;
    static ImVec2 WorkStartPos;
    static bool IsPlaying;
    static int FrameJumpSize = 1;

    void Annotation::DisplayFrame(int NewFrameNum)
    {
        CurrentFrame = NewFrameNum;
        if (!DataBrowser::Get().VideoFrames.count(CurrentFrame))
        {
            cv::VideoCapture cap(DataBrowser::Get().SelectedClipInfo.ClipPath);
            cv::Mat Frame;
            cap.set(cv::CAP_PROP_POS_FRAMES, CurrentFrame);
            cap >> Frame;
            cv::resize(Frame, Frame, cv::Size(1280, 720)); // 16 : 9
            cv::cvtColor(Frame, Frame, cv::COLOR_BGR2RGB);
            cap.release();
            DataBrowser::Get().MatToGL(Frame);
        }
        else
        {
            DataBrowser::Get().LoadVideoFrame(NewFrameNum);
        }
    }

    void Annotation::DisplayImage()
    {
        IsImage = true;
        // imread and update info
        cv::Mat Img = cv::imread(DataBrowser::Get().SelectedImageInfo.ImagePath);
        DataBrowser::Get().SelectedImageInfo.Update(Img.cols, Img.rows);
        cv::cvtColor(Img, Img, cv::COLOR_BGR2RGB);
        cv::resize(Img, Img, cv::Size((int)WorkArea.x, (int)WorkArea.y));
        DataBrowser::Get().MatToGL(Img);
        // SaveData();
        LoadData();
    }


    void Annotation::PrepareVideo()
    {
        IsImage = false;
        // show frame 1 and update info
        CurrentFrame = 0;
        StartFrame = 0;
        // SaveData();
        LoadData();
        cv::Mat Frame;
        cv::VideoCapture cap(DataBrowser::Get().SelectedClipInfo.ClipPath);
        int Width = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
        int Height = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
        float FPS = (float)cap.get(cv::CAP_PROP_FPS);
        int FrameCount = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
        EndFrame = FrameCount - 1;
        DataBrowser::Get().SelectedClipInfo.Update(FrameCount, FPS, Width, Height);
        cap >> Frame;
        cv::resize(Frame, Frame, cv::Size(1280, 720)); // 16 : 9
        cv::cvtColor(Frame, Frame, cv::COLOR_BGR2RGB);
        cap.release();
        DataBrowser::Get().MatToGL(Frame);

        if (IsLoadingVideo) return;
        DataBrowser::Get().VideoFrames.clear();
        IsLoadingVideo = true;
        // async load frame with 4 core?
        // TODO: sometimes it will still hand there... but it's pretty robust now?
        auto LoadVideo = [this](int Ith)
        {
            cv::VideoCapture Cap;
            Cap.open(DataBrowser::Get().SelectedClipInfo.ClipPath);
            const int FramesToLoad = std::min(DataBrowser::Get().SelectedClipInfo.FrameCount, BlockSize * 3);
            int i = int(float(Ith) * float(FramesToLoad) / 4.f);
            int End = int(float(Ith + 1) / 4.f * float(FramesToLoad));
            Cap.set(cv::CAP_PROP_POS_FRAMES, i);
            while (1)
            {
                if (i > End)
                {
                    break;
                }
                cv::Mat Frame;
                Cap >> Frame;
                if (Frame.empty())
                {
                    break;
                }
                cv::resize(Frame, Frame, cv::Size((int)WorkArea.x, (int)WorkArea.y));
                cv::cvtColor(Frame, Frame, cv::COLOR_BGR2RGB);
                DataBrowser::Get().VideoFrames[i] = Frame;
                i++;
            }
            Cap.release();
        };
        for (int T = 0; T < 4; T++)
        {
            Tasks[T] = std::async(std::launch::async, LoadVideo, T);
        }
    }

    void Annotation::LoadingVideoBlock()
    {
        // check which block to load and which to erase
        if (IsLoadingVideo) return;
        if (CurrentFrame + BlockSize * 2 > DataBrowser::Get().SelectedClipInfo.FrameCount) return;
        if (DataBrowser::Get().VideoFrames.count(CurrentFrame + BlockSize * 2)) return;
        IsLoadingVideo = true;
        const int OldBlockStart = (CurrentFrame / BlockSize - 1) * BlockSize;
        for (int i = OldBlockStart; i < OldBlockStart + BlockSize; i++)
        {
            DataBrowser::Get().VideoFrames.erase(i);
        }
        int NewBlockStart = ((CurrentFrame / BlockSize) + 2) * BlockSize;
        // async load frame with multi core?
        auto LoadVideo = [=](int Ith)
        {
            cv::VideoCapture Cap;
            Cap.open(DataBrowser::Get().SelectedClipInfo.ClipPath);
            int i = NewBlockStart + int(float(Ith) * float(BlockSize) / 4.f);
            int End = NewBlockStart + int(float(Ith + 1) / 4.f * float(BlockSize));
            Cap.set(cv::CAP_PROP_POS_FRAMES, i);
            while (1)
            {
                if (i > End)
                {
                    break;
                }
                cv::Mat Frame;
                Cap.read(Frame);
                if (Frame.empty())
                {
                    break;
                }
                cv::resize(Frame, Frame, cv::Size((int)WorkArea.x, (int)WorkArea.y));
                cv::cvtColor(Frame, Frame, cv::COLOR_BGR2RGB);
                DataBrowser::Get().VideoFrames[i] = Frame;
                i++;
            }
            Cap.release();
        };
        for (int T = 0; T < 4; T++)
        {
            Tasks[T] = std::async(std::launch::async, LoadVideo, T);
        }
    }


    std::map<int, FAnnotationToDisplay> Annotation::GetAnnotationToDisplay()
    {
        std::map<int, FAnnotationToDisplay> Out;
        for (const auto& [F, M] : Data)
        {
            Out[F] = FAnnotationToDisplay((int)M.size(), CheckData[F].IsReady);
        }
        return Out;
    }


    void Annotation::RenderContent()
    {
        RenderSelectCombo();

        RenderAnnotationWork();

        ImGui::Dummy({0, ImGui::GetTextLineHeightWithSpacing()});
        if (!DataBrowser::Get().SelectedClipInfo.ClipPath.empty())
        {
            RenderVideoControls();
            ImGui::Dummy({0, ImGui::GetTextLineHeightWithSpacing() * 0.5f});
        }
        RenderAnnotationToolbar();

        if (IsLoadingVideo)
        {
            ImSpinner::SpinnerBarsScaleMiddle("Spinner_LoadVideo_1", 6,
                                              ImColor(Style::BLUE(400, Setting::Get().Theme)));
            ImGui::Text("Video loading...");
            int NumCoreFinished = 0;
            for (size_t T = 0; T < 4; T++)
            {
                if (Tasks[T].wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
                {
                    NumCoreFinished++;
                }
            }
            if (NumCoreFinished == 4) IsLoadingVideo = false;
        }

        if (Tick > 60)
        {
            Tick = 0;
            if (NeedSave) // check need save every 300 frame?
            {
                SaveData();
                NeedSave = false;
                if (NeedUpdateCategoryStatics)
                {
                    CategoryManagement::Get().NeedUpdate = true;
                    NeedUpdateCategoryStatics = false;
                }
            }
        }
        Tick++;
    }


    void Annotation::RenderSelectCombo()
    {
        ImGui::SetNextItemWidth(480);
        if (IsImage)
        {
            const size_t RelOffset = Setting::Get().ProjectPath.size() + 8;
            if (ImGui::BeginCombo("##ImageSelect", DataBrowser::Get().SelectedImageInfo.GetRelativePath().c_str()))
            {
                for (const std::string& Img : DataBrowser::Get().GetAllImages())
                {
                    if (ImGui::Selectable(Img.substr(RelOffset).c_str()))
                    {
                        DataBrowser::Get().SelectedClipInfo.ClipPath = ""; // Deselect Clip
                        DataBrowser::Get().SelectedImageInfo.ImagePath = Img;
                        DisplayImage();
                    }
                }
                ImGui::EndCombo();
            }
        }
        else
        {
            const size_t RelOffset = Setting::Get().ProjectPath.size() + 7;
            if (ImGui::BeginCombo("##ClipSelect", DataBrowser::Get().SelectedClipInfo.GetRelativePath().c_str()))
            {
                for (const std::string& C : DataBrowser::Get().GetAllClips())
                {
                    if (ImGui::Selectable(C.substr(RelOffset).c_str()))
                    {
                        DataBrowser::Get().SelectedImageInfo.ImagePath = ""; // Deselect image...
                        DataBrowser::Get().SelectedClipInfo.ClipPath = C;
                        PrepareVideo();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            if (ImGui::BeginCombo("##ClipFrame", std::to_string(CurrentFrame).c_str()))
            {
                for (const auto& [F, Map] : Data)
                {
                    if (ImGui::Selectable(std::to_string(F).c_str(), F == CurrentFrame))
                    {
                        MoveFrame(F);
                    }
                }
                ImGui::EndCombo();
            }
        }
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
        if (IsImage)
        {
            ImGui::Text(ICON_FA_IMAGE);
        }
        else
        {
            ImGui::Text(ICON_FA_VIDEO);
        }
    }


    void Annotation::RenderAnnotationWork()
    {
        ImU32 BgColor = ImGui::ColorConvertFloat4ToU32(Style::GRAY(600, Setting::Get().Theme));
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - WorkArea.x) * 0.5f);
        ImGui::BeginChild("AnnotaitonWork", WorkArea, false);
        ImGui::GetWindowDrawList()->AddRectFilled(WorkStartPos, WorkStartPos + WorkArea, BgColor); // draw bg
        ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)DataBrowser::Get().LoadedFramePtr,
                                             WorkStartPos + PanAmount,
                                             WorkStartPos + PanAmount + GetZoom() * WorkArea); // image
        static UUID ToDeleteAnnID = 0;
        std::unordered_map<UUID, FAnnotation>* DataToDisplay = nullptr;
        if (IsImage)
        {
            DataToDisplay = &Data_Img;
        }
        else
        {
            if (Data.count(CurrentFrame))
                DataToDisplay = &Data[CurrentFrame];
        }
        // render things from data
        if (DataToDisplay)
        {
            for (auto& [ID, A] : *DataToDisplay)
            {
                FCategory Category = CategoryManagement::Get().Data[A.CategoryID];
                if (!Category.Visibility) continue;
                std::array<float, 4> TransformedXYWH = TransformXYWH(A.XYWH);
                float X = TransformedXYWH[0];
                float Y = TransformedXYWH[1];
                float W = TransformedXYWH[2];
                float H = TransformedXYWH[3];
                ImVec2 TL, BL, BR, TR, C; // top-left, bottom-left, bottom-right, top-right, center
                TL = {X - 0.5f * W, Y - 0.5f * H};
                BL = {X - 0.5f * W, Y + 0.5f * H};
                BR = {X + 0.5f * W, Y + 0.5f * H};
                TR = {X + 0.5f * W, Y - 0.5f * H};
                C = {X, Y};

                ImU32 Color = ImGui::ColorConvertFloat4ToU32(Category.Color);
                ImGui::GetWindowDrawList()->AddRect(TL, BR, Color, 0, 0, 2);
                // annotation box
                ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 24.f, TL - ImVec2(0, 30.f), Color,
                                                    Category.DisplayName.c_str()); // display name
                static const float PanControlRadius = 12.f;
                ImGui::PushID(int(ID));
                const bool IsMouseDragging = ImGui::IsMouseDragging(0);
                if (EditMode == EAnnotationEditMode::Edit)
                {
                    // pan controller Center
                    ImGui::GetWindowDrawList()->AddCircleFilled(C, PanControlRadius, Color);
                    ImGui::SetCursorScreenPos(C - ImVec2(PanControlRadius, PanControlRadius));
                    ImGui::InvisibleButton("Pan", ImVec2(PanControlRadius * 2, PanControlRadius * 2));
                    bool IsPanActive = ImGui::IsItemActive();
                    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                    if (IsPanActive && IsMouseDragging)
                    {
                        ImVec2 Delta = ImGui::GetIO().MouseDelta / GetZoom();
                        A.Pan({Delta.x, Delta.y});
                        if (IsImage)
                            UpdateTime_Img = Utils::GetCurrentTimeString();
                        else
                            CheckData[CurrentFrame].UpdateTime = Utils::GetCurrentTimeString();
                        NeedSave = true;
                    }
                    // resize top left
                    ResizeImpl(EBoxCorner::TopLeft, TL, Color, IsMouseDragging, A);
                    // resize bottom left
                    ResizeImpl(EBoxCorner::BottomLeft, BL, Color, IsMouseDragging, A);
                    // resize bottom right
                    ResizeImpl(EBoxCorner::BottomRight, BR, Color, IsMouseDragging, A);
                    // resize top right
                    ResizeImpl(EBoxCorner::TopRight, TR, Color, IsMouseDragging, A);

                    ImVec2 ComboPos = TL - ImVec2(36, 0);
                    ImGui::SetCursorScreenPos(ComboPos);
                    ImGui::PushStyleColor(ImGuiCol_Button, Category.Color);
                    if (ImGui::BeginCombo("##EditCombo", "", ImGuiComboFlags_NoPreview))
                    {
                        for (const auto& [UID, Cat] : CategoryManagement::Get().Data)
                        {
                            if (Cat.DisplayName == Category.DisplayName) continue;
                            if (ImGui::Selectable((std::string("Change to ") + Cat.DisplayName).c_str()))
                            {
                                A.CategoryID = UID;
                                if (IsImage)
                                    UpdateTime_Img = Utils::GetCurrentTimeString();
                                else
                                    CheckData[CurrentFrame].UpdateTime = Utils::GetCurrentTimeString();
                                NeedUpdateCategoryStatics = true;
                                NeedSave = true;
                            }
                        }
                        if (ImGui::Selectable("Delete"))
                        {
                            ToDeleteAnnID = ID;
                            if (IsImage)
                                UpdateTime_Img = Utils::GetCurrentTimeString();
                            else
                                CheckData[CurrentFrame].UpdateTime = Utils::GetCurrentTimeString();
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::PopStyleColor();
                }
                ImGui::PopID();
            }
        }
        if (ToDeleteAnnID != 0)
        {
            if (IsImage)
                Data_Img.erase(ToDeleteAnnID);
            else
                Data[CurrentFrame].erase(ToDeleteAnnID);

            ToDeleteAnnID = 0;
            NeedSave = true;
        }
        static ImVec2 AddPointStart;
        static bool IsAdding;
        // render working stuff
        if (IsAdding)
        {
            ImU32 NewColor = ImGui::ColorConvertFloat4ToU32(CategoryManagement::Get().GetSelectedCategory()->Color);
            ImGui::GetWindowDrawList()->AddRect(AddPointStart, ImGui::GetMousePos(), NewColor, 0, 0, 3);
        }
        ImGui::EndChild();
        WorkStartPos = ImGui::GetItemRectMin();
        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
            {
                PanAmount.x = PanAmount.x + ImGui::GetIO().MouseDelta.x;
                PanAmount.y = PanAmount.y + ImGui::GetIO().MouseDelta.y;
            }
            float Wheel = ImGui::GetIO().MouseWheel;
            if (Wheel != 0.f)
            {
                int OldZoomLevel = ZoomLevel;
                ImVec2 ZoomPos = ImGui::GetMousePos() - ImGui::GetItemRectMin();
                ZoomLevel += int(Wheel);
                if (ZoomLevel < -3) ZoomLevel = -3;
                if (ZoomLevel > 3) ZoomLevel = 3;
                // make same position after zoom
                ImVec2 ImgPos = (ZoomPos - PanAmount) / GetZoom(OldZoomLevel);
                PanAmount = ZoomPos - ImgPos * GetZoom();
            }
            if (EditMode == EAnnotationEditMode::Add && ImGui::IsMouseClicked(0) && DataBrowser::Get().
                IsSelectAnyClipOrImg())
            {
                if (CategoryManagement::Get().GetSelectedCategory())
                {
                    IsAdding = true;
                    AddPointStart = ImGui::GetMousePos();
                }
            }

            /////////////////
            // HOTKEYS
            ////////////////

            if (ImGui::IsKeyPressed(ImGuiKey_W))
            {
                EditMode = EAnnotationEditMode::Add;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_E))
            {
                EditMode = EAnnotationEditMode::Edit;
            }
            // TODO: it's better off not add these hotkeys... it will conflict with ImGui default hotkeys...
            if (!IsImage)
            {
                if (ImGui::IsKeyPressed(ImGuiKey_A))
                {
                    if (CurrentFrame == EndFrame)
                        CurrentFrame = StartFrame;
                    IsPlaying = !IsPlaying;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_S))
                {
                    if (!IsPlaying)
                        MoveFrame(CurrentFrame - FrameJumpSize);
                }
                if (ImGui::IsKeyPressed(ImGuiKey_D))
                {
                    if (!IsPlaying)
                        MoveFrame(CurrentFrame + FrameJumpSize);
                }
            }
        }
        if (ImGui::IsMouseReleased(0) && IsAdding)
        {
            IsAdding = false;
            ImVec2 AbsMin, AbsMax;
            Utils::GetAbsRectMinMax(AddPointStart, ImGui::GetMousePos(), AbsMin, AbsMax);
            std::array<float, 4> NewXYWH = MouseRectToXYWH(AbsMin, AbsMax);
            if (IsImage)
            {
                Data_Img[UUID()] = FAnnotation(CategoryManagement::Get().SelectedCatID, NewXYWH);
                UpdateTime_Img = Utils::GetCurrentTimeString();
            }
            else
            {
                Data[CurrentFrame][UUID()] = FAnnotation(CategoryManagement::Get().SelectedCatID, NewXYWH);
                CheckData[CurrentFrame].UpdateTime = Utils::GetCurrentTimeString();
            }
            CategoryManagement::Get().AddCount();
            NeedSave = true;
        }
    }

    void Annotation::RenderVideoControls()
    {
        static char* PlayIcon;
        if (CurrentFrame == EndFrame)
            PlayIcon = ICON_FA_SYNC;
        else if (IsPlaying)
            PlayIcon = ICON_FA_PAUSE;
        else
            PlayIcon = ICON_FA_PLAY;
        if (ImGui::Button(PlayIcon, {120, 32}))
        {
            if (CurrentFrame == EndFrame)
                CurrentFrame = StartFrame;
            IsPlaying = !IsPlaying;
        }
        Utils::AddSimpleTooltip("Play (A)");
        ImGui::SameLine();
        static ImVec2 NewFramePad(4, (32 - ImGui::GetFontSize()) / 2);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, NewFramePad);
        ImGui::SetNextItemWidth(120.f);
        if (ImGui::DragInt("##PlayStart", &StartFrame, 1, 0, EndFrame - 1))
        {
            if (CurrentFrame < StartFrame) CurrentFrame = StartFrame;
            if (StartFrame > EndFrame)
            {
                StartFrame = EndFrame - 1;
                CurrentFrame = StartFrame;
            }
        }
        ImGui::PopStyleVar();
        ImGui::SameLine();
        ImVec2 CurrentPos = ImGui::GetCursorScreenPos();
        ImU32 Color = ImGui::ColorConvertFloat4ToU32(Style::BLUE(600, Setting::Get().Theme));
        ImU32 PlayColor = ImGui::ColorConvertFloat4ToU32(Style::YELLOW(400, Setting::Get().Theme));
        const float Width = ImGui::GetContentRegionAvail().x - 130;
        ImVec2 RectStart = CurrentPos + ImVec2(5, 22);
        ImVec2 RectEnd = RectStart + ImVec2(Width, 5);
        ImGui::GetWindowDrawList()->AddRectFilled(RectStart, RectEnd, Color, 20.f);
        ImVec2 RectPlayStart = RectStart + ImVec2(0, -10);
        ImVec2 RectPlayEnd = RectStart + ImVec2(float(CurrentFrame - StartFrame) / float(EndFrame - StartFrame) * Width,
                                                10);
        ImGui::GetWindowDrawList()->AddRectFilled(RectPlayStart, RectPlayEnd, PlayColor);
        // draw current frame text?
        if (RectPlayEnd.x > RectPlayStart.x + 30)
        {
            char FrameTxt[8];
            sprintf(FrameTxt, "%d", CurrentFrame);
            ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 16,
                                                RectPlayEnd + ImVec2(strlen(FrameTxt) * -16.f, -18.f), Color, FrameTxt);
        }
        ImGui::SetCursorScreenPos(RectPlayStart);
        ImGui::InvisibleButton("PlayControl", ImVec2(Width, 15));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            const int FrameAtPosition = int(
                (ImGui::GetMousePos().x - RectStart.x) / Width * (float)(EndFrame - StartFrame)) + StartFrame;
            ImGui::SetTooltip("%d", FrameAtPosition);
        }
        if (ImGui::IsItemClicked(0))
        {
            int NewFramePos = int((ImGui::GetMousePos().x - RectStart.x) / Width * (float)(EndFrame - StartFrame)) +
                StartFrame;
            MoveFrame(NewFramePos);
            IsPlaying = false;
            DisplayFrame(CurrentFrame);
        }

        ImGui::SameLine();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, NewFramePad);
        ImGui::SetNextItemWidth(120.f);
        int MaxFrame = DataBrowser::Get().SelectedClipInfo.FrameCount - 1;
        if (ImGui::DragInt("##PlayEnd", &EndFrame, 1, StartFrame + 1, MaxFrame))
        {
            if (CurrentFrame > EndFrame) CurrentFrame = EndFrame;
            if (EndFrame < StartFrame)
            {
                EndFrame = StartFrame + 1;
                CurrentFrame = StartFrame;
            }
            if (EndFrame > MaxFrame) EndFrame = MaxFrame;
        }
        ImGui::PopStyleVar();

        // more frame controls
        ImGui::Dummy({0, ImGui::GetTextLineHeightWithSpacing() * 0.5f});
        static float ButtonsOffset;
        static ImVec2 BtnSize(ImGui::GetFont()->FontSize * 3, ImGui::GetFont()->FontSize * 1.5f);
        ImGui::SetCursorPosX(ButtonsOffset);
        ImGui::BeginGroup();
        {
            char buf[32];
            sprintf(buf, "%s %d", ICON_FA_MINUS, FrameJumpSize);
            if (ImGui::Button(buf, BtnSize))
            {
                if (!IsPlaying)
                    MoveFrame(CurrentFrame - FrameJumpSize);
            }
            Utils::AddSimpleTooltip("Jump backward frames (S)");
            ImGui::SameLine();
            char buf1[32];
            sprintf(buf1, "%s %d", ICON_FA_PLUS, FrameJumpSize);
            if (ImGui::Button(buf1, BtnSize))
            {
                if (!IsPlaying)
                    MoveFrame(CurrentFrame + FrameJumpSize);
            }
            Utils::AddSimpleTooltip("Jump forward frames (D)");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(BtnSize.x);
            static ImVec2 BtnFramePad(4, (BtnSize.y - ImGui::GetFontSize()) / 2);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, BtnFramePad);
            ImGui::DragInt("##JustSize", &FrameJumpSize, 1, 1, 60);
            ImGui::PopStyleVar();
            Utils::AddSimpleTooltip("Set jump size");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_EXPAND, BtnSize))
            {
                StartFrame = 0;
                EndFrame = MaxFrame;
            }
            Utils::AddSimpleTooltip("Reset play start and end");
        }
        ImGui::EndGroup();
        ButtonsOffset = (ImGui::GetContentRegionAvail().x - ImGui::GetItemRectSize().x) * 0.5f;

        // processing play...
        if (!IsPlaying) return;
        static float TimePassed = 0.f;
        TimePassed += (1.0f / ImGui::GetIO().Framerate);
        if (TimePassed < (1 / DataBrowser::Get().SelectedClipInfo.FPS)) return;
        TimePassed = 0.f;
        DisplayFrame(CurrentFrame);
        CurrentFrame += 1;
        LoadingVideoBlock();
        //check should release cap and stop play
        if (CurrentFrame == EndFrame)
        {
            IsPlaying = false;
        }
    }

    void Annotation::RenderAnnotationToolbar()
    {
        static float ButtonsOffset;
        static ImVec2 BtnSize(ImGui::GetFont()->FontSize * 5, ImGui::GetFont()->FontSize * 2.5f);
        static bool ShouldChangeEditMode = false;
        static EAnnotationEditMode EditModeToChange;
        ImGui::SetCursorPosX(ButtonsOffset);
        ImGui::BeginGroup();
        {
            ImGui::PushFont(Setting::Get().TitleFont);
            if (EditMode == EAnnotationEditMode::Add)
                ImGui::PushStyleColor(ImGuiCol_Button, GImGui->Style.Colors[ImGuiCol_CheckMark]);
            if (ImGui::Button(ICON_FA_EDIT, BtnSize)) // add
            {
                ShouldChangeEditMode = true;
                EditModeToChange = EAnnotationEditMode::Add;
            }
            Utils::AddSimpleTooltip("Add annotation (W)");
            if (EditMode == EAnnotationEditMode::Add)
                ImGui::PopStyleColor();
            ImGui::SameLine();
            if (EditMode == EAnnotationEditMode::Edit)
                ImGui::PushStyleColor(ImGuiCol_Button, GImGui->Style.Colors[ImGuiCol_CheckMark]);
            if (ImGui::Button(ICON_FA_VECTOR_SQUARE, BtnSize)) // edit
            {
                ShouldChangeEditMode = true;
                EditModeToChange = EAnnotationEditMode::Edit;
            }
            Utils::AddSimpleTooltip("Edit annotation (E)");
            if (EditMode == EAnnotationEditMode::Edit)
                ImGui::PopStyleColor();
            ImGui::SameLine();

            if (ImGui::Button(ICON_FA_SYNC_ALT, BtnSize)) // reset pan zoom
            {
                PanAmount = ImVec2(0, 0);
                ZoomLevel = 0;
            }
            Utils::AddSimpleTooltip("Reset zoon and pan");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_CHEVRON_LEFT, BtnSize))
            {
                if (IsImage)
                {
                    std::vector<std::string> AllImgs = DataBrowser::Get().GetAllImages();
                    YAML::Node AnnData = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/Annotations.yaml");
                    std::string LastFoundImg;
                    for (const auto& ImgPath : AllImgs)
                    {
                        if (DataBrowser::Get().SelectedImageInfo.ImagePath == ImgPath)
                        {
                            break;
                        }
                        if (AnnData[ImgPath]) LastFoundImg = ImgPath;
                    }
                    if (!LastFoundImg.empty())
                    {
                        DataBrowser::Get().SelectedImageInfo.ImagePath = LastFoundImg;
                        DisplayImage();
                    }
                }
                else
                {
                    for (int i = CurrentFrame - 1; i > -1; i--)
                    {
                        if (Data.count(i))
                        {
                            MoveFrame(i);
                            break;
                        }
                    }
                }
            }
            Utils::AddSimpleTooltip("Previous annotated frame/image");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_CHEVRON_RIGHT, BtnSize))
            {
                if (IsImage)
                {
                    std::vector<std::string> AllImgs = DataBrowser::Get().GetAllImages();
                    YAML::Node AnnData = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/Annotations.yaml");
                    std::string NextFoundImg;
                    bool ReachedCurrent = false;
                    for (const auto& ImgPath : AllImgs)
                    {
                        if (AnnData[ImgPath] && ReachedCurrent)
                        {
                            NextFoundImg = ImgPath;
                            break;
                        }
                        if (!ReachedCurrent)
                            if (DataBrowser::Get().SelectedImageInfo.ImagePath == ImgPath)
                                ReachedCurrent = true;
                    }
                    if (!NextFoundImg.empty())
                    {
                        DataBrowser::Get().SelectedImageInfo.ImagePath = NextFoundImg;
                        DisplayImage();
                    }
                }
                else
                {
                    for (int i = CurrentFrame + 1; i < DataBrowser::Get().SelectedClipInfo.FrameCount - 1; i++)
                    {
                        if (Data.count(i))
                        {
                            MoveFrame(i);
                            break;
                        }
                    }
                }
            }
            Utils::AddSimpleTooltip("Next annotated frame/image");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_TIMES, BtnSize))
            {
                if (IsImage)
                    Data_Img.clear();
                else
                    Data.erase(CurrentFrame);
                NeedSave = true;
            }
            Utils::AddSimpleTooltip("Delete all annotations in this frame/image");
            ImGui::PopFont();
        }
        ImGui::EndGroup();
        ButtonsOffset = (ImGui::GetContentRegionAvail().x - ImGui::GetItemRectSize().x) * 0.5f;

        if (ShouldChangeEditMode)
        {
            EditMode = EditModeToChange;
            ShouldChangeEditMode = false;
        }

        if (IsImage)
        {
            if (!Data_Img.empty())
            {
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 100);
                if (ImGui::Checkbox("Is ready?", &IsReady_Img))
                {
                    UpdateTime_Img = Utils::GetCurrentTimeString();
                    NeedSave = true;
                }
            }
        }
        else
        {
            if (Data.count(CurrentFrame))
            {
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 100);
                if (ImGui::Checkbox("Is ready?", &CheckData[CurrentFrame].IsReady))
                {
                    CheckData[CurrentFrame].UpdateTime = Utils::GetCurrentTimeString();
                    NeedSave = true;
                }
            }
        }
    }

    void Annotation::SaveData()
    {
        YAML::Node Origin = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Annotations.yaml"));
        if (IsImage)
        {
            if (Data_Img.empty())
            {
                Origin.remove(DataBrowser::Get().SelectedImageInfo.ImagePath);
            }
            else
            {
                YAML::Node ImgNode;
                ImgNode["IsReady"] = IsReady_Img;
                ImgNode["UpdateTime"] = UpdateTime_Img;
                for (const auto& [ID, Ann] : Data_Img)
                {
                    ImgNode[std::to_string(ID)] = Ann.Serialize();
                }
                Origin[DataBrowser::Get().SelectedImageInfo.ImagePath]["0"] = ImgNode;
            }
        }
        else
        {
            YAML::Node ClipNode;
            for (const auto& [F, Map] : Data)
            {
                for (const auto& [ID, Ann] : Map)
                {
                    ClipNode[F][std::to_string(ID)] = Ann.Serialize();
                }
            }
            for (const auto& [F, C] : CheckData)
            {
                ClipNode[F]["IsReady"] = C.IsReady;
                ClipNode[F]["UpdateTime"] = C.UpdateTime;
            }
            Origin[DataBrowser::Get().SelectedClipInfo.ClipPath] = ClipNode;
        }

        YAML::Emitter Out;
        Out << Origin;
        std::ofstream Fout(Setting::Get().ProjectPath + std::string("/Data/Annotations.yaml"));
        Fout << Out.c_str();
    }

    void Annotation::LoadData()
    {
        YAML::Node AllAnnData = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Annotations.yaml"));
        if (IsImage)
        {
            Data_Img.clear();
            IsReady_Img = false;
            if (!AllAnnData[DataBrowser::Get().SelectedImageInfo.ImagePath]) return;
            YAML::Node ImgNode = AllAnnData[DataBrowser::Get().SelectedImageInfo.ImagePath][0];
            for (YAML::const_iterator it = ImgNode.begin(); it != ImgNode.end(); ++it)
            {
                if (it->first.as<std::string>() == "UpdateTime")
                    UpdateTime_Img = it->second.as<std::string>();
                else if (it->first.as<std::string>() == "IsReady")
                    IsReady_Img = it->second.as<bool>();
                else
                    Data_Img[it->first.as<uint64_t>()] = FAnnotation(it->second);
            }
        }
        else
        {
            Data.clear();
            CheckData.clear();
            YAML::Node ClipData = AllAnnData[DataBrowser::Get().SelectedClipInfo.ClipPath];
            for (YAML::const_iterator it = ClipData.begin(); it != ClipData.end(); ++it)
            {
                CheckData[it->first.as<int>()] = FCheck();
                for (YAML::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
                {
                    if (it2->first.as<std::string>() == "UpdateTime")
                    {
                        CheckData[it->first.as<int>()].UpdateTime = it2->second.as<std::string>();
                    }
                    else if (it2->first.as<std::string>() == "IsReady")
                    {
                        CheckData[it->first.as<int>()].IsReady = it2->second.as<bool>();
                    }
                    else
                    {
                        Data[it->first.as<int>()][it2->first.as<uint64_t>()] = FAnnotation(it2->second);
                    }
                }
            }
        }
    }


    float Annotation::GetZoom(int InZoomLevel)
    {
        switch (InZoomLevel)
        {
        case -3: return 0.125f;
        case -2: return 0.25f;
        case -1: return 0.5;
        case 0: return 1.0f;
        case 1: return 2.0f;
        case 2: return 4.0f;
        case 3: return 8.0f;
        default: return 1.0f;
        }
    }

    float Annotation::GetZoom()
    {
        return GetZoom(ZoomLevel);
    }

    std::array<float, 4> Annotation::MouseRectToXYWH(ImVec2 RectMin, ImVec2 RectMax)
    {
        ImVec2 RawRectDiff = RectMax - RectMin;
        RectMin = (RectMin - WorkStartPos - PanAmount) / GetZoom();
        RectMax = RectMin + RawRectDiff / GetZoom();
        if (RectMin.x <= 0) RectMin.x = 0;
        if (RectMin.y <= 0) RectMin.y = 0;
        if (RectMax.x >= WorkArea.x) RectMax.x = WorkArea.x;
        if (RectMax.y >= WorkArea.y) RectMax.y = WorkArea.y;
        return {
            (RectMax.x + RectMin.x) / 2,
            (RectMax.y + RectMin.y) / 2,
            RectMax.x - RectMin.x,
            RectMax.y - RectMin.y
        };
    }

    std::array<float, 4> Annotation::TransformXYWH(const std::array<float, 4> InXYWH)
    {
        ImVec2 RectMin = {InXYWH[0] - 0.5f * InXYWH[2], InXYWH[1] - 0.5f * InXYWH[3]};
        ImVec2 TMin = RectMin * GetZoom() + WorkStartPos + PanAmount;
        ImVec2 TMax = TMin + ImVec2(InXYWH[2], InXYWH[3]) * GetZoom();
        return {
            (TMax.x + TMin.x) / 2,
            (TMax.y + TMin.y) / 2,
            TMax.x - TMin.x,
            TMax.y - TMin.y
        };
    }

    void Annotation::ResizeImpl(EBoxCorner WhichCorner, const ImVec2& InPos, ImU32 InColor, bool IsDragging,
                                FAnnotation& Ann)
    {
        const float CircleRadius = 9.f;
        ImGui::GetWindowDrawList()->AddCircleFilled(InPos, CircleRadius, InColor);
        ImGui::SetCursorScreenPos(InPos - ImVec2(CircleRadius, CircleRadius));
        std::string ResizeID;
        switch (WhichCorner)
        {
        case EBoxCorner::TopLeft: ResizeID = "TopLeft";
            break;
        case EBoxCorner::BottomLeft: ResizeID = "BottomLeft";
            break;
        case EBoxCorner::BottomRight: ResizeID = "BottomRight";
            break;
        case EBoxCorner::TopRight: ResizeID = "TopRight";
            break;
        }
        ImGui::InvisibleButton(ResizeID.c_str(), ImVec2(CircleRadius * 2, CircleRadius * 2));
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (ImGui::IsItemActive() && IsDragging)
        {
            const ImVec2 Delta = ImGui::GetIO().MouseDelta / GetZoom();
            Ann.Resize(WhichCorner, {Delta.x, Delta.y});
            if (IsImage)
                UpdateTime_Img = Utils::GetCurrentTimeString();
            else
                CheckData[CurrentFrame].UpdateTime = Utils::GetCurrentTimeString();
            NeedSave = true;
        }
    }


    void Annotation::MoveFrame(int NewFrame)
    {
        CurrentFrame = NewFrame;
        if (CurrentFrame < StartFrame) StartFrame = CurrentFrame;
        if (CurrentFrame > EndFrame) EndFrame = CurrentFrame;
        DisplayFrame(CurrentFrame);
    }
}
