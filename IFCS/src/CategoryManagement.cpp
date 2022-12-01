#include "CategoryManagement.h"
#include "Setting.h"
#include "Style.h"
#include <fstream>
#include "imgui_internal.h"
#include "IconFontCppHeaders/IconsFontAwesome5.h"
#include "Implot/implot.h"
#include <yaml-cpp/yaml.h>

// #include "TrainingSetGenerator.h"


namespace IFCS
{
    // TODO: count in images is missing...

    
    void CategoryManagement::Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose)
    {
        Panel::Setup(InName, InShouldOpen, InFlags, InCanClose);
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

    static int Tick = 0;
    static bool NeedSave = false;

    bool CategoryManagement::CheckValidNewCatName()
    {
        if (std::string(NewCatName).empty()) return true;
        for (auto& [ID, Cat] : Data)
        {
            if (std::string(NewCatName) == Cat.DisplayName)
                return true;
        }
        return false;
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
                if (ImGui::ColorEdit3("##cat_color_picker", (float*)&Cat.Color, ImGuiColorEditFlags_NoInputs))
                {
                    NeedSave = true;
                }
                ImGui::SameLine();

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
                    ImGui::SetTooltip("(number of annotation in this project)");
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
        ImGui::BeginGroup();
        {
            ImGui::SetNextItemWidth(120);
            ImGui::InputText("##new cat name", NewCatName, IM_ARRAYSIZE(NewCatName));
            ImGui::SameLine();
            ImGui::BeginDisabled(CheckValidNewCatName());
            if (ImGui::Button("Add", ImVec2(65, 0)))
            {
                const UUID NewID = UUID();
                Data[NewID] = FCategory(NewCatName);
                NewCatName[0] = 0;
                GeneratorCheckData[NewID] = true;
                NeedSave = true;
            }
            ImGui::EndDisabled();
            if (Data.size() > 1)
            {
                ImGui::SameLine();
                if (ImGui::Button("Delete", ImVec2(65, 0)))
                {
                    if (Data[SelectedCatID].TotalUsedCount == 0)
                    {
                        Data.erase(SelectedCatID);
                        GeneratorCheckData.erase(SelectedCatID);
                        SelectedCatID = Data.begin()->first;
                        NeedSave = true;
                    }
                    else
                    {
                        ImGui::OpenPopup("Delete?");
                    }
                }
            }
            ImGui::Dummy(ImVec2(0, LineHeight));
            if (Data.size() > 1)
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
                NeedSave = true;
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
                for (YAML::const_iterator it = Node.begin(); it != Node.end(); ++it)
                {
                    auto Clip = it->first.as<std::string>();
                    auto CNode = it->second.as<YAML::Node>();
                    for (YAML::const_iterator f = CNode.begin(); f != CNode.end(); ++f)
                    {
                        auto Frame = f->first.as<std::string>();
                        auto FNode = f->second.as<YAML::Node>();
                        for (YAML::const_iterator ANN = FNode.begin(); ANN != FNode.end(); ++ANN)
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

        if (Tick > 60)
        {
            if (NeedUpdate)
            {
                UpdateCategoryStatics();
                NeedUpdate = false;
            }
            if (NeedSave)
            {
                Save();
                NeedSave = false;
            }
            Tick = 0;
        }
        Tick++;
    }

    void CategoryManagement::LoadCategoriesFromFile()
    {
        Data.clear();
        YAML::Node Node = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Categories.yaml"));
        
        for (YAML::const_iterator it = Node.begin(); it != Node.end(); ++it)
        {
            Data[UUID(it->first.as<uint64_t>())] = FCategory(it->second.as<YAML::Node>());
            if (it == Node.begin())
                SelectedCatID = UUID(it->first.as<uint64_t>());
        }
        GeneratorCheckData.clear();
        for (const auto& [ID, Cat] : Data)
        {
            GeneratorCheckData[ID] = true;
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
    }

    void CategoryManagement::UpdateCategoryStatics()
    {
        LoadCategoriesFromFile();
        if (Data.empty())
        {
            const UUID NewUID;
            Data[NewUID] = FCategory("Default");
            SelectedCatID = NewUID;
            NeedSave = true;
        }
        for (auto [UID, Cat] : Data)
            Cat.TotalUsedCount = 0;
        YAML::Node AllNodes = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/Annotations.yaml");
        for (YAML::const_iterator File = AllNodes.begin(); File != AllNodes.end(); ++File)
        {
            // if (File->second.IsMap())
            // {
                const YAML::Node& Frames = File->second;
                for (YAML::const_iterator Fit = Frames.begin(); Fit != Frames.end(); ++Fit)
                {
                    const YAML::Node& Frame = Fit->second;
                    for (YAML::const_iterator i = Frame.begin(); i != Frame.end(); ++i)
                    {
                        if (i->first.as<std::string>() == "IsReady" || i->first.as<std::string>() == "UpdateTime")
                            continue;
                        Data[i->second["CategoryID"].as<uint64_t>()].TotalUsedCount += 1;
                    }
                }
            // }
            // else
            // {
            //     for (YAML::const_iterator i = File->second[0].begin(); i != File->second[0].end(); ++i)
            //     {
            //         if (i->first.as<std::string>() == "IsReady" || i->first.as<std::string>() == "UpdateTime")
            //             continue;
            //         Data[i->second["CategoryID"].as<uint64_t>()].TotalUsedCount += 1;
            //     }
            // }
        }
    }
}
