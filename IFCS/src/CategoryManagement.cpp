#include "CategoryManagement.h"
#include "Setting.h"
#include "Style.h"

#include <fstream>

#include "imgui_internal.h"
#include "ImguiNotify/font_awesome_5.h"
#include "Implot/implot.h"
#include <yaml-cpp/yaml.h>

#include "Annotation.h"
#include "TrainingSetGenerator.h"


namespace IFCS
{
    void CategoryManagement::Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose)
    {
        Panel::Setup(InName, InShouldOpen, InFlags, InCanClose);
        LoadCategoriesFromFile();
        TrainingSetGenerator::Get().SyncCategoyData(GetRegisteredCIDs());
        // make sure their are always at least one category...
        if (Data.empty())
        {
            const UUID NewUID;
            Data[NewUID] = FCategory("Default");
            SelectedCatID = NewUID;
        }
        UpdateCategoryStatics();
    }

    FCategory* CategoryManagement::GetSelectedCategory()
    {
        if (Data.find(SelectedCatID) == Data.end()) return nullptr;
        return &Data[SelectedCatID];
    }

    void CategoryManagement::AddCount()
    {
        if (SelectedCatID == 0) return;
        Data[SelectedCatID].TotalUsedCount += 1;
    }

    std::vector<UUID> CategoryManagement::GetRegisteredCIDs() const
    {
        std::vector<UUID> Out;
        for (auto& [k,v] : Data)
            Out.push_back(k);
        return Out;
    }

    void CategoryManagement::DrawSummaryBar()
    {
        std::vector<ImVec4> Colors;
        std::vector<std::vector<ImU16>> BarData;
        std::vector<const char*> CatNames;
        std::vector<double> Positions;
        const size_t NumBars = Data.size();
        size_t i = 0;
        for (const auto& [CID, Cat] : Data)
        {
            std::vector<ImU16> Temp(NumBars, 0);
            Temp[i] = Cat.TotalUsedCount;
            CatNames.emplace_back(Cat.DisplayName.c_str());
            BarData.emplace_back(Temp);
            Colors.emplace_back(Cat.Color);
            i++;
            Positions.push_back((double)i);
        }
        static ImPlotColormap RC = ImPlot::AddColormap("BarColors", Colors.data(), (int)NumBars);
        ImPlot::PushColormap(RC);
        if (ImPlot::BeginPlot("Category Counts", ImVec2(-1, 0),
                              ImPlotFlags_NoInputs | ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoFrame))
        {
            ImPlot::SetupAxes(0, 0, ImPlotAxisFlags_NoLabel, ImPlotAxisFlags_NoLabel);
            ImPlot::SetupAxisTicks(ImAxis_Y1, Positions.data(), (int)NumBars, CatNames.data());
            // TODO: is the order important? So hard to match so far...
            for (size_t j = 0; j < NumBars; j++)
                ImPlot::PlotBars(CatNames[j], BarData[j].data(), (int)NumBars, 0.5, 1, ImPlotBarGroupsFlags_Horizontal);
            ImPlot::EndPlot();
        }
        ImPlot::PopColormap();
    }

    void CategoryManagement::RenderContent()
    {
        const float LineHeight = ImGui::GetTextLineHeightWithSpacing();
        static float BottomGroupHeight;
        ImVec2 ChildWindowSize = ImGui::GetContentRegionAvail();
        ChildWindowSize.y -= BottomGroupHeight + 5;
        ImGuiWindowFlags Flags = ImGuiWindowFlags_HorizontalScrollbar;

        ImGui::BeginChild("CategoriesArea", ChildWindowSize, false, Flags);
        {
            for (auto& [ID, Cat] : Data)
            {
                ImGui::PushID(int(ID));
                const char* VisibilityIcon = Cat.Visibility ? ICON_FA_EYE : ICON_FA_EYE_SLASH;

                ImGui::PushStyleColor(ImGuiCol_Button, GImGui->Style.Colors[ImGuiCol_WindowBg]);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, GImGui->Style.Colors[ImGuiCol_WindowBg]);
                if (ImGui::Button(VisibilityIcon))
                {
                    Cat.Visibility = !Cat.Visibility;
                }
                ImGui::PopStyleColor(2);
                ImGui::SameLine();
                ImGui::ColorEdit3("##cat_color_picker", (float*)&Cat.Color, ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine();

                //TODO: replace it with SelectableInput()
                if (ImGui::Selectable(Cat.DisplayName.c_str(), ID == SelectedCatID,
                                      ImGuiSelectableFlags_AllowDoubleClick, {120, LineHeight}))
                {
                    SelectedCatID = ID;
                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        OpenChangeName = true;
                    }
                }
                ImGui::SameLine();
                ImGui::Text("(%d)", Cat.TotalUsedCount);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("(use annotation in: this frame / all project)");
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
        ImGui::BeginGroup();
        {
            ImGui::SetNextItemWidth(120);
            ImGui::InputText("##new cat name", NewCatName, IM_ARRAYSIZE(NewCatName));
            ImGui::SameLine();
            if (ImGui::Button("Add", ImVec2(65, 0)))
            {
                // TODO: check name duplicated...
                Data[UUID()] = FCategory(NewCatName);
                NewCatName[0] = 0;
                Save();
            }
            if (Data.size() > 1)
            {
                ImGui::SameLine();
                if (ImGui::Button("Delete", ImVec2(65, 0)))
                {
                    if (Data[SelectedCatID].TotalUsedCount == 0)
                    {
                        Data.erase(SelectedCatID);
                        SelectedCatID = Data.begin()->first;
                        Save();
                    }
                    else
                    {
                        ImGui::OpenPopup("Delete?");
                    }
                }
            }
            ImGui::Dummy(ImVec2(0, LineHeight));
            if (Data.size()>1)
            {
                ImGui::Separator();
                ImGui::Checkbox("Show Bar?", &ShowBar);
                if (ShowBar)
                {
                    DrawSummaryBar();
                }
            }
        }
        ImGui::EndGroup();
        BottomGroupHeight = ImGui::GetItemRectSize().y;

        if (OpenChangeName)
            ImGui::OpenPopup("Change display name?");

        if (ImGui::BeginPopup("Change display name?"))
        {
            static char Temp[64];
            if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
                ImGui::SetKeyboardFocusHere(0);

            ImGui::InputText("##New display name", Temp, IM_ARRAYSIZE(Temp));
            ImGui::Separator();
            if (ImGui::Button("Change?", ImVec2(120, 0)))
            {
                Data[SelectedCatID].DisplayName = Temp;
                Temp[0] = 0;
                OpenChangeName = false;
                Save();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                OpenChangeName = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // delete category modal
        if (ImGui::BeginPopupModal("Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Will also delete (change) %d annotations. \n  Are you sure?",
                        Data[SelectedCatID].TotalUsedCount);
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                // Can never "delete" element!! only rebuild it!!!
                YAML::Node Node = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/Annotations.yaml");
                YAML::Node NewNode;
                for (YAML::const_iterator it=Node.begin(); it!=Node.end();++it)
                {
                    auto Clip = it->first.as<std::string>();
                    auto CNode = it->second.as<YAML::Node>();
                    for (YAML::const_iterator f=CNode.begin();f!=CNode.end();++f)
                    {
                        auto Frame = f->first.as<std::string>();
                        auto FNode = f->second.as<YAML::Node>();
                        for (YAML::const_iterator ANN=FNode.begin();ANN!=FNode.end();++ANN)
                        {
                            if (ANN->second["CategoryID"].as<uint64_t>() != SelectedCatID)
                            {
                                NewNode[Clip][Frame][ANN->first] = ANN->second;
                            }
                        }
                    }
                }
                YAML::Emitter Out;
                Out << NewNode;
                std::ofstream ofs(Setting::Get().ProjectPath + "/Data/Annotations.yaml");
                ofs << Out.c_str();
                ofs.close();

                Data.erase(SelectedCatID);
                SelectedCatID = Data.begin()->first;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
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
        YAML::Node Node;
        for (const auto& [ID, Cat] : Data)
        {
            Node[std::to_string(ID)] = Cat.Serialize();
        }
        YAML::Emitter Out;
        Out << Node;
        std::ofstream Fout(Setting::Get().ProjectPath + std::string("/Data/Categories.yaml"));
        Fout << Out.c_str();
        TrainingSetGenerator::Get().SyncCategoyData(GetRegisteredCIDs());
    }

    void CategoryManagement::UpdateCategoryStatics()
    {
        for (auto [UID, Cat] : Data)
            Cat.TotalUsedCount = 0;
        YAML::Node ClipNode = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/Annotations.yaml");
        for (YAML::const_iterator it = ClipNode.begin(); it != ClipNode.end(); ++it)
        {
            auto FrameNode = it->second.as<YAML::Node>();
            for (YAML::const_iterator it1 = FrameNode.begin(); it1 != FrameNode.end(); ++it1)
            {
                uint64_t TT = it1->first.as<uint64_t>();
                auto ANode = it1->second.as<YAML::Node>();
                for (YAML::const_iterator it2 = ANode.begin(); it2 != ANode.end(); ++it2)
                {
                    uint64_t TTT = it2->first.as<uint64_t>();
                    uint64_t CID = it2->second.as<YAML::Node>()["CategoryID"].as<uint64_t>();
                    Data[CID].TotalUsedCount += 1;
                }
            }
        }
    }

    ImVec4 CategoryManagement::GetColorFrameDisplayName(const std::string& InName)
    {
        for (const auto& [UID, Cat] : Data)
        {
            if (Cat.DisplayName == InName) return Cat.Color;
        }
        return {};
    }
}
