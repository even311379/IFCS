#include "Annotation.h"

#include <fstream>

#include "CategoryManagement.h"
#include "DataBrowser.h"
#include "imgui_internal.h"
#include "Log.h"
#include "Setting.h"
#include "ImguiNotify/font_awesome_5.h"
#include "Spectrum/imgui_spectrum.h"

#include "yaml-cpp/yaml.h"

namespace IFCS
{
    void Annotation::RenderContent()
    {
        if (!DataBrowser::Get().AnyFrameLoaded) return;

        std::string Title = DataBrowser::Get().FrameTitle;
        ImGui::PushFont(Setting::Get().TitleFont);
        float TitleWidth = ImGui::CalcTextSize(Title.c_str()).x;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - TitleWidth) * 0.5f);
        ImGui::Text(Title.c_str());
        ImGui::PopFont();

        ImU32 BgColor = ImGui::ColorConvertFloat4ToU32(Spectrum::GRAY(600, Setting::Get().Theme == ETheme::Light));
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - WorkArea.x) * 0.5f);
        ImGui::BeginChild("AnnotaitonWork", WorkArea, true); // show border for debug...
        ImGuiWindow* Win = ImGui::GetCurrentWindow();
        WorkStartPos = ImGui::GetItemRectMin();
        Win->DrawList->AddRectFilled(WorkStartPos, WorkStartPos + WorkArea, BgColor); // draw bg
        Win->DrawList->AddImage((void*)(intptr_t)DataBrowser::Get().LoadedFramePtr, WorkStartPos + PanAmount,
                                WorkStartPos + PanAmount + GetZoom() * WorkArea); // image
        // render things from data
        for (const auto& [ID, A] : Data)
        {
            FCategory Category = CategoryManagement::Get().Data[A.CategoryID];
            if (!Category.Visibility) continue;
            std::array<float, 4> TransformedXYWH = TransformXYWH(A.XYWH);
            float X = TransformedXYWH[0];
            float Y = TransformedXYWH[1];
            float W = TransformedXYWH[2];
            float H = TransformedXYWH[3];
            ImVec2 TL, BL, BR, TR, C; // top-left, bottom-left, bottom-right, top-right, center
            // TODO: zoom and pan?
            TL = {X - 0.5f * W, Y - 0.5f * H};
            BL = {X - 0.5f * W, Y + 0.5f * H};
            BR = {X + 0.5f * W, Y + 0.5f * H};
            TR = {X + 0.5f * W, Y - 0.5f * H};
            C = {X, Y};

            ImU32 Color = ImGui::ColorConvertFloat4ToU32(Category.Color);
            Win->DrawList->AddRect(TL, BR, Color, 0, 0, 2);
            // annotation box
            Win->DrawList->AddText(ImGui::GetFont(), 24.f, TL - ImVec2(0, 30.f), Color,
                                   Category.DisplayName.c_str()); // display name
            float CircleRadius = 6.f;
            float PanControlRadius = 12.f;
            ImGui::PushID(int(ID));
            const bool IsMouseDragging = ImGui::IsMouseDragging(0);
            // TODO: maybe change color when hover control btn?
            if (CurrentMode == EAnnotationEditMode::Edit)
            {
                // pan controller Center
                Win->DrawList->AddCircleFilled(C, PanControlRadius, Color);
                ImGui::SetCursorScreenPos(C - ImVec2(PanControlRadius, PanControlRadius));
                ImGui::InvisibleButton("Pan", ImVec2(PanControlRadius * 2, PanControlRadius * 2));
                bool IsPanActive = ImGui::IsItemActive();
                if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                if (IsPanActive && IsMouseDragging)
                {
                    ImVec2 Delta = ImGui::GetIO().MouseDelta / GetZoom();
                    Data[ID].Pan({Delta.x, Delta.y});
                    IsCurrentFrameModified = true;
                }
                // resize top left
                ResizeImpl(Win, EBoxCorner::TopLeft, TL, Color, IsMouseDragging, ID);
                // resize bottom left
                ResizeImpl(Win, EBoxCorner::BottomLeft, BL, Color, IsMouseDragging, ID);
                // resize bottom right
                ResizeImpl(Win, EBoxCorner::BottomRight, BR, Color, IsMouseDragging, ID);
                // resize top right
                ResizeImpl(Win, EBoxCorner::TopRight, TR, Color, IsMouseDragging, ID);

                // trash
                ImVec2 TrashPos = TR + ImVec2(24.f, -36);
                Win->DrawList->AddText(ImGui::GetFont(), 36.f, TrashPos, Color,
                                       ICON_FA_TRASH_ALT);
                ImGui::SetCursorScreenPos(TrashPos);
                if (ImGui::InvisibleButton("Trash", ImVec2(36, 36)))
                {
                    Trash(ID);
                }
                if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

                // reassign
                ImVec2 ReassignPos = TL - ImVec2(48, 36);
                Win->DrawList->AddText(ImGui::GetFont(), 36.f, ReassignPos, Color,
                                       ICON_FA_LONG_ARROW_ALT_RIGHT);
                ImGui::SetCursorScreenPos(ReassignPos);
                if (ImGui::InvisibleButton("Reassign", ImVec2(36, 36)))
                {
                    spdlog::info("Reassign is clicked");
                    Reassign(ID);
                }
                if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            ImGui::PopID();
        }
        // render working stuff
        if (IsAdding)
        {
            ImU32 NewColor = ImGui::ColorConvertFloat4ToU32(CategoryManagement::Get().GetSelectedCategory()->Color);
            Win->DrawList->AddRect(AddPointStart, ImGui::GetMousePos(), NewColor, 0, 0, 3);
        }
        ImGui::EndChild();
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
            if (CurrentMode == EAnnotationEditMode::Add && ImGui::IsMouseClicked(0))
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
                GetAbsRectMinMax(AddPointStart, ImGui::GetMousePos(), AbsMin, AbsMax);
                std::array<float, 4> NewXYWH = MouseRectToXYWH(AbsMin, AbsMax);
                Data[UUID()] = FAnnotation(CategoryManagement::Get().SelectedCatID, NewXYWH);
                IsCurrentFrameModified = true;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_W))
            {
                CurrentMode = EAnnotationEditMode::Add;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_E))
            {
                CurrentMode = EAnnotationEditMode::Edit;
            }
        }

        ImGui::SetCursorPosX(ButtonsOffset);
        ImGui::BeginGroup();
        {
            ImGui::PushFont(Setting::Get().TitleFont);
            ImVec2 BtnSize(ImGui::GetFont()->FontSize * 5, ImGui::GetFont()->FontSize * 2.5f);
            if (ImGui::Button(ICON_FA_EDIT, BtnSize)) // add
            {
                CurrentMode = EAnnotationEditMode::Add;
            }
            Utils::AddSimpleTooltip("Add annotation");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_VECTOR_SQUARE, BtnSize)) // edit
            {
                CurrentMode = EAnnotationEditMode::Edit;
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
            if (ImGui::Button(ICON_FA_CHEVRON_LEFT, BtnSize)) // reset pan zoom
            {
                DataBrowser::Get().LoadOtherFrame(false);
            }
            Utils::AddSimpleTooltip("Previous frame");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_CHEVRON_RIGHT, BtnSize)) // reset pan zoom
            {
                DataBrowser::Get().LoadOtherFrame(true);
            }
            Utils::AddSimpleTooltip("Next frame");
            ImGui::PopFont();
        }
        ImGui::EndGroup();
        ButtonsOffset = (ImGui::GetContentRegionAvail().x - ImGui::GetItemRectSize().x) * 0.5f;
    }

    void Annotation::PostRender()
    {
        if (ToTrashID != 0)
        {
            Data.erase(ToTrashID);
            ToTrashID = 0;
        }
    }

    void Annotation::Save()
    {
        if (!IsCurrentFrameModified) return;
        YAML::Node DataNode;
        for (const auto& [k,v] : Data)
            DataNode[std::to_string(k)] = v.Serialize();

        // TODO: this make out of order, but it shouldn't...
        YAML::Node OutNode = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Annotation.yaml"));
        OutNode[DataBrowser::Get().SelectedClipInfo.ClipPath][std::to_string(DataBrowser::Get().SelectedFrame)] =
            DataNode;

        YAML::Emitter Out;
        Out << OutNode;
        std::ofstream Fout(Setting::Get().ProjectPath + std::string("/Data/Annotation.yaml"));
        Fout << Out.c_str();
        IsCurrentFrameModified = true;
    }

    void Annotation::Load()
    {
        Data.clear();
        YAML::Node Node = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Annotation.yaml"))[
            DataBrowser::Get().SelectedClipInfo.ClipPath][std::to_string(DataBrowser::Get().SelectedFrame)];
        for (YAML::const_iterator it = Node.begin(); it != Node.end(); ++it)
        {
            Data[UUID(it->first.as<uint64_t>())] = FAnnotation(it->second);
        }
    }

    void Annotation::ResizeImpl(ImGuiWindow* WinPtr, const EBoxCorner& WhichCorner, const ImVec2& InPos,
                                const ImU32& InColor, const bool& IsDragging, const UUID& InID)
    {
        const float CircleRadius = 9.f;
        WinPtr->DrawList->AddCircleFilled(InPos, CircleRadius, InColor);
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
        bool IsResizeActive = ImGui::IsItemActive();
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (IsResizeActive && IsDragging)
        {
            ImVec2 Delta = ImGui::GetIO().MouseDelta / GetZoom();
            Data[InID].Resize(WhichCorner, {Delta.x, Delta.y});
            IsCurrentFrameModified = true;
        }
    }

    void Annotation::Reassign(UUID ID)
    {
        UUID NewCat = CategoryManagement::Get().SelectedCatID;
        if (NewCat != 0)
            Data[ID].CategoryID = NewCat;
    }

    void Annotation::Trash(UUID ID)
    {
        ToTrashID = ID;
        IsCurrentFrameModified = true;
    }

    float Annotation::GetZoom()
    {
        return GetZoom(ZoomLevel);
    }

    float Annotation::GetZoom(int InZoom)
    {
        switch (InZoom)
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

    void Annotation::GetAbsRectMinMax(ImVec2 p0, ImVec2 p1, ImVec2& OutMin, ImVec2& OutMax)
    {
        OutMin.x = std::min(p0.x, p1.x);
        OutMin.y = std::min(p0.y, p1.y);
        OutMax.x = std::max(p0.x, p1.x);
        OutMax.y = std::max(p0.y, p1.y);
    }

    std::array<float, 4> Annotation::MouseRectToXYWH(ImVec2 RectMin, ImVec2 RectMax)
    {
        // ImVec2 ModRectMax;
        // ImVec2 WorkAreaMax = WorkStartPos + WorkArea;
        // // TODO: fix more clamp response...
        // ModRectMax.x = RectMax.x >= WorkAreaMax.x ? WorkAreaMax.x : RectMax.x;
        // ModRectMax.y = RectMax.y >= WorkAreaMax.y ? WorkAreaMax.y : RectMax.y;
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

    std::array<float, 4> Annotation::TransformXYWH(const std::array<float, 4>& InXYWH)
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
}
