#include "CategoryManagement.h"
#include <fstream>
#include <yaml-cpp/yaml.h>

#include "imgui_internal.h"
#include "Setting.h"
#include "ImguiNotify/font_awesome_5.h"
#include "Spectrum/imgui_spectrum.h"


namespace IFCS
{
    void CategoryManagement::Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose)
    {
        Panel::Setup(InName, InShouldOpen, InFlags, InCanClose);
        LoadCategoriesFromFile();
    }
    
    void CategoryManagement::RenderContent()
    {
        const float LineHeight = ImGui::GetTextLineHeightWithSpacing();
        for (FCategory& cat : Categories)
        {
            ImGui::PushID(cat.ID);
            // TODO: move a few pixel down for this eye button? it's just not aligned correctly
            const char* VisibilityIcon = cat.Visibility ? ICON_FA_EYE : ICON_FA_EYE_SLASH;
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
            ImGui::Selectable(cat.DisplayName.c_str(), cat.ID == SelectedCatID, ImGuiSelectableFlags_AllowDoubleClick,
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
            Categories.push_back(FCategory(NewCatName));
            NewCatName[0] = 0;
            Save();
        }
    }

    void CategoryManagement::LoadCategoriesFromFile()
    {
        if (!Setting::Get().ProjectIsLoaded) return;
        YAML::Node Node = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Categories.yaml"));
        for (YAML::const_iterator it = Node.begin(); it != Node.end(); ++it)
        {
            Categories.push_back(FCategory(it->first.as<uint64_t>(), it->second.as<YAML::Node>())); 
        }
    }

    void CategoryManagement::Save()
    {
        YAML::Node Node = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Categories.yaml"));
        for (const FCategory& cat : Categories)
        {
            Node[std::to_string(cat.ID)] = cat.Serialize();
        }
        YAML::Emitter Out;
        Out << Node;
        std::ofstream Fout(Setting::Get().ProjectPath + std::string("/Data/Categories.yaml"));
        Fout << Out.c_str();
    }



    // TODO: pass... need get access to Curent Edit Frame!
    void CategoryManagement::UpdateCategoryStatics()
    {
        for (FCategory& cat : Categories)
        {
            cat.TotalUsedCount = 100;
            cat.UsedCountInThisFrame = 100;
        }
    }
}
