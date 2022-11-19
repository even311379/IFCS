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
    void Annotation::DisplayFrame(int NewFrameNum)
    {
        CurrentFrame = NewFrameNum;
        if (IsLoadingVideo)
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
        // imread and update info
        cv::Mat Img = cv::imread(DataBrowser::Get().SelectedImageInfo.ImagePath);
        DataBrowser::Get().SelectedImageInfo.Update(Img.cols, Img.rows);
        cv::cvtColor(Img, Img, cv::COLOR_BGR2RGB);
        cv::resize(Img, Img, cv::Size((int)WorkArea.x, (int)WorkArea.y));
        DataBrowser::Get().MatToGL(Img);
    }

    int StartFrame;
    int EndFrame;

    void Annotation::PrepareVideo()
    {
        // show frame 1 and update info
        CurrentFrame = 0;
        StartFrame = 0;
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
        // TODO: need wisely utilize memery...
        IsLoadingVideo = true;
        // async load frame with multi core?

        // TODO: read long long video?
        auto LoadVideo = [this](int Ith)
        {
            cv::VideoCapture Cap;
            Cap.open(DataBrowser::Get().SelectedClipInfo.ClipPath);
            int FrameCount = DataBrowser::Get().SelectedClipInfo.FrameCount;
            int i = int(float(Ith) * float(FrameCount) / 4.f);
            int End = int(float(Ith + 1) / 4.f * float(FrameCount));
            spdlog::info("{}:{}", i, End);
            cv::Mat Frame;
            Cap.set(cv::CAP_PROP_POS_FRAMES, i);
            while (1)
            {
                if (i > End)
                {
                    spdlog::info("End?");
                    break;
                }
                Cap.read(Frame);
                if (Frame.empty())
                {
                    spdlog::info("no more frame?");
                    break;
                }
                cv::resize(Frame, Frame, cv::Size((int)WorkArea.x, (int)WorkArea.y));
                // 16 : 9 // no need to resize?
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

    bool NeedSave = false;
    bool NeedUpdateCategoryStatics = false;

    int Tick = 0;

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
            ImGui::Dummy({0, ImGui::GetTextLineHeightWithSpacing()});
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

        if (Tick > 300)
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
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 200);
        ImGui::Checkbox("Annotate image?", &IsImage);
    }

    ImVec2 WorkStartPos;
    // TODO: update time and remove unrequired check data!
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
        // render things from data
        if (Data.count(CurrentFrame))
        {
            for (auto& [ID, A] : Data[CurrentFrame])
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
                                CheckData[CurrentFrame].UpdateTime = Utils::GetCurrentTimeString();
                                NeedUpdateCategoryStatics = true;
                                NeedSave = true;
                            }
                        }
                        if (ImGui::Selectable("Delete"))
                        {
                            ToDeleteAnnID = ID;
                            CheckData[CurrentFrame].UpdateTime = Utils::GetCurrentTimeString();
                            NeedSave = true;
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
            Data[CurrentFrame].erase(ToDeleteAnnID);
            ToDeleteAnnID = 0;
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
                // PanAmount += ImGui::GetIO().MouseDelta; // weird... this make it 0...
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
            if (EditMode == EAnnotationEditMode::Add && ImGui::IsMouseClicked(0))
            {
                if (CategoryManagement::Get().GetSelectedCategory())
                {
                    IsAdding = true;
                    AddPointStart = ImGui::GetMousePos();
                }
            }
            if (ImGui::IsMouseReleased(0) && IsAdding)
            {
                IsAdding = false;
                ImVec2 AbsMin, AbsMax;
                Utils::GetAbsRectMinMax(AddPointStart, ImGui::GetMousePos(), AbsMin, AbsMax);
                std::array<float, 4> NewXYWH = MouseRectToXYWH(AbsMin, AbsMax);
                Data[CurrentFrame][UUID()] = FAnnotation(CategoryManagement::Get().SelectedCatID, NewXYWH);
                CategoryManagement::Get().AddCount();
                CheckData[CurrentFrame].UpdateTime = Utils::GetCurrentTimeString();
                NeedSave = true;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_W))
            {
                EditMode = EAnnotationEditMode::Add;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_E))
            {
                EditMode = EAnnotationEditMode::Edit;
            }
        }
    }

    void Annotation::RenderVideoControls()
    {
        static bool IsPlaying;
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
        if (ImGui::DragInt("##PlayEnd", &EndFrame, 1, StartFrame + 1,
                           DataBrowser::Get().SelectedClipInfo.FrameCount - 1))
        {
            if (CurrentFrame > EndFrame) CurrentFrame = EndFrame;
            if (EndFrame < StartFrame)
            {
                EndFrame = StartFrame + 1;
                CurrentFrame = StartFrame;
            }
        }
        ImGui::PopStyleVar();

        // processing play...
        if (!IsPlaying) return;
        static float TimePassed = 0.f;
        TimePassed += (1.0f / ImGui::GetIO().Framerate);
        if (TimePassed < (1 / DataBrowser::Get().SelectedClipInfo.FPS)) return;
        TimePassed = 0.f;
        DisplayFrame(CurrentFrame);

        CurrentFrame += 1;
        //check should release cap and stop play
        if (CurrentFrame == EndFrame)
        {
            IsPlaying = false;
        }
    }

    void Annotation::RenderAnnotationToolbar()
    {
        static float ButtonsOffset;
        ImGui::SetCursorPosX(ButtonsOffset);
        ImGui::BeginGroup();
        {
            ImGui::PushFont(Setting::Get().TitleFont);
            ImVec2 BtnSize(ImGui::GetFont()->FontSize * 5, ImGui::GetFont()->FontSize * 2.5f);
            if (ImGui::Button(ICON_FA_EDIT, BtnSize)) // add
            {
                EditMode = EAnnotationEditMode::Add;
            }
            Utils::AddSimpleTooltip("Add annotation");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_VECTOR_SQUARE, BtnSize)) // edit
            {
                EditMode = EAnnotationEditMode::Edit;
            }
            Utils::AddSimpleTooltip("Edit annotation");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_SYNC, BtnSize)) // reset pan zoom
            {
                PanAmount = ImVec2(0, 0);
                ZoomLevel = 0;
            }
            Utils::AddSimpleTooltip("Reset zoon and pan");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_CHEVRON_LEFT, BtnSize))
            {
                spdlog::info("???");
                for (int i = CurrentFrame-1; i > -1; i--)
                {
                    if (Data.count(i))
                    {
                        MoveFrame(i);
                        break;
                    }
                }
            }
            Utils::AddSimpleTooltip("Previous annotated frame/image");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_CHEVRON_RIGHT, BtnSize))
            {
                for (int i = CurrentFrame+1; i < DataBrowser::Get().SelectedClipInfo.FrameCount - 1; i++ )
                {
                    if (Data.count(i))
                    {
                        MoveFrame(i);
                        break;
                    }
                }
            }
            Utils::AddSimpleTooltip("Next annotated frame/image");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_TIMES, BtnSize))
            {
                Data.erase(CurrentFrame);
            }
            Utils::AddSimpleTooltip("Delete annotations in this frame/image");
            ImGui::PopFont();
        }
        ImGui::EndGroup();
        ButtonsOffset = (ImGui::GetContentRegionAvail().x - ImGui::GetItemRectSize().x) * 0.5f;
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

    void Annotation::SaveData()
    {
        YAML::Node Origin = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Annotations.yaml"));
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
        YAML::Emitter Out;
        Out << Origin;
        std::ofstream Fout(Setting::Get().ProjectPath + std::string("/Data/Annotations.yaml"));
        Fout << Out.c_str();
    }

    void Annotation::LoadData()
    {
        Data.clear();
        CheckData.clear();
        YAML::Node ClipData = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Annotations.yaml"))
            [DataBrowser::Get().SelectedClipInfo.ClipPath];
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
