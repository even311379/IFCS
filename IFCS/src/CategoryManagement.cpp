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

    FCategory* CategoryManagement::GetSelectedCategory()
    {
        if (Data.find(SelectedCatID) == Data.end()) return nullptr;
        return &Data[SelectedCatID];
    }

    void CategoryManagement::RenderContent()
    {
        const float LineHeight = ImGui::GetTextLineHeightWithSpacing();
        for (auto& [ID, Cat] : Data)
        {
            ImGui::PushID(int(ID));
            // TODO: move a few pixel down for this eye button? it's just not aligned correctly
            const char* VisibilityIcon = Cat.Visibility ? ICON_FA_EYE : ICON_FA_EYE_SLASH;
            if (ImGui::InvisibleButton("##vis_btn", ImGui::CalcTextSize(VisibilityIcon)))
            {
                Cat.Visibility = !Cat.Visibility;
            }
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::CalcTextSize(VisibilityIcon).x);
            ImGui::Text(VisibilityIcon);
            ImGui::SameLine();
            ImGui::ColorEdit3("##cat_color_picker", (float*)&Cat.Color, ImGuiColorEditFlags_NoInputs);
            ImGui::SameLine();
            if (ImGui::Selectable(Cat.DisplayName.c_str(), ID == SelectedCatID, 0, {120, LineHeight}))
            {
                SelectedCatID = ID;
            }
            ImGui::SameLine();
            ImGui::Text("(%d/%d)", Cat.UsedCountInThisFrame, Cat.TotalUsedCount);
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
            Data[UUID()] = FCategory(NewCatName);
            NewCatName[0] = 0;
            Save();
        }
    }

    void CategoryManagement::LoadCategoriesFromFile()
    {
        if (!Setting::Get().ProjectIsLoaded) return;
        Data.clear();
        YAML::Node Node = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Categories.yaml"));
        for (YAML::const_iterator it = Node.begin(); it != Node.end(); ++it)
        {
            Data[UUID(it->first.as<uint64_t>())] = FCategory(it->second.as<YAML::Node>());
        }
    }

    void CategoryManagement::Save()
    {
        YAML::Node Node = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Categories.yaml"));
        for (const auto& [ID, Cat] : Data)
        {
            Node[std::to_string(ID)] = Cat.Serialize();
        }
        YAML::Emitter Out;
        Out << Node;
        std::ofstream Fout(Setting::Get().ProjectPath + std::string("/Data/Categories.yaml"));
        Fout << Out.c_str();
    }

    // TODO: pass... need get access to Curent Edit Frame!
    void CategoryManagement::UpdateCategoryStatics()
    {
        for (auto& [ID, Cat] : Data)
        {
            /*Cat.TotalUsedCount = 100;
            Cat.UsedCountInThisFrame = 100;*/
        }
    }
}
