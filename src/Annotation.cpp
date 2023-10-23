#include "Annotation.h"

#include <fstream>
#include <regex>

#include "CategoryManagement.h"
#include "DataBrowser.h"
#include "Setting.h"
#include "Style.h"
#include "UUID.h"
#include "Application.h"
#include "Utils.h"
#include "Imspinner/imspinner.h"
#include "fa_solid_900.h"
#include <IconFontCppHeaders/IconsFontAwesome5.h>

#include "yaml-cpp/yaml.h"
#include "spdlog/spdlog.h"

// use this macro to reduce typing...
#define DB DataBrowser::Get()

namespace IFCS
{
    static bool NeedSave = false;
    static bool NeedUpdateCategoryStatics = false;
    static int Tick = 0;
    static ImVec2 WorkStartPos;
    static bool IsPlaying;
    static int FrameJumpSize = 1;

    void Annotation::Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose)
    {
        Panel::Setup(InName, InShouldOpen, InFlags, InCanClose);
        LoadDataFile();
    }

    std::map<int, FAnnotationToDisplay> Annotation::GetAnnotationToDisplay()
    {
        std::map<int, FAnnotationToDisplay> Out;
        for (const auto& [F, S] : Data_Frame)
        {
            Out[F] = FAnnotationToDisplay(int(S.AnnotationData.size()), S.IsReady);
        }
        return Out;
    }


    void Annotation::RenderContent()
    {
        RenderSelectCombo();

        RenderAnnotationWork();

        ImGui::Dummy({0, ImGui::GetTextLineHeightWithSpacing()});
        if (!DB.SelectedClipInfo.ClipPath.empty())
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
            for (size_t T = 0; T < Setting::Get().CoresToUse; T++)
            {
                if (DB.LoadingVideoTasks[T].wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
                {
                    NumCoreFinished++;
                }
            }
            if (NumCoreFinished == Setting::Get().CoresToUse)
            {
                IsLoadingVideo = false;
                if (DB.ForceCloseLoading)
                {
                    DB.ForceCloseLoading = false;
                    DB.PrepareVideo(IsLoadingVideo);
                }
            }
        }

        if (Tick > 15)
        {
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
            Tick = 0;
        }
        Tick++;
    }


    void Annotation::RenderSelectCombo()
    {
        ImGui::SetNextItemWidth(480);
        if (DB.IsImageDisplayed)
        {
            const size_t RelOffset = Setting::Get().ProjectPath.size() + 8;
            if (ImGui::BeginCombo("##ImageSelect", DB.SelectedImageInfo.GetRelativePath().c_str()))
            {
                for (const std::string& Img : DB.GetAllImages())
                {
                    if (ImGui::Selectable(Img.substr(RelOffset).c_str()))
                    {
                        DB.SelectedClipInfo.ClipPath = ""; // Deselect Clip
                        DB.SelectedImageInfo.ImagePath = Img;
                        DB.DisplayImage();
                    }
                }
                ImGui::EndCombo();
            }
        }
        else
        {
            const size_t RelOffset = Setting::Get().ProjectPath.size() + 7;
            if (ImGui::BeginCombo("##ClipSelect", DB.SelectedClipInfo.GetRelativePath().c_str()))
            {
                for (const std::string& C : DB.GetAllClips())
                {
                    if (ImGui::Selectable(C.substr(RelOffset).c_str()))
                    {
                        DB.SelectedImageInfo.ImagePath = ""; // Deselect image...
                        DB.SelectedClipInfo.ClipPath = C;
                        DB.PrepareVideo(IsLoadingVideo);
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            if (ImGui::BeginCombo("##ClipFrame", std::to_string(DB.CurrentFrame).c_str()))
            {
                for (const auto& [F, Map] : Data_Frame)
                {
                    if (ImGui::Selectable(std::to_string(F).c_str(), F == DB.CurrentFrame))
                    {
                        DB.MoveFrame(F);
                    }
                }
                ImGui::EndCombo();
            }
        }
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
        if (DB.IsImageDisplayed)
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
        ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)DB.LoadedFramePtr,
                                             WorkStartPos + PanAmount,
                                             WorkStartPos + PanAmount + GetZoom() * WorkArea); // image
        if (Setting::Get().bEnableGuideLine)
        {
            ImVec2 MousePos = ImGui::GetMousePos();
            if (Utils::IsPointInsideRect(MousePos, WorkStartPos, WorkStartPos + WorkArea))
            {
                const ImU32 GuideCol = ImGui::ColorConvertFloat4ToU32({ Setting::Get().GuidelineColor[0], Setting::Get().GuidelineColor[1], Setting::Get().GuidelineColor[2], 1.0f});
                ImGui::GetWindowDrawList()->AddLine({MousePos.x, WorkStartPos.y}, {MousePos.x, WorkStartPos.y + WorkArea.y}, GuideCol, 1);
                ImGui::GetWindowDrawList()->AddLine({WorkStartPos.x, MousePos.y}, {WorkStartPos.x + WorkArea.x, MousePos.y}, GuideCol, 1);
            }
        }
        static UUID ToDeleteAnnID = 0;
        std::unordered_map<UUID, FAnnotation>* DataToDisplay = nullptr;
        if (DB.IsImageDisplayed)
        {
            DataToDisplay = &Data_Img.AnnotationData;
        }
        else
        {
            if (Data_Frame.count(DB.CurrentFrame))
                DataToDisplay = &Data_Frame[DB.CurrentFrame].AnnotationData;
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
                        if (DB.IsImageDisplayed)
                            Data_Img.UpdateTime = Utils::GetCurrentTimeString();
                        else
                            Data_Frame[DB.CurrentFrame].UpdateTime = Utils::GetCurrentTimeString();
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
                                if (DB.IsImageDisplayed)
                                    Data_Img.UpdateTime = Utils::GetCurrentTimeString();
                                else
                                    Data_Frame[DB.CurrentFrame].UpdateTime = Utils::GetCurrentTimeString();
                                NeedUpdateCategoryStatics = true;
                                NeedSave = true;
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImVec2 DelPos = TR + ImVec2(PanControlRadius, 0);
                    ImGui::SetCursorScreenPos(DelPos);
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
                    if (ImGui::Button(ICON_FA_TIMES_CIRCLE))
                    {
                        ToDeleteAnnID = ID;
                        if (DB.IsImageDisplayed)
                            Data_Img.UpdateTime = Utils::GetCurrentTimeString();
                        else
                            Data_Frame[DB.CurrentFrame].UpdateTime = Utils::GetCurrentTimeString();
                    }
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor();
                }
                ImGui::PopID();
            }
        }
        if (ToDeleteAnnID != 0)
        {
            if (DB.IsImageDisplayed)
                Data_Img.AnnotationData.erase(ToDeleteAnnID);
            else
                Data_Frame[DB.CurrentFrame].AnnotationData.erase(ToDeleteAnnID);

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
            if (EditMode == EAnnotationEditMode::Add && ImGui::IsMouseClicked(0) && DB.
                IsSelectAnyClipOrImg())
            {
                if (CategoryManagement::Get().GetSelectedCategory())
                {
                    IsAdding = true;
                    AddPointStart = ImGui::GetMousePos();
                }
            }
        }

        /////////////////
        // HOTKEYS
        ////////////////
        // Make hotkey all avail on ann workspace...
        if (Utils::IsPointInsideRect(ImGui::GetMousePos(),
            ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize()))
        {
            if (ImGui::IsKeyPressed(ImGuiKey_W))
            {
                EditMode = EAnnotationEditMode::Add;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_E))
            {
                EditMode = EAnnotationEditMode::Edit;
            }
            if (!DB.IsImageDisplayed)
            {
                if (ImGui::IsKeyPressed(ImGuiKey_A))
                {
                    if (DB.CurrentFrame == DB.VideoEndFrame)
                        DB.CurrentFrame = DB.VideoStartFrame;
                    IsPlaying = !IsPlaying;
                    // TODO: very hack to fix this 1 frame jump when pressing pause issue... not sure this is actually the solution? or maybe trigger other bugs in other video
                    
                    // fix 1 frame diff when press playing button?
                    if (DB.CurrentFrame > 0 && !IsPlaying)
                        DB.CurrentFrame -= 1;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_S))
                {
                    if (!IsPlaying)
                        DB.MoveFrame(DB.CurrentFrame - FrameJumpSize);
                }
                if (ImGui::IsKeyPressed(ImGuiKey_D))
                {
                    if (!IsPlaying)
                        DB.MoveFrame(DB.CurrentFrame + FrameJumpSize);
                }
            }
        }
        
        if (ImGui::IsMouseReleased(0) && IsAdding)
        {
            IsAdding = false;
            ImVec2 AbsMin, AbsMax;
            Utils::GetAbsRectMinMax(AddPointStart, ImGui::GetMousePos(), AbsMin, AbsMax);
            bool WasSuccess;
            std::array<float, 4> NewXYWH = MouseRectToXYWH(AbsMin, AbsMax, WasSuccess);
            if (WasSuccess)
            {
                if (DB.IsImageDisplayed)
                {
                    Data_Img.AnnotationData[UUID()] = FAnnotation(CategoryManagement::Get().SelectedCatID, NewXYWH);
                    Data_Img.UpdateTime = Utils::GetCurrentTimeString();
                }
                else
                {
                    Data_Frame[DB.CurrentFrame].AnnotationData[UUID()] = FAnnotation(CategoryManagement::Get().SelectedCatID, NewXYWH);
                    Data_Frame[DB.CurrentFrame].UpdateTime = Utils::GetCurrentTimeString();
                }
                CategoryManagement::Get().AddCount();
                NeedSave = true;
            }
        }
    }

    void Annotation::RenderVideoControls()
    {
        // TODO: linux temp fix
        static std::string PlayIcon;
        if (DB.CurrentFrame == DB.VideoEndFrame)
            PlayIcon = ICON_FA_SYNC;
        else if (IsPlaying)
            PlayIcon = ICON_FA_PAUSE;
        else
            PlayIcon = ICON_FA_PLAY;
        if (ImGui::Button(PlayIcon.c_str(), {120, 32}))
        // if (ImGui::Button(ICON_FA_PLAY, {120, 32}))
        {
            if (DB.CurrentFrame == DB.VideoEndFrame)
                DB.CurrentFrame = DB.VideoStartFrame;
            IsPlaying = !IsPlaying;
            // fix 1 frame diff when press playing button?
            if (DB.CurrentFrame > 0 && !IsPlaying)
                DB.CurrentFrame -= 1;

        }
        Utils::AddSimpleTooltip("Play (A)");
        ImGui::SameLine();
        static ImVec2 NewFramePad(4, (32 - ImGui::GetFontSize()) / 2);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, NewFramePad);
        ImGui::SetNextItemWidth(120.f);
        if (ImGui::DragInt("##PlayStart", &DB.VideoStartFrame, 1, 0, DB.VideoEndFrame - 1))
        {
            if (DB.CurrentFrame < DB.VideoStartFrame) DB.CurrentFrame = DB.VideoStartFrame;
            if (DB.VideoStartFrame > DB.VideoEndFrame)
            {
                DB.VideoStartFrame = DB.VideoEndFrame - 1;
                DB.CurrentFrame = DB.VideoStartFrame;
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
        ImVec2 RectPlayEnd = RectStart + ImVec2(float(DB.CurrentFrame - DB.VideoStartFrame) / float(DB.VideoEndFrame - DB.VideoStartFrame) * Width,
                                                10);
        ImGui::GetWindowDrawList()->AddRectFilled(RectPlayStart, RectPlayEnd, PlayColor);
        // draw current frame text?
        if (RectPlayEnd.x > RectPlayStart.x + 30)
        {
            char FrameTxt[8];
            sprintf(FrameTxt, "%d", DB.CurrentFrame);
            ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 16,
                                                RectPlayEnd + ImVec2(strlen(FrameTxt) * -16.f, -18.f), Color, FrameTxt);
        }
        ImGui::SetCursorScreenPos(RectPlayStart);
        ImGui::InvisibleButton("PlayControl", ImVec2(Width, 15));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            const int FrameAtPosition = int(
                (ImGui::GetMousePos().x - RectStart.x) / Width * (float)(DB.VideoEndFrame - DB.VideoStartFrame)) + DB.VideoStartFrame;
            ImGui::SetTooltip("%d", FrameAtPosition);
        }
        if (ImGui::IsItemClicked(0) && !IsLoadingVideo)
        {
            int NewFramePos = int((ImGui::GetMousePos().x - RectStart.x) / Width * (float)(DB.VideoEndFrame - DB.VideoStartFrame)) +
                DB.VideoStartFrame;
            DB.MoveFrame(NewFramePos);
            IsPlaying = false;
            DB.DisplayFrame(DB.CurrentFrame);
            DB.LoadingVideoBlock(IsLoadingVideo, DB.CurrentFrame);
        }

        ImGui::SameLine();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, NewFramePad);
        ImGui::SetNextItemWidth(120.f);
        int MaxFrame = DB.SelectedClipInfo.FrameCount - 1;
        if (ImGui::DragInt("##PlayEnd", &DB.VideoEndFrame, 1, DB.VideoStartFrame + 1, MaxFrame))
        {
            if (DB.CurrentFrame > DB.VideoEndFrame) DB.CurrentFrame = DB.VideoEndFrame;
            if (DB.VideoEndFrame < DB.VideoStartFrame)
            {
                DB.VideoEndFrame = DB.VideoStartFrame + 1;
                DB.CurrentFrame = DB.VideoStartFrame;
            }
            if (DB.VideoEndFrame > MaxFrame) DB.VideoEndFrame = MaxFrame;
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
                    DB.MoveFrame(DB.CurrentFrame - FrameJumpSize);
            }
            Utils::AddSimpleTooltip("Jump backward frames (S)");
            ImGui::SameLine();
            char buf1[32];
            sprintf(buf1, "%s %d", ICON_FA_PLUS, FrameJumpSize);
            if (ImGui::Button(buf1, BtnSize))
            {
                if (!IsPlaying)
                    DB.MoveFrame(DB.CurrentFrame + FrameJumpSize);
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
                DB.VideoStartFrame = 0;
                DB.VideoEndFrame = MaxFrame;
            }
            Utils::AddSimpleTooltip("Reset play start and end");
        }
        ImGui::EndGroup();
        ButtonsOffset = (ImGui::GetContentRegionAvail().x - ImGui::GetItemRectSize().x) * 0.5f;

        // processing play...
        if (!IsPlaying) return;
        static float TimePassed = 0.f;
        TimePassed += (1.0f / ImGui::GetIO().Framerate);
        if (TimePassed < (1 / DB.SelectedClipInfo.FPS)) return;
        TimePassed = 0.f;
        DB.DisplayFrame(DB.CurrentFrame);
        DB.LoadingVideoBlock(IsLoadingVideo, DB.CurrentFrame);
        DB.CurrentFrame += 1;
        //check should release cap and stop play
        if (DB.CurrentFrame == DB.VideoEndFrame)
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
            Utils::AddSimpleTooltip("Reset zoom and pan");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_CHEVRON_LEFT, BtnSize))
            {
                if (DB.IsImageDisplayed)
                {
                    std::vector<std::string> AllImgs = DB.GetAllImages();
                    std::string LastFoundImg;
                    for (const auto& ImgPath : AllImgs)
                    {
                        if (DB.SelectedImageInfo.ImagePath == ImgPath)
                        {
                            break;
                        }
                        if (Data.count(ImgPath)) LastFoundImg = ImgPath;
                    }
                    if (!LastFoundImg.empty())
                    {
                        DB.SelectedImageInfo.ImagePath = LastFoundImg;
                        DB.DisplayImage();
                    }
                }
                else
                {
                    for (int i = DB.CurrentFrame - 1; i > -1; i--)
                    {
                        if (Data_Frame.count(i))
                        {
                            DB.MoveFrame(i);
                            break;
                        }
                    }
                }
            }
            Utils::AddSimpleTooltip("Previous annotated frame/image");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_CHEVRON_RIGHT, BtnSize))
            {
                if (DB.IsImageDisplayed)
                {
                    std::vector<std::string> AllImgs = DB.GetAllImages();
                    std::string NextFoundImg;
                    bool ReachedCurrent = false;
                    for (const auto& ImgPath : AllImgs)
                    {
                        if (Data.count(ImgPath) && ReachedCurrent)
                        {
                            NextFoundImg = ImgPath;
                            break;
                        }
                        if (!ReachedCurrent)
                            if (DB.SelectedImageInfo.ImagePath == ImgPath)
                                ReachedCurrent = true;
                    }
                    if (!NextFoundImg.empty())
                    {
                        DB.SelectedImageInfo.ImagePath = NextFoundImg;
                        DB.DisplayImage();
                    }
                }
                else
                {
                    for (int i = DB.CurrentFrame + 1; i < DB.SelectedClipInfo.FrameCount - 1; i++)
                    {
                        if (Data_Frame.count(i))
                        {
                            DB.MoveFrame(i);
                            break;
                        }
                    }
                }
            }
            Utils::AddSimpleTooltip("Next annotated frame/image");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_TIMES, BtnSize))
            {
                if (DB.IsImageDisplayed)
                    Data_Img.AnnotationData.clear();
                else
                    Data_Frame.erase(DB.CurrentFrame);
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

        if (DB.IsImageDisplayed)
        {
            if (!Data_Img.IsEmpty())
            {
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 100);
                if (ImGui::Checkbox("Is ready?", &Data_Img.IsReady))
                {
                    Data_Img.UpdateTime = Utils::GetCurrentTimeString();
                    NeedSave = true;
                    if (Data_Img.IsReady && DB.NeedReviewedOnly )
                    {
                        // TODO: jump to next one which need review (too tedious..., leave for future update? )
                    }
                }
            }
        }
        else
        {
            if (Data_Frame.count(DB.CurrentFrame))
            {
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 100);
                if (ImGui::Checkbox("Is ready?", &Data_Frame[DB.CurrentFrame].IsReady))
                {
                    Data_Frame[DB.CurrentFrame].UpdateTime = Utils::GetCurrentTimeString();
                    NeedSave = true;
                    if (Data_Img.IsReady && DB.NeedReviewedOnly )
                    {
                        // TODO: jump to next one which need review (too tedious..., leave for future update? )
                    }
                }
            }
        }
    }

    void Annotation::SaveDataFile()
    {
        YAML::Node ToSave;
        for (auto [FileName, FrameSave ] : Data)
        {
            for (auto [FrameNum, SaveData] : FrameSave)
            {
                // must save as string.. not int!!!
                ToSave[FileName][std::to_string(FrameNum)] = SaveData.Serialize();
            }
        }
        YAML::Emitter Out;
        Out << ToSave;
        std::ofstream Fout(Setting::Get().ProjectPath + std::string("/Data/Annotations.yaml"));
        Fout << Out.c_str();
        spdlog::info("Data file is saved...");
        NeedSaveFile = false;
        App->RequestToChangeTitle = true;
    }

    void Annotation::LoadDataFile()
    {
        if (!Setting::Get().ProjectIsLoaded) return;
        Data.clear();
        YAML::Node FileData = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/Annotations.yaml");
        for (YAML::const_iterator it = FileData.begin(); it!= FileData.end(); ++it)
        {
            auto FileName = it->first.as<std::string>();
            if (FileName.empty()) continue;
            // FileName = std::regex_replace(FileName, std::regex(Setting::Get().ProjectPath), "");
            // FileName.replace(Setting::Get().ProjectPath, "");
            auto CNode = it->second.as<YAML::Node>();
            for (YAML::const_iterator f = CNode.begin(); f != CNode.end(); ++f)
            {
                auto Frame = f->first.as<int>();
                FAnnotationSave NewSave;
                auto FNode = f->second.as<YAML::Node>();
                for (YAML::const_iterator ANN = FNode.begin(); ANN != FNode.end(); ++ANN)
                {
                    if (ANN->first.as<std::string>() == "UpdateTime")
                    {
                        NewSave.UpdateTime = ANN->second.as<std::string>();
                    }
                    else if (ANN->first.as<std::string>() == "IsReady")
                    {
                        NewSave.IsReady = ANN->second.as<bool>();
                    }
                    else
                    {
                        NewSave.AnnotationData[ANN->first.as<uint64_t>()] = FAnnotation(ANN->second);
                    }
                }
                Data[FileName][Frame] = NewSave;
            }
        }
    }

    void Annotation::SaveData()
    {
        if (DB.IsImageDisplayed)
        {
            const std::string ImgPath = DB.SelectedImageInfo.ImagePath.substr(Setting::Get().ProjectPath.length());
            if (Data_Img.IsEmpty())
            {
                Data.erase(ImgPath);
            }
            else
            {
                Data[ImgPath][0] = Data_Img; 
            }
        }
        else
        {
            if (!DB.SelectedClipInfo.ClipPath.empty())
            {
                const std::string RelPath = DB.SelectedClipInfo.ClipPath.substr(Setting::Get().ProjectPath.length());
                Data[RelPath] = Data_Frame;
            }
        }
        NeedSaveFile = true;
        App->RequestToChangeTitle = true;
    }

    void Annotation::GrabData()
    {
        if (DB.IsImageDisplayed)
        {
            Data_Img.AnnotationData.clear();
            const std::string RelPath = std::regex_replace(DB.SelectedImageInfo.ImagePath, std::regex(Setting::Get().ProjectPath), "");
            if (!Data.count(RelPath)) return;
            Data_Img = Data[RelPath][0];
        }
        else
        {
            const std::string RelPath = std::regex_replace(DB.SelectedClipInfo.ClipPath, std::regex(Setting::Get().ProjectPath), "");
            if (!Data.count(RelPath))
            {
                Data_Frame.clear();
                return;
            }
            Data_Frame = Data[RelPath];
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

    std::array<float, 4> Annotation::MouseRectToXYWH(ImVec2 RectMin, ImVec2 RectMax, bool& bSuccess)
    {
        bSuccess = true;
        ImVec2 RawRectDiff = RectMax - RectMin;
        RectMin = (RectMin - WorkStartPos - PanAmount) / GetZoom();
        RectMax = RectMin + RawRectDiff / GetZoom();
        if (RectMin.x <= 0) RectMin.x = 0;
        if (RectMin.y <= 0) RectMin.y = 0;
        if (RectMax.x >= WorkArea.x) RectMax.x = WorkArea.x;
        if (RectMax.y >= WorkArea.y) RectMax.y = WorkArea.y;
        if (RectMax.x < 0.f || RectMin.x > WorkArea.x || RectMax.y < 0.f || RectMin.y > WorkArea.y)
            bSuccess = false;
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
            if (DB.IsImageDisplayed)
                Data_Img.UpdateTime = Utils::GetCurrentTimeString();
            else
                Data_Frame[DB.CurrentFrame].UpdateTime = Utils::GetCurrentTimeString();
            NeedSave = true;
        }
    }



}
