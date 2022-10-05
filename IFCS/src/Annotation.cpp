#include "Annotation.h"

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
        std::string Title = "Bad Title...";
        // std::string Title = DataBrowser::Get().GetClipName() + "......(" + std::to_string(
        //     DataBrowser::Get().SelectedFrame) + ")";
        ImGui::PushFont(Setting::Get().TitleFont);
        ImVec2 TitleSize = ImGui::CalcTextSize(Title.c_str());
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - TitleSize.x) * 0.5f);
        ImGui::Text(Title.c_str());
        ImGui::PopFont();

        ImU32 BgColor = ImGui::ColorConvertFloat4ToU32(Spectrum::GRAY(600, Setting::Get().Theme == ETheme::Light));
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - WorkArea.x) * 0.5f);
        ImGui::BeginChild("AnnotaitonWork", WorkArea, true); // show border for debug...
        ImGuiWindow* Win = ImGui::GetCurrentWindow();
        WorkStartPos = ImGui::GetItemRectMin();
        Win->DrawList->AddRectFilled(WorkStartPos, WorkStartPos + WorkArea, BgColor); // draw bg
        Win->DrawList->AddImage((void*)(intptr_t)ImageID, WorkStartPos + PanAmount,
                                WorkStartPos + PanAmount + GetZoom() * WorkArea); // image
        // render things from data
        for (const auto& [ID, A] : Data)
        {
            FCategory Category = CategoryManagement::Get().Data[A.CategoryID];
            if (!Category.Visibility) continue;
            std::array<float, 4> TXYWH = TransformXYWH(A.XYWH);
            float X = TXYWH[0];
            float Y = TXYWH[1];
            float W = TXYWH[2];
            float H = TXYWH[3];
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
            float CircleRadius = 12.f;
            if (CurrentMode == EAnnotationEditMode::Edit)
            {
                Win->DrawList->AddCircleFilled(C, CircleRadius, Color); // pan controller Center
                Win->DrawList->AddCircleFilled(TL, CircleRadius, Color); // resize top left
                Win->DrawList->AddCircleFilled(BL, CircleRadius, Color); // resize bottom left
                Win->DrawList->AddCircleFilled(BR, CircleRadius, Color); // resize bottom right
                Win->DrawList->AddCircleFilled(TR, CircleRadius, Color); // resize top right
                // TODO: add many invisible buttons?
            }
            else if (CurrentMode == EAnnotationEditMode::Reassign)
            {
                Win->DrawList->AddText(ImGui::GetFont(), 28.f, TL - ImVec2(30.f, 30), Color,
                                       ICON_FA_LONG_ARROW_ALT_RIGHT); // Assign icon!!
            }
            else if (CurrentMode == EAnnotationEditMode::Remove)
            {
                // it's not stable enough to detect hover...
                // ImVec2 mouse_pos_projected_on_segment = ImLineClosestPoint(TL, BR, ImGui::GetMousePos());
                // ImVec2 mouse_pos_delta_to_segment = mouse_pos_projected_on_segment - ImGui::GetMousePos();
                // if ((ImLengthSqr(mouse_pos_delta_to_segment) <= 30 * 30)) // is segment hovered

                // TODO: hoverring way to control may be a bad idea... since multi box may overlap together...
                ImVec2 MP = ImGui::GetMousePos();
                if (TL.x <= MP.x && MP.x <= BR.x && TL.y <= MP.y && MP.y <= BR.y)
                {
                    Win->DrawList->AddRectFilled(TL + 1.f, BR - 1.f, ImGui::ColorConvertFloat4ToU32(Spectrum::RED()));
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                    if (ImGui::IsMouseClicked(0))
                    {
                        LogPanel::Get().AddLog(ELogLevel::Warning, "Remove an annotation!");
                    // TODO: implement real delete...
                    }
                }
            }
        }
        // render working stuff
        if (IsAdding)
        {
            ImU32 NewColor = ImGui::ColorConvertFloat4ToU32(CategoryManagement::Get().GetSelectedCategory()->Color);
            Win->DrawList->AddRect(AddPointStart, ImGui::GetMousePos(), NewColor, 0, 0, 3);
        }
        spdlog::info("pan {}, {}: zoom: {}", PanAmount.x, PanAmount.y, GetZoom());
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
                // TODO: when to save?
            }
        }
        // else
        // {
        //     IsAdding = false;
        // }

        ImGui::SetCursorPosX(ButtonsOffset);
        ImGui::BeginGroup();
        {
            ImGui::PushFont(Setting::Get().TitleFont);
            ImVec2 BtnSize(ImGui::GetFont()->FontSize * 5, ImGui::GetFont()->FontSize * 2.5f);
            if (ImGui::Button(ICON_FA_EDIT, BtnSize)) // add
            {
                CurrentMode = EAnnotationEditMode::Add;
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_VECTOR_SQUARE, BtnSize)) // edit
            {
                CurrentMode = EAnnotationEditMode::Edit;
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_LONG_ARROW_ALT_RIGHT, BtnSize)) // reassign
            {
                CurrentMode = EAnnotationEditMode::Reassign;
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_TRASH, BtnSize)) // remove
            {
                CurrentMode = EAnnotationEditMode::Remove;
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_SYNC, BtnSize)) // reset pan zoom
            {
                PanAmount = ImVec2(0, 0);
                ZoomLevel = 0;
            }
            ImGui::PopFont();
        }
        ImGui::EndGroup();
        ButtonsOffset = (ImGui::GetContentRegionAvail().x - ImGui::GetItemRectSize().x) * 0.5f;
    }

    void Annotation::Save()
    {
    }

    void Annotation::Load()
    {
        YAML::Node Node = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Annotation.yaml"));
        // for (YAML::const_iterator it = Node.begin(); it != Node.end(); ++it)
        //     Data[it->first.as<uint64_t>()] = FAnnotation(it->second.as<YAML::Node>());
        // load image here...
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
        ImVec2 ModRectMax;
        ImVec2 WorkAreaMax = WorkStartPos + WorkArea;
        ModRectMax.x = RectMax.x >= WorkAreaMax.x ? WorkAreaMax.x : RectMax.x;
        ModRectMax.y = RectMax.y >= WorkAreaMax.y ? WorkAreaMax.y : RectMax.y;
        ImVec2 RawRectDiff = ModRectMax - RectMin;
        RectMin = RectMin - WorkStartPos - PanAmount;
        RectMax = RectMin + RawRectDiff / GetZoom();
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
        // int OldZoomLevel = ZoomLevel;
        // ImVec2 ZoomPos = ImGui::GetMousePos() - ImGui::GetItemRectMin();
        // ZoomLevel += int(Wheel);
        // if (ZoomLevel < -3) ZoomLevel = -3;
        // if (ZoomLevel > 3) ZoomLevel = 3;
        // // make same position after zoom
        // ImVec2 ImgPos = (ZoomPos - PanAmount) / GetZoom(OldZoomLevel);
        // PanAmount = ZoomPos - ImgPos * GetZoom();
    }
}
