#include "CategoryManagement.h"

#include "imgui_internal.h"
#include "ImguiNotify/font_awesome_5.h"
#include "Spectrum/imgui_spectrum.h"


namespace IFCS
{
    void CategoryManagement::RenderContent()
    {
        const float LineHeight = ImGui::GetTextLineHeightWithSpacing();
        for (FCategory& cat : Categories)
        {
            ImGui::PushID(cat.uid);
            // TODO: move a few pixel down for this eye button? it's just not aligned correctly
            const char* VisibilityIcon = cat.Visibility? ICON_FA_EYE : ICON_FA_EYE_SLASH;
            if (ImGui::InvisibleButton("##vis_btn", ImGui::CalcTextSize(VisibilityIcon)))
            {
                cat.Visibility = !cat.Visibility;
            }
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::CalcTextSize(VisibilityIcon).x);
            ImGui::Text(VisibilityIcon);
            ImGui::SameLine();
            ImGui::ColorEdit3("##cat_color_picker", (float*)&cat.Color, ImGuiColorEditFlags_NoInputs);
            ImGui::SameLine();
            ImGui::Selectable(cat.DisplayName.c_str(), cat.uid == SelectedCatID, ImGuiSelectableFlags_AllowDoubleClick,
                              {120, LineHeight});
            ImGui::SameLine();
            ImGui::Text("(%d/%d)", cat.UsedCountInThisFrame, cat.TotalUsedCount);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("(use annotation in: this frame / all project)");
            ImGui::PopID();
        }
        ImGui::Separator();
        ImGui::SetNextItemWidth(120);
        ImGui::InputText("##new cat name", NewCatName, IM_ARRAYSIZE(NewCatName));
        ImGui::SameLine();
        if (ImGui::Button("Add"))
        {
            Categories.push_back(FCategory(NewCatName, Spectrum::CELERY(), 0));
            NewCatName[0] = 0;
        }
    }
}
