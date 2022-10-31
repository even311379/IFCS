#include "CategoryManagement.h"
#include "Setting.h"
#include "Style.h"

#include <fstream>

#include "imgui_internal.h"
#include "ImguiNotify/font_awesome_5.h"
#include "Implot/implot.h"
#include <yaml-cpp/yaml.h>


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

    std::vector<UUID> CategoryManagement::GetRegisterCIDs() const
    {
        std::vector<UUID> Out;
        for (auto& [k,v] : Data)
            Out.push_back(k);
        return Out;
    }

    void CategoryManagement::RenderContent()
    {
        const float LineHeight = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 ChildWindowSize = ImGui::GetContentRegionAvail();
        ChildWindowSize.y *= 0.6f;
        ImGuiWindowFlags Flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("CategoriesArea", ChildWindowSize, false, Flags);
        {
            for (auto& [ID, Cat] : Data)
            {
                ImGui::PushID(int(ID));
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
        }
        ImGui::EndChild();
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

        static ImS16 C0[] = {555,0,0,0};
        static ImS16 C1[] = {0,1234,0,0};
        static ImS16 C2[] = {0,0,321,0};
        static ImS16 C3[] = {0,0,0,128};
        static const char* glabels[] = {"G1", "G2", "G3", "G4"};
        static const double positions[] = {1,2,3,4};
        static ImVec4 Colors[] = {
            Style::RED(400), Style::RED(700), Style::CELERY(400), Style::CELERY(700)
        };
        static ImPlotColormap RC = ImPlot::AddColormap("RandomBarColor", Colors, 4); // Use category color!!!!
        ImGui::Checkbox("Show Bar?", &ShowBar);
        if (ShowBar)
        {
            ImPlot::PushColormap(RC);
            if (ImPlot::BeginPlot("Category Counts"))
            {
                ImPlot::SetupLegend(ImPlotLocation_NorthEast, 0);
                // plot indivisual for better viz?
                ImPlot::SetupAxes("Num of Annotations", "Category", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxisTicks(ImAxis_Y1, positions, 4, glabels);
                ImPlot::PlotBars(glabels[0], C0, 4, 0.5f, 1, ImPlotBarGroupsFlags_Horizontal);
                ImPlot::PlotBars(glabels[1], C1, 4, 0.5f, 1, ImPlotBarGroupsFlags_Horizontal);
                ImPlot::PlotBars(glabels[2], C2, 4, 0.5f, 1, ImPlotBarGroupsFlags_Horizontal);
                ImPlot::PlotBars(glabels[3], C3, 4, 0.5f, 1, ImPlotBarGroupsFlags_Horizontal);
                ImPlot::EndPlot();
            }
            ImPlot::PopColormap();
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
            if (it == Node.begin())
                SelectedCatID = UUID(it->first.as<uint64_t>());
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
