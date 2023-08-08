#include "Train.h"
#include "Setting.h"

#include <fstream>
#include <future>
#include <chrono>
#include <numeric>
#include <random>
#include <thread>
#include <string>
#include <regex>

#include "Annotation.h"
#include "CategoryManagement.h"
#include "DataBrowser.h"
#include "yaml-cpp/yaml.h"
#include "IconFontCppHeaders/IconsFontAwesome5.h"
#include "misc/cpp/imgui_stdlib.h"
#include "Modals.h"
#include "Style.h"

#include "shellapi.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"
#include "Implot/implot.h"
#include "Imspinner/imspinner.h"

// TODO: model comparison!!!

namespace IFCS
{
    static const char* ModelOptions[] = {
        "yolov7", "yolov7-d6", "yolov7-e6", "yolov7-e6e", "yolov7-tiny", "yolov7-w6", "yolov7x"
    };

    std::vector<int> Model6Indices = {1, 2, 3, 5};
    std::vector<std::string> Model6Names = {"yolov7-d6", "yolov7-e6", "yolov7-e6e", "yolov7-w6"};

    void Train::Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose)
    {
        Panel::Setup(InName, InShouldOpen, InFlags, InCanClose);
        Devices[0] = true;
        UpdateTrainScript();
    }

    bool IsGeneratingSet = false;
    char NewTrainingSetName[64];
    std::string ErrorMessage;
    std::string SuccessMessage;

    void Train::RenderContent()
    {
        if (ImGui::BeginTabBar("TrainTabs"))
        {
            if (ImGui::BeginTabItem("Training Set"))
            {
                if (IsGeneratingSet)
                {
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Style::BLUE(400, Setting::Get().Theme));
                    ImGui::ProgressBar(float(CurrentImgGenerated) / TotalExportImages, {256.f, 0});
                    ImGui::PopStyleColor();
                    ImGui::SameLine(0, 32);
                    ImSpinner::SpinnerBarsScaleMiddle("Spinner_GeneratingSet", 6,
                                                      ImColor(Style::BLUE(400, Setting::Get().Theme)));
                    ImGui::Text("Generating training set...");
                    if (GenerateSet_Future.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
                    {
                        ErrorMessage.clear();
                        SuccessMessage.clear();
                        IsGeneratingSet = false;
                        if (CurrentImgGenerated == TotalExportImages)
                        {
                            SuccessMessage = std::string(NewTrainingSetName) + " was successfully generated!";
                        }
                        else
                        {
                            ErrorMessage = std::string("Something was wrong... only ") + std::to_string(CurrentImgGenerated) + "/" +
                                std::to_string(TotalExportImages) + " was generated!";
                        }
                        NewTrainingSetName[0] = '\0';
                    }
                }
                else
                {
                    ImGui::PushFont(Setting::Get().TitleFont);
                    ImGui::Text("Training Set Generator");
                    ImGui::PopFont();
                    ImGui::Indent();
                    if (ImGui::TreeNode("Data Select"))
                    {
                        RenderDataSelectWidget();
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Category To Export"))
                    {
                        RenderCategoryWidget();
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Train / Valid / (Test) Split"))
                    {
                        RenderSplitWidget();
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Resize"))
                    {
                        RenderResizeWidget();
                        ImGui::TreePop();
                    }
                    ImGui::Unindent();
                    ImGui::PushFont(Setting::Get().TitleFont);
                    ImGui::Text("Summary");
                    ImGui::PopFont();
                    ImGui::Indent();
                    RenderSummary();
                    ImGui::Unindent();
                    ImGui::PushFont(Setting::Get().TitleFont);
                    ImGui::Text("Export");
                    ImGui::PopFont();
                    ImGui::Indent();
                    RenderTrainingSetExportWidget();

                    ImGui::Unindent();
                    ImGui::TextColored(Style::RED(), ErrorMessage.c_str());
                    ImGui::TextColored(Style::BLUE(), SuccessMessage.c_str());
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Training"))
            {
                if (!Setting::Get().IsEnvSet())
                {
                    ImGui::Text("Environment not setup yet!");
                    if (ImGui::Button("Open setting to set it now?"))
                    {
                        Modals::Get().IsModalOpen_Setting = true;
                    }
                    ImGui::EndTabItem();
                    ImGui::EndTabBar();
                    return;
                }
                if (IsTraining)
                {
                    ImSpinner::SpinnerBarsScaleMiddle("Spinner_Training", 6,
                                                      ImColor(Style::BLUE(400, Setting::Get().Theme)));
                    

                    ImGui::Text("Training...");
                    if (TrainingFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
                    {
                        OnTrainingFinished();
                    }
                }
                else
                {
                    RenderTrainingOptions();
                    RenderTrainingScript();
                }
                ImGui::Text("Monitor progress with tensorboard:");
                if (ImGui::Button("Host & Open tensorboard", ImVec2(240, 0)))
                {
                    OpenTensorBoard();
                }
                if (!TrainLog.empty())
                {
                    ImGui::Text("Training Log:");
                    ImGui::BeginChildFrame(ImGui::GetID("TrainLog"), ImVec2(0, ImGui::GetTextLineHeight() * 3));
                    ImGui::TextWrapped(TrainLog.c_str());
                    ImGui::EndChildFrame();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    void Train::RenderDataSelectWidget()
    {
        if (ImGui::Checkbox("Ready only?", &IncludeReadyOnly))
        {
            UpdateGenerartorInfo();
        }
        ImGui::Text("Select which folders / clips to include in this training set:");
        const ImVec2 FrameSize = ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 9);
        const size_t RelativeClipNameOffset = Setting::Get().ProjectPath.size() + 7;
        const size_t RelativeImageFolderNameOffset = Setting::Get().ProjectPath.size() + 8;
        ImGui::BeginChildFrame(ImGui::GetID("Included Contents"), FrameSize, ImGuiWindowFlags_NoMove);
        for (const std::string& Clip : IncludedClips)
        {
            std::string s = Clip.substr(RelativeClipNameOffset);
            ImGui::Text(s.c_str());
        }
        for (const std::string& ImageFolder : IncludedImageFolders)
        {
            std::string s = ImageFolder.substr(RelativeClipNameOffset);
            ImGui::Text("%s (Image folder)", s.c_str());
        }
        ImGui::EndChildFrame();
        if (ImGui::BeginDragDropTarget()) // for the previous item? 
        {
            if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("Clip"))
            {
                std::string ClipToInclude = std::string((const char*)Payload->Data);
                if (!Utils::Contains(IncludedClips, ClipToInclude))
                {
                    IncludedClips.push_back(ClipToInclude);
                    UpdateGenerartorInfo();
                }
            }
            if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("ImageFolder"))
            {
                std::string ImageFolderToInclude = std::string((const char*)Payload->Data);
                if (!Utils::Contains(IncludedImageFolders, ImageFolderToInclude))
                {
                    IncludedImageFolders.push_back(ImageFolderToInclude);
                    UpdateGenerartorInfo();
                }
            }
        }
        char AddClipPreviewTitle[128];
        sprintf(AddClipPreviewTitle, "%s Add Clip", ICON_FA_PLUS);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::BeginCombo("##AddClip", AddClipPreviewTitle))
        {
            std::vector<std::string> AvailClips = DataBrowser::Get().GetAllClips();
            auto NewEnd = std::remove_if(AvailClips.begin(), AvailClips.end(), [this](const std::string& C)
            {
                return std::find(IncludedClips.begin(), IncludedClips.end(), C) != IncludedClips.end();
            });
            AvailClips.erase(NewEnd, AvailClips.end());
            for (auto& AvailClip : AvailClips)
            {
                if (ImGui::Selectable(AvailClip.substr(RelativeClipNameOffset).c_str(), false))
                {
                    IncludedClips.push_back(AvailClip);
                    UpdateGenerartorInfo();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        char AddFolderPreviewTitle[128];
        sprintf(AddFolderPreviewTitle, "%s Add Folder", ICON_FA_PLUS);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::BeginCombo("##AddFolder", AddFolderPreviewTitle))
        {
            std::vector<std::string> AvailImageFolders = DataBrowser::Get().GetImageFolders();
            auto NewEnd =
                std::remove_if(AvailImageFolders.begin(), AvailImageFolders.end(), [this](const std::string& C)
                {
                    return std::find(IncludedImageFolders.begin(), IncludedImageFolders.end(), C) !=
                        IncludedImageFolders.end();
                });
            AvailImageFolders.erase(NewEnd, AvailImageFolders.end());
            for (auto& AvailImageFolder : AvailImageFolders)
            {
                std::string Label = AvailImageFolder.size() == RelativeImageFolderNameOffset
                                        ? "/"
                                        : AvailImageFolder.substr(RelativeImageFolderNameOffset);
                if (ImGui::Selectable(Label.c_str(), false))
                {
                    IncludedImageFolders.push_back(AvailImageFolder);
                    UpdateGenerartorInfo();
                }
            }
            ImGui::EndCombo();
        }
        static float HalfWidth = ImGui::GetContentRegionAvail().x * 0.5f;
        if (ImGui::Button("Add All", ImVec2(HalfWidth, ImGui::GetFontSize() * 1.5f)))
        {
            IncludedClips = DataBrowser::Get().GetAllClips();
            IncludedImageFolders = DataBrowser::Get().GetImageFolders();
            UpdateGenerartorInfo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear All", ImVec2(HalfWidth, ImGui::GetFontSize() * 1.5f)))
        {
            IncludedClips.clear();
            IncludedImageFolders.clear();
            UpdateGenerartorInfo();
        }
        ImGui::BulletText("Included Clips: %d", (int)IncludedClips.size());
        ImGui::SameLine(HalfWidth);
        ImGui::BulletText("Included frames: %d", NumIncludedFrames);
        ImGui::BulletText("Included Images: %d", NumIncludedImages);
        ImGui::SameLine(HalfWidth);
        ImGui::BulletText("Included Annotations: %d", NumIncludedAnnotations);
    }

    void Train::RenderCategoryWidget()
    {
        if (IncludedClips.size() + IncludedImageFolders.size() == 0)
        {
            ImGui::Text("No training clip / image folder to export! Add some in [Data Select]");
            return;
        }
        auto CatData = CategoryManagement::Get().Data;
        static char MergedName[64];
        static std::vector<UUID> UsedCatId;
        static std::vector<UUID> SelectedCatIDs;
        if (ImGui::Checkbox("Need to merge category to export?", &ShouldMergeCategories))
        {
            CategoryMergeData.clear();
            UsedCatId.clear();
            SelectedCatIDs.clear();
            for (auto& [UID, Should] : CategoryManagement::Get().GeneratorCheckData)
            {
                Should = false;
            }
            MergedName[0] = '\0';
            UpdateGenerartorInfo();
        }
        if (ShouldMergeCategories)
        {
            ImGui::BeginChild("ChildToMerge", ImVec2(300, 400), true);
            ImGui::BulletText("Select what to merge:");
            ImGui::Separator();
            for (auto& [UID, Should] : CategoryManagement::Get().GeneratorCheckData)
            {
                if (Utils::Contains(UsedCatId, UID)) continue;
                const bool HasSelected = Utils::Contains(SelectedCatIDs, UID);
                if (ImGui::Selectable(CatData[UID].DisplayName.c_str(), HasSelected))
                {
                    if (HasSelected)
                    {
                        SelectedCatIDs.erase(std::remove(SelectedCatIDs.begin(), SelectedCatIDs.end(), UID),
                                             SelectedCatIDs.end());
                    }
                    else
                    {
                        SelectedCatIDs.push_back(UID);
                    }
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild("ChildMergeBtns", ImVec2(400, 400), false);
            ImGui::Dummy({0, 200});
            ImGui::BeginDisabled(SelectedCatIDs.empty());
            if (ImGui::Button("Add >>"))
            {
                for (const UUID UID : SelectedCatIDs)
                {
                    FCategoryMergeData NewData;
                    NewData.DisplayName = CatData[UID].DisplayName;
                    NewData.SourceCatIdx.push_back(UID);
                    UsedCatId.push_back(UID);
                    CategoryManagement::Get().GeneratorCheckData[UID] = true;
                    CategoryMergeData.push_back(NewData);
                }
                UpdateGenerartorInfo();
                SelectedCatIDs.clear();
            }
            ImGui::EndDisabled();
            ImGui::InputText("Merged name", MergedName, IM_ARRAYSIZE(MergedName));
            bool CanMerge = false;
            if (MergedName[0] != '\0' && SelectedCatIDs.size() > 1)
            {
                CanMerge = true;
                for (const auto& Data : CategoryMergeData)
                {
                    if (Data.DisplayName == std::string(MergedName))
                    {
                        CanMerge = false;
                        break;
                    }
                }
            }
            ImGui::BeginDisabled(!CanMerge);
            if (ImGui::Button("Merge >>"))
            {
                FCategoryMergeData NewData;
                NewData.DisplayName = MergedName;
                for (const UUID UID : SelectedCatIDs)
                {
                    NewData.SourceCatIdx.push_back(UID);
                    UsedCatId.push_back(UID);
                    CategoryManagement::Get().GeneratorCheckData[UID] = true;
                }
                CategoryMergeData.push_back(NewData);
                UpdateGenerartorInfo();
                SelectedCatIDs.clear();
                MergedName[0] = '\0';
            }
            ImGui::EndDisabled();
            ImGui::Separator();
            if (ImGui::Button("<< Reset"))
            {
                CategoryMergeData.clear();
                UsedCatId.clear();
                SelectedCatIDs.clear();
                for (auto& [UID, Should] : CategoryManagement::Get().GeneratorCheckData)
                {
                    Should = false;
                }
                MergedName[0] = '\0';
                UpdateGenerartorInfo();
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild("ChildMerged", ImVec2(300, 400), true);
            ImGui::BulletText("Merged result:");
            ImGui::Separator();
            for (const auto& Data : CategoryMergeData)
            {
                ImGui::TextColored(Style::RED(400, Setting::Get().Theme), Data.DisplayName.c_str());
                if (Data.SourceCatIdx.size() == 1) continue;
                ImGui::Indent(75);
                for (const UUID& ID : Data.SourceCatIdx)
                {
                    ImGui::Text(CatData[ID].DisplayName.c_str());
                }
                ImGui::Unindent(75);
            }
            ImGui::EndChild();
            // draw export histogram
            int TotalCount = 0;
            for (auto [s, i] : CategoriesExportCounts)
            {
                TotalCount += i;
            }
            const size_t NumBars = CategoryMergeData.size();
            if (TotalCount > 0 && NumBars > 1)
            {
                std::vector<std::vector<ImU16>> BarData;
                std::vector<const char*> CatNames;
                std::vector<double> Positions;
                size_t i = 0;
                for (const auto& [CatName, N] : CategoriesExportCounts_Merged)
                {
                    std::vector<ImU16> Temp(NumBars, 0);
                    Temp[i] = (ImU16)N;
                    CatNames.emplace_back(CatName.c_str());
                    BarData.emplace_back(Temp);
                    i++;
                    Positions.push_back((double)i);
                }
                if (ImPlot::BeginPlot("Category Counts", ImVec2(-1, 0), ImPlotFlags_NoInputs |
                                      ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoFrame))
                {
                    ImPlot::SetupAxes(0, 0, ImPlotAxisFlags_NoLabel, ImPlotAxisFlags_NoLabel);
                    ImPlot::SetupAxisTicks(ImAxis_Y1, Positions.data(), (int)NumBars, CatNames.data());
                    for (size_t j = 0; j < NumBars; j++)
                        ImPlot::PlotBars(CatNames[j], BarData[j].data(), (int)NumBars, 0.5, 1,
                                         ImPlotBarGroupsFlags_Horizontal);
                    ImPlot::EndPlot();
                }
                ImGui::TextWrapped("HINT! Imbalanced data may impair model! You should prevent it somehow...");
            }
        }
        else
        {
            for (auto& [UID, Should] : CategoryManagement::Get().GeneratorCheckData)
            {
                if (ImGui::Checkbox(CatData[UID].DisplayName.c_str(), &Should))
                {
                    UpdateGenerartorInfo();
                }
            }

            if (ImGui::Button("Add all catogories"))
            {
                for (auto& [UID, Should] : CategoryManagement::Get().GeneratorCheckData)
                {
                    Should = true;
                }
                UpdateGenerartorInfo();
            }
            // draw export histogram
            int TotalCount = 0;
            for (auto [s, i] : CategoriesExportCounts)
            {
                TotalCount += i;
            }
            const size_t NumBars = CategoriesExportCounts.size();

            // TODO: bar size is wrong when only 2 bar displayed...  why??
            // for (const auto [K, V] : CategoriesExportCounts)
            // {
            //     spdlog::info("ID {} : {}", K, V);
            // }
            if (TotalCount > 0 && NumBars > 1)
            {
                std::vector<ImVec4> Colors;
                std::vector<std::vector<ImU16>> BarData;
                std::vector<const char*> CatNames;
                std::vector<double> Positions;
                size_t i = 0;
                for (const auto& [CID, N] : CategoriesExportCounts)
                {
                    std::vector<ImU16> Temp(NumBars, 0);
                    Temp[i] = (ImU16)N;
                    CatNames.emplace_back(CatData[CID].DisplayName.c_str());
                    BarData.emplace_back(Temp);
                    Colors.emplace_back(CatData[CID].Color);
                    i++;
                    Positions.push_back((double)i);
                }
                static ImPlotColormap RC = ImPlot::AddColormap("ExpBarColors", Colors.data(), (int)NumBars);
                ImPlot::PushColormap(RC);
                if (ImPlot::BeginPlot("Category Counts", ImVec2(-1, 0), ImPlotFlags_NoInputs |
                                      ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoFrame))
                {
                    ImPlot::SetupAxes(0, 0, ImPlotAxisFlags_NoLabel, ImPlotAxisFlags_NoLabel);
                    ImPlot::SetupAxisTicks(ImAxis_Y1, Positions.data(), (int)NumBars, CatNames.data());
                    for (size_t j = 0; j < NumBars; j++)
                        ImPlot::PlotBars(CatNames[j], BarData[j].data(), (int)NumBars, 0.5, 1,
                                         ImPlotBarGroupsFlags_Horizontal);
                    ImPlot::EndPlot();
                }
                ImPlot::PopColormap();
                ImGui::TextWrapped("HINT! Imbalanced data may impair model! You should prevent it somehow...");
            }
        }
    }


    float SplitPercent[3] = {70, 30, 0};
    int SplitImgs[3];
    bool IncludeTestSet = false;

    void Train::RenderSplitWidget()
    {
        if (TotalExportImages == 0)
        {
            ImGui::Text("No training image to export! Add some in [Data Select] or [Category To Export]");
            return;
        }
        static float SplitControlPos1 = 0.7f;
        static float SplitControlPos2 = 1.f;
        if (ImGui::Checkbox("Include test set?", &IncludeTestSet))
        {
            if (!IncludeTestSet)
            {
                SplitPercent[2] = 0;
                SplitPercent[0] = 100 - SplitPercent[1];
                SplitImgs[2] = 0;
                SplitImgs[0] = TotalExportImages - SplitImgs[1];
                SplitControlPos2 = 1.f;
                SplitControlPos1 = SplitPercent[0] / 100.f;
            }
        }

        if (IncludeTestSet) // in most case, two sets are enough!
        {
            ImVec2 CurrentPos = ImGui::GetCursorScreenPos();
            ImU32 Color = ImGui::ColorConvertFloat4ToU32(Style::BLUE(600, Setting::Get().Theme));
            const float AvailWidth = ImGui::GetContentRegionAvail().x * 0.95f;
            ImVec2 RectStart = CurrentPos + ImVec2(0, 17);
            ImVec2 RectEnd = CurrentPos + ImVec2(AvailWidth, 22);
            ImGui::GetWindowDrawList()->AddRectFilled(RectStart, RectEnd, Color, 20.f);
            const ImVec2 ControlPos1 = CurrentPos + ImVec2(SplitControlPos1 * AvailWidth - 6.f, 5);
            ImGui::GetWindowDrawList()->AddRectFilled(ControlPos1, ControlPos1 + ImVec2(12.f, 25), Color, 6.f);
            ImGui::SetCursorScreenPos(ControlPos1);
            ImGui::InvisibleButton("TrainControlMin", ImVec2(12.f, 20));
            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsMouseDragging(0) && ImGui::IsItemActive())
            {
                SplitControlPos1 += ImGui::GetIO().MouseDelta.x / AvailWidth;
                SplitControlPos1 = Utils::Round(SplitControlPos1, 2);
                if (SplitControlPos1 <= 0) SplitControlPos1 = 0;
                if (SplitControlPos1 >= 1) SplitControlPos1 = 1;
                if (SplitControlPos1 > SplitControlPos2)
                {
                    const float temp = SplitControlPos2;
                    SplitControlPos2 = SplitControlPos1;
                    SplitControlPos1 = temp;
                }
                SplitPercent[0] = SplitControlPos1 * 100.f;
                SplitPercent[1] = (SplitControlPos2 - SplitControlPos1) * 100.f;
                SplitPercent[2] = (1 - SplitControlPos2) * 100.f;
                SplitImgs[0] = int(SplitPercent[0] * 0.01f * TotalExportImages);
                SplitImgs[1] = int(SplitPercent[1] * 0.01f * TotalExportImages);
                SplitImgs[2] = TotalExportImages - SplitImgs[0] - SplitImgs[1];
            }
            const ImVec2 ControlPos2 = CurrentPos + ImVec2(SplitControlPos2 * AvailWidth - 6.f, 5);
            ImGui::GetWindowDrawList()->AddRectFilled(ControlPos2, ControlPos2 + ImVec2(12.f, 25), Color, 6.f);
            ImGui::SetCursorScreenPos(ControlPos2);
            ImGui::InvisibleButton("TrainControlMax", ImVec2(12.f, 20));
            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsMouseDragging(0) && ImGui::IsItemActive())
            {
                SplitControlPos2 += ImGui::GetIO().MouseDelta.x / AvailWidth;
                SplitControlPos2 = Utils::Round(SplitControlPos2, 2);
                if (SplitControlPos2 <= 0) SplitControlPos2 = 0;
                if (SplitControlPos2 >= 1) SplitControlPos2 = 1;
                if (SplitControlPos1 > SplitControlPos2)
                {
                    const float temp = SplitControlPos2;
                    SplitControlPos2 = SplitControlPos1;
                    SplitControlPos1 = temp;
                }
                SplitPercent[0] = SplitControlPos1 * 100.f;
                SplitPercent[1] = (SplitControlPos2 - SplitControlPos1) * 100.f;
                SplitPercent[2] = (1 - SplitControlPos2) * 100.f;
                SplitImgs[0] = int(SplitPercent[0] * 0.01f * TotalExportImages);
                SplitImgs[1] = int(SplitPercent[1] * 0.01f * TotalExportImages);
                SplitImgs[2] = TotalExportImages - SplitImgs[0] - SplitImgs[1];
            }
            // Add text in middle of the control
            float TxtWidth1 = ImGui::CalcTextSize("Train").x;
            float TxtWidth2 = ImGui::CalcTextSize("Valid").x;
            float TxtWidth3 = ImGui::CalcTextSize("Test").x;
            const float TxtPosY = 35.f;
            ImVec2 TxtPos1 = CurrentPos + ImVec2((SplitControlPos1 * AvailWidth - TxtWidth1) * 0.5f, TxtPosY);
            ImVec2 TxtPos2 = CurrentPos + ImVec2(
                (SplitControlPos1 + (SplitControlPos2 - SplitControlPos1) * 0.5f) * AvailWidth - TxtWidth2 * 0.5f,
                TxtPosY);
            ImVec2 TxtPos3 = CurrentPos + ImVec2(
                (SplitControlPos2 + (1.f - SplitControlPos2) * 0.5f) * AvailWidth - TxtWidth3 * 0.5f,
                TxtPosY);
            ImGui::GetWindowDrawList()->AddText(TxtPos1, Color, "Train");
            ImGui::GetWindowDrawList()->AddText(TxtPos2, Color, "Valid");
            ImGui::GetWindowDrawList()->AddText(TxtPos3, Color, "Test");
            ImGui::Dummy({0, 50});

            // draw the percent
            const float CachedSplitPercent[3] = {SplitPercent[0], SplitPercent[1], SplitPercent[2]};
            if (ImGui::InputFloat3("Train/Valid/Test (%)", SplitPercent, "%.0f%%"))
            {
                // check which one changed
                size_t ChangedID = 0;
                for (size_t i = 0; i < 3; i++)
                {
                    if (!Utils::FloatCompare(CachedSplitPercent[i], SplitPercent[i]))
                    {
                        ChangedID = i;
                    }
                }
                // clamp
                if (SplitPercent[ChangedID] > 100.f) SplitPercent[ChangedID] = 100.f;
                if (SplitPercent[ChangedID] < 0.f) SplitPercent[ChangedID] = 0.f;
                // modify other
                // still tries to remain train > test > valid && and mimic the movement widget as possible
                const float ChangedAmount = SplitPercent[ChangedID] - CachedSplitPercent[ChangedID];
                if (ChangedID == 0)
                {
                    if (ChangedAmount > SplitPercent[1])
                    {
                        SplitPercent[2] -= ChangedAmount - SplitPercent[1];
                        SplitPercent[1] = 0.f;
                    }
                    else
                    {
                        SplitPercent[1] -= ChangedAmount;
                    }
                }
                else if (ChangedID == 1)
                {
                    if (ChangedAmount > SplitPercent[2])
                    {
                        SplitPercent[1] -= ChangedAmount - SplitPercent[2];
                        SplitPercent[2] = 0.f;
                    }
                    else
                    {
                        SplitPercent[2] -= ChangedAmount;
                    }
                }
                else if (ChangedID == 2)
                {
                    if (ChangedAmount > SplitPercent[1])
                    {
                        SplitPercent[0] -= ChangedAmount - SplitPercent[1];
                        SplitPercent[1] = 0.f;
                    }
                    else
                    {
                        SplitPercent[1] -= ChangedAmount;
                    }
                }

                // propagate to images and widget
                SplitImgs[0] = static_cast<int>(std::nearbyint(
                    SplitPercent[0] * static_cast<float>(TotalExportImages) / 100.f));
                SplitImgs[1] = static_cast<int>(std::nearbyint(
                    SplitPercent[1] * static_cast<float>(TotalExportImages) / 100.f));
                SplitImgs[2] = static_cast<int>(std::nearbyint(
                    SplitPercent[2] * static_cast<float>(TotalExportImages) / 100.f));
                SplitControlPos1 = SplitPercent[0] * 0.01f;
                SplitControlPos2 = (SplitPercent[0] + SplitPercent[1]) * 0.01f;
            }
            int CachedSplitImgs[3] = {SplitImgs[0], SplitImgs[1], SplitImgs[2]};
            if (ImGui::InputInt3("Train/Valid/Test (Images)", SplitImgs))
            {
                // check which one changed
                size_t ChangedID = 0;
                for (size_t i = 0; i < 3; i++)
                {
                    if (CachedSplitImgs[i] != SplitImgs[i])
                    {
                        ChangedID = i;
                    }
                }
                // clamp
                if (SplitImgs[ChangedID] > TotalExportImages) SplitImgs[ChangedID] = TotalExportImages;
                if (SplitImgs[ChangedID] < 0) SplitImgs[ChangedID] = 0;
                // modify other
                // still tries to remain train > test > valid && and mimic the movement widget as possible
                const int ChangedAmount = SplitImgs[ChangedID] - CachedSplitImgs[ChangedID];
                if (ChangedID == 0)
                {
                    if (ChangedAmount > SplitImgs[1])
                    {
                        SplitImgs[2] -= ChangedAmount - SplitImgs[1];
                        SplitImgs[1] = 0;
                    }
                    else
                    {
                        SplitImgs[1] -= ChangedAmount;
                    }
                }
                else if (ChangedID == 1)
                {
                    if (ChangedAmount > SplitImgs[2])
                    {
                        SplitImgs[1] -= ChangedAmount - SplitImgs[2];
                        SplitImgs[2] = 0;
                    }
                    else
                    {
                        SplitImgs[2] -= ChangedAmount;
                    }
                }
                else if (ChangedID == 2)
                {
                    if (ChangedAmount > SplitImgs[1])
                    {
                        SplitImgs[0] -= ChangedAmount - SplitImgs[1];
                        SplitImgs[1] = 0;
                    }
                    else
                    {
                        SplitImgs[1] -= ChangedAmount;
                    }
                }

                // propagate to images and widget
                SplitPercent[0] = Utils::Round((float)SplitImgs[0] / (float)TotalExportImages, 2) * 100.f;
                SplitPercent[1] = Utils::Round((float)SplitImgs[1] / (float)TotalExportImages, 2) * 100.f;
                SplitPercent[2] = Utils::Round((float)SplitImgs[2] / (float)TotalExportImages, 2) * 100.f;
                SplitControlPos1 = SplitPercent[0] * 0.01f;
                SplitControlPos2 = (SplitPercent[0] + SplitPercent[1]) * 0.01f;
            }
        }
        else
        {
            if (ImGui::SliderFloat("Train / Valid split", &SplitControlPos1, 0.f, 1.f))
            {
                SplitImgs[2] = 0;
                SplitImgs[0] = int(TotalExportImages * SplitControlPos1);
                SplitImgs[1] = TotalExportImages - SplitImgs[0];
                SplitPercent[0] = SplitControlPos1 * 100;
                SplitPercent[1] = 100 - SplitPercent[0];
            }
            int CachedSplitImgs[2] = {SplitImgs[0], SplitImgs[1]};
            if (ImGui::InputInt2("Train / Valid (Images)", SplitImgs))
            {
                if (SplitImgs[0] != CachedSplitImgs[0])
                {
                    SplitImgs[1] = TotalExportImages - SplitImgs[0];
                }
                else
                {
                    SplitImgs[0] = TotalExportImages - SplitImgs[1];
                }
                SplitControlPos1 = (float)SplitImgs[0] / (float)TotalExportImages;
                SplitPercent[0] = SplitControlPos1 * 100;
                SplitPercent[1] = 100 - SplitPercent[0];
            }
        }
    }

    void Train::RenderResizeWidget()
    {
        static const char* AspectRatioOptions[] = {"16x9", "4x3", "Custom"};
        if (ImGui::Combo("Aspect Ratio", &SelectedResizeAspectRatio, AspectRatioOptions,
                         IM_ARRAYSIZE(AspectRatioOptions)))
        {
            switch (SelectedResizeAspectRatio)
            {
            case 0: // 16x9
                ExportedImageSize[1] = ExportedImageSize[0] * 9 / 16;
                break;
            case 1: // 4x3
                ExportedImageSize[1] = ExportedImageSize[0] * 3 / 4;
                break;
            default: break;
            }
        }
        int OldSize[2] = {ExportedImageSize[0], ExportedImageSize[1]};
        if (ImGui::InputInt2("New Size", ExportedImageSize))
        {
            const bool OtherIdx = OldSize[0] != ExportedImageSize[0];
            const bool ChangedIdx = !OtherIdx;
            switch (SelectedResizeAspectRatio)
            {
            case 0: // 16x9
                ExportedImageSize[OtherIdx] = ChangedIdx
                                                  ? ExportedImageSize[ChangedIdx] * 16 / 9
                                                  : ExportedImageSize[ChangedIdx] * 9 / 16;
                break;
            case 1: // 4x3
                ExportedImageSize[OtherIdx] = ChangedIdx
                                                  ? ExportedImageSize[ChangedIdx] * 4 / 3
                                                  : ExportedImageSize[ChangedIdx] * 3 / 4;
                break;
            default: break;
            }
        }
    }

    void Train::RenderSummary()
    {
        const ImVec2 FrameSize = ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 7);
        ImGui::BeginChildFrame(ImGui::GetID("ExportSummary"), FrameSize, ImGuiWindowFlags_NoMove);
        ImGui::BulletText("Total images to export: %d", TotalExportImages);
        ImGui::BulletText("Train / Valid / Test split: %.0f : %.0f : %.0f", SplitPercent[0], SplitPercent[1],
                          SplitPercent[2]);
        ImGui::BulletText("Resize to: %d x %d", ExportedImageSize[0], ExportedImageSize[1]);
        ImGui::BulletText("Category: %s", CatExportSummary.c_str());
        ImGui::Separator();
        ImGui::Text("Total Generation: %d  =  %d (train) + %d (valid) + %d (test)",
                    SplitImgs[0] + SplitImgs[1] + SplitImgs[2], SplitImgs[0], SplitImgs[1], SplitImgs[2]);
        ImGui::EndChildFrame();
    }


    void Train::RenderTrainingSetExportWidget()
    {
        ImGui::SetNextItemWidth(240.f);
        ImGui::InputText("Training Set Name", NewTrainingSetName, IM_ARRAYSIZE(NewTrainingSetName));
        ImGui::SameLine();
        if (ImGui::Button("Generate", ImVec2(240, 0)))
        {
            if (std::strlen(NewTrainingSetName) == 0) return;
            if (NumIncludedAnnotations == 0) return;
            for (auto Node : YAML::LoadFile(Setting::Get().ProjectPath + "/Data/TrainingSets.yaml"))
            {
                if (std::string(NewTrainingSetName) == Node.first.as<std::string>()) return;
            }
            CurrentImgGenerated = 0;
            IsGeneratingSet = true;
            
            // TODO: multi thread here??
            auto func = [this]()
            {
                GenerateTrainingSet();
            };
            GenerateSet_Future = std::async(std::launch::async, func);
        }
    }

    void Train::UpdateGenerartorInfo()
    {
        Annotation::Get().SaveData();
        auto Data = Annotation::Get().Data;
        auto CatData = CategoryManagement::Get().Data;
        auto CatChecker = CategoryManagement::Get().GeneratorCheckData;
        CategoriesExportCounts.clear();
        CategoriesExportCounts_Merged.clear();
        NumIncludedFrames = 0;
        NumIncludedAnnotations = 0;
        NumIncludedImages = 0;
        for (const auto& C : IncludedClips)
        {
            for (auto [FrameNum, Save] : Data[C.substr(Setting::Get().ProjectPath.length())])
            {
                if (IncludeReadyOnly && !Save.IsReady) continue;
                bool ContainsExportedCategory = false;
                for (const auto& [UID, Ann] : Save.AnnotationData)
                {
                    if (CatChecker[Ann.CategoryID])
                    {
                        if (!CategoriesExportCounts.count(Ann.CategoryID))
                            CategoriesExportCounts[Ann.CategoryID] = 1;
                        else
                            CategoriesExportCounts[Ann.CategoryID] += 1;

                        NumIncludedAnnotations++;
                        ContainsExportedCategory = true;
                    }
                }
                if (ContainsExportedCategory) NumIncludedFrames += 1;
            }
            
        }
        // handle images
        std::vector<std::string> IncludedImgs;
        for (const auto& IF : IncludedImageFolders)
        {
            for (const auto& p : std::filesystem::directory_iterator(Utils::ConvertUtf8ToWide(IF)))
            {
                if (p.is_directory()) continue;
                std::string FullPath = p.path().u8string();
                std::replace(FullPath.begin(), FullPath.end(), '\\', '/');
                IncludedImgs.push_back(FullPath);
            }
        }

        for (const auto& Img : IncludedImgs)
        {
            std::string RelImg = Img.substr(Setting::Get().ProjectPath.length());
            if (!Data.count(RelImg)) continue;
            FAnnotationSave ImgSave = Data[RelImg][0];
            if (IncludeReadyOnly && !ImgSave.IsReady) continue;
            bool ContainsExportedCategory = false;
            for (const auto& [UID, Ann] : ImgSave.AnnotationData)
            {
                UUID CID = Ann.CategoryID;
                if (CatChecker[CID])
                {
                    if (!CategoriesExportCounts.count(CID))
                        CategoriesExportCounts[CID] = 1;
                    else
                        CategoriesExportCounts[CID] += 1;
                    NumIncludedAnnotations++;
                    ContainsExportedCategory = true;
                }
            }
            if (ContainsExportedCategory) NumIncludedImages += 1;
        }

        TotalExportImages = NumIncludedFrames + NumIncludedImages;
        SplitImgs[0] = int(SplitPercent[0] * 0.01f * TotalExportImages);
        SplitImgs[1] = int(SplitPercent[1] * 0.01f * TotalExportImages);
        SplitImgs[2] = TotalExportImages - SplitImgs[0] - SplitImgs[1]; // to prevent lacking due to round off...

        CatExportSummary = "";
        char buf[64];
        if (ShouldMergeCategories)
        {
            for (const auto& Data : CategoryMergeData)
            {
                int Sum = 0;
                for (const UUID& CID : Data.SourceCatIdx)
                {
                    if (CategoriesExportCounts.count(CID))
                        Sum += CategoriesExportCounts[CID];
                }
                CategoriesExportCounts_Merged[Data.DisplayName] = Sum;
            }
            for (const auto& [CatName, Count] : CategoriesExportCounts_Merged)
            {
                sprintf(buf, "%s(%.2f %%) ", CatName.c_str(),
                        (float)Count / (float)NumIncludedAnnotations * 100.f);
                CatExportSummary += buf;
            }
        }
        else
        {
            for (const auto& [CID, Count] : CategoriesExportCounts)
            {
                sprintf(buf, "%s(%.2f %%) ", CatData[CID].DisplayName.c_str(),
                        (float)Count / (float)NumIncludedAnnotations * 100.f);
                CatExportSummary += buf;
            }
        }
    }

    std::unordered_map<UUID, size_t> CategoryExportedID;

    void Train::GenerateTrainingSet()
    {
        // create folders and description files
        const std::string ProjectPath = Setting::Get().ProjectPath;
        std::unordered_map<UUID, FCategory> CatData = CategoryManagement::Get().Data;
        std::unordered_map<UUID, bool> CatCheckData = CategoryManagement::Get().GeneratorCheckData;
        std::filesystem::create_directories(ProjectPath + "/Data/" + NewTrainingSetName);
        std::string SplitName[3] = {"train", "valid", "test"};
        std::string TypeName[2] = {"images", "labels"};
        CategoryExportedID.clear();
        size_t TempID = 0;
        for (const auto& [CID, V] : CategoriesExportCounts)
        {
            CategoryExportedID[CID] = TempID;
            TempID++;
        }
        for (const auto& s : SplitName)
        {
            std::filesystem::create_directories(ProjectPath + "/Data/" + NewTrainingSetName + "/" + s);
            for (const auto& t : TypeName)
            {
                std::filesystem::create_directories(ProjectPath + "/Data/" + NewTrainingSetName + "/" + s + "/" + t);
            }
        }
        // write readme.txt (export time, where it is from, some export settings?)
        std::ofstream ofs;
        ofs.open(ProjectPath + "/Data/" + NewTrainingSetName + "/IFCS.README.txt");
        if (ofs.is_open())
        {
            ofs << "Generated from IFCS" << std::endl;
            ofs << Utils::GetCurrentTimeString() << std::endl;
        }
        ofs.close();

        // write file to /Data/TrainingSets.yaml
        FTrainingSetDescription Desc;
        Desc.Name = NewTrainingSetName;
        if (ShouldMergeCategories)
        {
            for (const auto& [CatName, V] : CategoriesExportCounts_Merged)
            {
                Desc.Categories.push_back(CatName);
            }
        }
        else
        {
            for (const auto& [CID, V] : CategoriesExportCounts)
            {
                Desc.Categories.push_back(CatData[CID].DisplayName);
            }
        }
        Desc.CreationTime = Utils::GetCurrentTimeString();
        std::vector<std::string> IC;
        IC.assign(IncludedClips.begin(), IncludedClips.end());
        Desc.IncludeClips = IC;
        std::vector<std::string> II;
        II.assign(IncludedImageFolders.begin(), IncludedImageFolders.end());
        Desc.IncludeImageFolders = II;
        Desc.Size[0] = ExportedImageSize[0];
        Desc.Size[1] = ExportedImageSize[1];
        Desc.Split[0] = SplitPercent[0] / 100;
        Desc.Split[1] = SplitPercent[1] / 100;
        Desc.Split[2] = SplitPercent[2] / 100;
        Desc.TotalImagesExported = TotalExportImages;
        YAML::Node TSData = YAML::LoadFile(ProjectPath + "/Data/TrainingSets.yaml");
        TSData[Desc.Name] = Desc.Serialize();
        ofs.open(ProjectPath + "/Data/TrainingSets.yaml");
        YAML::Emitter Out;
        Out << TSData;
        ofs << Out.c_str();
        ofs.close();

        //write data file for yolo
        ofs.open(ProjectPath + "/Data/" + NewTrainingSetName + "/" + NewTrainingSetName + ".yaml");
        ofs << "train: " << ProjectPath + "/Data/" + NewTrainingSetName + "/train/images" << std::endl;
        ofs << "val: " << ProjectPath + "/Data/" + NewTrainingSetName + "/valid/images" << std::endl;
        ofs << "test: " << ProjectPath + "/Data/" + NewTrainingSetName + "/test/images" << std::endl << std::endl;
        size_t S = ShouldMergeCategories ? CategoriesExportCounts_Merged.size() : CategoriesExportCounts.size();
        ofs << "nc: " << S << std::endl;
        ofs << "names: [";
        size_t j = 0;
        if (ShouldMergeCategories)
        {
            for (const auto& Data : CategoryMergeData)
            {
                ofs << "'" << Data.DisplayName << "'";
                if (j < S - 1)
                    ofs << ", ";
                j++;
            }
        }
        else
        {
            for (const auto& [CID, N] : CategoriesExportCounts)
            {
                ofs << "'" << CatData[CID].DisplayName << "'";
                if (j < S - 1)
                    ofs << ", ";
                j++;
            }
        }
        ofs << "]" << std::endl;
        ofs.close();
        // save export setting

        // random choose train, valid, test based on options
        std::vector<int> RemainIdx(TotalExportImages);
        std::vector<int> TrainingIdx, ValidIdx;
        auto rnd = std::mt19937_64{std::random_device{}()};
        std::iota(std::begin(RemainIdx), std::end(RemainIdx), 0);
        std::sample(RemainIdx.begin(), RemainIdx.end(), std::back_inserter(TrainingIdx), SplitImgs[0], rnd);
        std::sort(TrainingIdx.begin(), TrainingIdx.end());
        auto NewEnd = std::remove_if(RemainIdx.begin(), RemainIdx.end(), [=](const int& i)
        {
            return std::find(TrainingIdx.begin(), TrainingIdx.end(), i) != TrainingIdx.end();
        });
        RemainIdx.erase(NewEnd, RemainIdx.end());
        std::sample(RemainIdx.begin(), RemainIdx.end(), std::back_inserter(ValidIdx), SplitImgs[1], rnd);
        std::sort(ValidIdx.begin(), ValidIdx.end());
        NewEnd = std::remove_if(RemainIdx.begin(), RemainIdx.end(), [=](const int& i)
        {
            return std::find(ValidIdx.begin(), ValidIdx.end(), i) != ValidIdx.end();
        });
        RemainIdx.erase(NewEnd, RemainIdx.end());
        std::vector<int> TestIdx = RemainIdx;

        // distribute train / valid / test and generate .jpg .txt to folders
        // only include frames with at least 1 annotation...
        int N = 0;
        YAML::Node Data = YAML::LoadFile(ProjectPath + "/Data/Annotations.yaml");
        for (const std::string& Clip : IncludedClips)
        {
            cv::VideoCapture Cap(Clip);
            std::string RelClip = Clip.substr(Setting::Get().ProjectPath.length());
            for (YAML::const_iterator it = Data[RelClip].begin(); it != Data[RelClip].end(); ++it)
            {
                if (it->second.size() == 0) continue;
                if (IncludeReadyOnly && !it->second["IsReady"].as<bool>()) continue;
                int FrameNum = it->first.as<int>();
                std::string GenName = Clip.substr(ProjectPath.size() + 1) + "_" + std::to_string(FrameNum);
                std::replace(GenName.begin(), GenName.end(), '/', '-');
                std::vector<FAnnotation> Annotations;
                auto Node = it->second.as<YAML::Node>();
                // should check category in each image
                for (YAML::const_iterator A = Node.begin(); A != Node.end(); ++A)
                {
                    if (A->first.as<std::string>() == "UpdateTime" || A->first.as<std::string>() == "IsReady")
                        continue;
                    if (CatCheckData[A->second["CategoryID"].as<uint64_t>()])
                        Annotations.emplace_back(A->second);
                }
                if (Annotations.empty()) continue;

                if (std::find(TrainingIdx.begin(), TrainingIdx.end(), N) != TrainingIdx.end())
                {
                    GenerateImgTxt(Cap, FrameNum, Annotations, GenName.c_str(), true, "Train");
                }
                else if (std::find(ValidIdx.begin(), ValidIdx.end(), N) != ValidIdx.end())
                {
                    GenerateImgTxt(Cap, FrameNum, Annotations, GenName.c_str(), true, "Valid");
                }
                else
                {
                    GenerateImgTxt(Cap, FrameNum, Annotations, GenName.c_str(), true, "Test");
                }
                N++;
            }
            Cap.release();
        }

        // handle images
        // TODO: chinese path will still block img generation here???
        /*
         * write different logic... that directory_iterator runsd on the top img folder... otherwise it will just crash...
         */
        std::vector<std::string> IncludedImgs;
        for (const auto& IF : IncludedImageFolders)
        {
            for (const auto& p : std::filesystem::directory_iterator(IF))
            {
                if (p.is_directory()) continue;
                std::string FullPath = p.path().u8string();
                std::replace(FullPath.begin(), FullPath.end(), '\\', '/');
                std::string RelPath = FullPath.substr(Setting::Get().ProjectPath.length());
                if (!Data[RelPath]) continue;
                if (IncludeReadyOnly && !Data[RelPath][0]["IsReady"].as<bool>()) continue;
                IncludedImgs.push_back(FullPath);
            }
        }

        
        for (const std::string& Img : IncludedImgs)
        {
            std::string SplitName;
            if (std::find(TrainingIdx.begin(), TrainingIdx.end(), N) != TrainingIdx.end())
                SplitName = "Train";
            else if (std::find(ValidIdx.begin(), ValidIdx.end(), N) != ValidIdx.end())
                SplitName = "Valid";
            else
                SplitName = "Test";
            std::string GenName = Img.substr(ProjectPath.size() + 1) + "_";
            std::replace(GenName.begin(), GenName.end(), '/', '-');
            const std::string OutImgName = Setting::Get().ProjectPath + "/Data/" + NewTrainingSetName + "/" + SplitName
                + "/images/" + GenName + ".jpg";
            const std::string OutTxtName = Setting::Get().ProjectPath + "/Data/" + NewTrainingSetName + "/" + SplitName
                + "/labels/" + GenName + ".txt";
            cv::Mat ImgData = cv::imread(Img);
            if (ImgData.empty())
            {
                // Solution to unicode path in opencv: https://stackoverflow.com/a/43369056
                std::wstring wpath = Utils::ConvertUtf8ToWide(Img);
                
                std::ifstream f(wpath, std::iostream::binary);
                // Get its size
                std::filebuf* pbuf = f.rdbuf();
                size_t size = pbuf->pubseekoff(0, f.end, f.in);
                pbuf->pubseekpos(0, f.in);

                // Put it in a vector
                std::vector<uchar> buffer(size);
                pbuf->sgetn((char*)buffer.data(), size);

                // Decode the vector
                ImgData = cv::imdecode(buffer, cv::IMREAD_COLOR);
            }
            
            cv::resize(ImgData, ImgData, cv::Size(ExportedImageSize[0], ExportedImageSize[1]));
            cv::imwrite(OutImgName, ImgData);

            std::vector<FAnnotation> Annotations;
            for (YAML::const_iterator it = Data[Img][0].begin(); it != Data[Img][0].end(); ++it)
            {
                if (it->first.as<std::string>() == "UpdateTime" || it->first.as<std::string>() == "IsReady")
                    continue;
                if (CatCheckData[it->second["CategoryID"].as<uint64_t>()])
                    Annotations.emplace_back(it->second);
            }
            std::ofstream ofs;
            ofs.open(OutTxtName);
            if (ofs.is_open())
            {
                for (auto& A : Annotations)
                {
                    if (ShouldMergeCategories)
                    {
                        int i = 0;
                        for (const auto& Data : CategoryMergeData)
                        {
                            if (Utils::Contains(Data.SourceCatIdx, A.CategoryID))
                                break;
                            i++;
                        }
                        ofs << A.GetExportTxt(i).c_str() << std::endl;
                    }
                    else
                    {
                        ofs << A.GetExportTxt(CategoryExportedID[A.CategoryID]).c_str() << std::endl;
                    }
                }
            }
            ofs.close();
            CurrentImgGenerated += 1;
            N++;
        }
    }

    void Train::GenerateImgTxt(cv::VideoCapture& Cap, int FrameNum, const std::vector<FAnnotation>& InAnnotations,
                               const char* GenName, bool IsOriginal, const char* SplitName)
    {
        const std::string OutImgName = Setting::Get().ProjectPath + "/Data/" + NewTrainingSetName + "/" + SplitName +
            "/images/" + GenName + ".jpg";
        const std::string OutTxtName = Setting::Get().ProjectPath + "/Data/" + NewTrainingSetName + "/" + SplitName +
            "/labels/" + GenName + ".txt";
        cv::Mat Frame;
        Cap.set(cv::CAP_PROP_POS_FRAMES, FrameNum);
        Cap >> Frame;
        cv::resize(Frame, Frame, cv::Size(ExportedImageSize[0], ExportedImageSize[1]));
        cv::imwrite(OutImgName, Frame);
        std::ofstream ofs;
        ofs.open(OutTxtName);
        if (ofs.is_open())
        {
            for (auto& A : InAnnotations)
            {
                if (ShouldMergeCategories)
                {
                    int i = 0;
                    for (const auto& Data : CategoryMergeData)
                    {
                        if (Utils::Contains(Data.SourceCatIdx, A.CategoryID))
                            break;
                        i++;
                    }
                    ofs << A.GetExportTxt(i).c_str() << std::endl;
                }
                else
                {
                    ofs << A.GetExportTxt(CategoryExportedID[A.CategoryID]).c_str() << std::endl;
                }
            }
        }
        ofs.close();
        CurrentImgGenerated += 1;
    }

    bool ModelNameIsDuplicated = false;

    void Train::RenderTrainingOptions()
    {
        // model select
        if (ImGui::BeginCombo("Choose Training Set", SelectedTrainingSet.Name.c_str()))
        {
            YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/TrainingSets.yaml");
            for (auto D : Data)
            {
                auto Name = D.first.as<std::string>();
                if (ImGui::Selectable(Name.c_str(), Name == SelectedTrainingSet.Name))
                {
                    SelectedTrainingSet = FTrainingSetDescription(Name, D.second);
                    UpdateTrainScript();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Checkbox("Train on existing model?", &bTrainOnExisitingModel);

        if (bTrainOnExisitingModel)
        {
            if (bTrainOnExternalModel)
            {
                ImGui::InputText("External Model Path", ExternalModelPath, IM_ARRAYSIZE(ExternalModelPath), ImGuiInputTextFlags_ReadOnly);
                ImGui::SameLine();
                if (ImGui::Button("..."))
                {
                    ImGuiFileDialog::Instance()->OpenDialog("ChooseExternalModelPath", "Choose external model file", ".pt", Setting::Get().ProjectPath, 1, nullptr, ImGuiFileDialogFlags_Modal);
                    UpdateTrainScript();
                }
            }
            else
            {
                if (ImGui::BeginCombo("Choose Existing model", SelectedExistingModel.Name.c_str()))
                {
                    YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Models/Models.yaml");
                    for (auto D : Data)
                    {
                        auto Name = D.first.as<std::string>();
                        if (ImGui::Selectable(Name.c_str(), Name == SelectedExistingModel.Name))
                        {
                            SelectedExistingModel = FModelDescription(Name, D.second);
                            UpdateTrainScript();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            if (ImGui::Checkbox("Choose external model?", &bTrainOnExternalModel))
            {
                UpdateTrainScript();
            }
        }
        else
        {
            if (ImGui::Combo("Choose Model", &SelectedModelIdx, ModelOptions, IM_ARRAYSIZE(ModelOptions)))
            {
                UpdateTrainScript();
                if (SelectedModelIdx == 4)
                    bApplyTransferLearning = false;
            }
            
            // apply transfer learning?
            if (ImGui::Checkbox("Apply Transfer learning?", &bApplyTransferLearning))
            {
                UpdateTrainScript();
            }
            ImGui::SameLine();
            ImGui::Text("(Tiny not supported)");
        }


        // epoch
        if (ImGui::InputInt("Num Epoch", &Epochs, 1, 10))
        {
            if (Epochs < 1) Epochs = 1;
            if (Epochs > 300) Epochs = 300;
            UpdateTrainScript();
        }
        // batch size
        if (ImGui::InputInt("Num batch size", &BatchSize, 1, 10))
        {
            if (BatchSize < 1) BatchSize = 1;
            if (BatchSize > 128) BatchSize = 128;
            UpdateTrainScript();
        }
        // Image size
        if (ImGui::InputInt("Image size (Train)", &TrainImgSize, 32, 128))
        {
            UpdateTrainScript();
        }
        if (ImGui::InputInt("Image size (Test)", &TestImgSize, 32, 128))
        {
            UpdateTrainScript();
        }
        // model name
        if (ImGui::InputText("Model name", &ModelName))
        {
            ModelNameIsDuplicated = false;
            for (auto D : YAML::LoadFile(Setting::Get().ProjectPath + "/Models/Models.yaml"))
            {
                if (D.first.as<std::string>() == ModelName)
                {
                    ModelNameIsDuplicated = true;
                    break;
                }
            }
            UpdateTrainScript();
        }

        // Add advanced options
        if (ImGui::TreeNode("Advanced"))
        {
            if (ImGui::Checkbox("Evolve hyperparameters?", &ApplyEvolve))
            {
                UpdateTrainScript();
            }
            if (ImGui::Checkbox("Use Adam optimizer?", &ApplyAdam))
            {
                UpdateTrainScript();
            }
            if (ImGui::Checkbox("Cache images for faster training?", &ApplyCacheImage))
            {
                UpdateTrainScript();
            }
            if (ImGui::Checkbox("Use weighted image selection for training?", &ApplyWeightedImageSelection))
            {
                UpdateTrainScript();
            }
            if (ImGui::Checkbox("Vary img-size +/- 50%%?", &ApplyMultiScale))
            {
                UpdateTrainScript();
            }
            if (ImGui::InputInt("Max number of dataloader workers", &MaxWorkers))
            {
                if (MaxWorkers < 1) MaxWorkers = 1;
                UpdateTrainScript();
            }
            // cuda device
            ImGui::Text("Cuda devices to use:");
            if (ImGui::Checkbox("0", &Devices[0])) UpdateTrainScript();
            ImGui::SameLine();
            if (ImGui::Checkbox("1", &Devices[1])) UpdateTrainScript();
            ImGui::SameLine();
            if (ImGui::Checkbox("2", &Devices[2])) UpdateTrainScript();
            ImGui::SameLine();
            if (ImGui::Checkbox("3", &Devices[3])) UpdateTrainScript();
            ImGui::SameLine();
            if (ImGui::Checkbox("4", &Devices[4])) UpdateTrainScript();
            ImGui::SameLine();
            if (ImGui::Checkbox("5", &Devices[5])) UpdateTrainScript();
            ImGui::SameLine();
            if (ImGui::Checkbox("6", &Devices[6])) UpdateTrainScript();
            ImGui::SameLine();
            if (ImGui::Checkbox("7", &Devices[7])) UpdateTrainScript();


            ImGui::TreePop();
        }
    }

    void Train::RenderTrainingScript()
    {
        if ( ModelName.empty() || ModelNameIsDuplicated)
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Model name is empty or duplicated!");
        }
        else if (SelectedTrainingSet.Name.empty())
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Training set is not selected!");
        }
        else if (bTrainOnExisitingModel && !bTrainOnExternalModel && SelectedExistingModel.Name.empty())
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Existing model is not selected!");
        }
        else if (bTrainOnExisitingModel && bTrainOnExternalModel && strlen(ExternalModelPath) == 0)
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "External model path is empty!");
        }
        else
        {
            ImGui::Text("About to run: ");
            ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 32);
            if (ImGui::Button(ICON_FA_COPY))
            {
                ImGui::SetClipboardText((SetPathScript + "\n" + TrainScript).c_str());
            }
            ImGui::BeginChildFrame(ImGui::GetID("TrainScript"), ImVec2(0, ImGui::GetTextLineHeight() * 6));
            ImGui::TextWrapped((SetPathScript + "\n" + TrainScript).c_str());
            ImGui::EndChildFrame();
            if (ImGui::Button("Start Training", ImVec2(240, 0)))
            {
                Training();
            }
        }
    }

    void Train::Training()
    {
        if (IsTraining) return;
        TrainLog = Utils::GetCurrentTimeString(true) + " Start training... Check progross in Console or TensorBoard\n";

        // system can noy handle command quene...
        auto func = [=]()
        {
            system(SetPathScript.c_str());
            if (bApplyTransferLearning)
            {
                std::string PtFile = Setting::Get().YoloV7Path + "/" + ModelOptions[SelectedModelIdx] + "_training.pt";
                if (!std::filesystem::exists(PtFile))
                {
                    std::string AppPath = std::filesystem::current_path().u8string();
                    std::string DownloadWeightCommand = Setting::Get().PythonPath + "/python " + AppPath +
                        "/Scripts/DownloadWeight.py --model " + std::string(ModelOptions[SelectedModelIdx]);
                    std::ofstream ofs;
                    ofs.open("DownloadWeight.bat");
                    ofs << SetPathScript << " &^\n" << DownloadWeightCommand;
                    ofs.close();
                    system("DownloadWeight.bat");
                }
            }
            std::ofstream ofs;
            ofs.open("Train.bat");
            ofs << SetPathScript << " &^\n" << TrainScript;
            ofs.close();
            system("Train.bat");
        };
        IsTraining = true;
        TrainingFuture = std::async(std::launch::async, func);
    }


    void Train::OpenTensorBoard()
    {
        if (!HasHostTensorBoard)
        {
            // make host only once
            char host[255];
            if (Setting::Get().PythonPath.find("Scripts") != std::string::npos)
                sprintf(host, "%s/tensorboard --logdir %s/Models", Setting::Get().PythonPath.c_str(),
                    Setting::Get().ProjectPath.c_str());
            else
                sprintf(host, "%s/Scripts/tensorboard --logdir %s/Models", Setting::Get().PythonPath.c_str(),
                    Setting::Get().ProjectPath.c_str());
            auto func = [=]()
            {
                system(host);
            };
            std::thread t(func);
            t.detach();
            HasHostTensorBoard = true;
        }
        char url[100] = "http://localhost:6006/";
        ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    }

    void Train::OnTrainingFinished()
    {
        IsTraining = false;
        // if there is no results.txt, just assume it failed...
        if (!std::filesystem::exists(Setting::Get().ProjectPath + "/Models/" + ModelName + "/results.txt"))
        {
            TrainLog += Utils::GetCurrentTimeString(true) + " Training FAILED!\n";
            std::filesystem::remove_all(Setting::Get().ProjectPath + "/Models/" + ModelName);
            // delete duplicated folder... 
            return;
        }
        TrainLog += Utils::GetCurrentTimeString(true) + " Training complete!\n";

        // store results data
        std::ifstream InFile(Setting::Get().ProjectPath + "/Models/" + ModelName + "/results.txt");
        std::vector<std::vector<std::string>> Results;
        std::string line;
        while (std::getline(InFile, line))
        {
            std::vector<std::string> Row;
            std::stringstream sline(std::regex_replace(line, std::regex(" +"), ",").substr(1));
            std::string item;
            while (std::getline(sline, item, ','))
            {
                Row.push_back(item);
            }
            Results.push_back(Row);
        }
        // get best (compared by 0.1*mAP@0.5 + 0.9*mAP@0.5:0.95)
        size_t BestIt = 0;
        float BestFitness = 0.f;
        int i = 0;
        for (auto Row : Results)
        {
            float Fitness = std::stof(Row[10]) * 0.1f + std::stof(Row[11]) * 0.9f;
            if (BestFitness < Fitness)
            {
                BestFitness = Fitness;
                BestIt = i;
            }
            i++;
        }

        FModelDescription Desc;
        Desc.Name = ModelName;
        Desc.CreationTime = Utils::GetCurrentTimeString();
        Desc.Categories = SelectedTrainingSet.Categories;
        Desc.ModelType = ModelOptions[SelectedModelIdx];
        Desc.SourceTrainingSet = SelectedTrainingSet.Name;
        Desc.Precision = std::stof(Results[BestIt][8]);
        Desc.Recall = std::stof(Results[BestIt][9]);
        Desc.mAP5 = std::stof(Results[BestIt][10]);
        Desc.mAP5_95 = std::stof(Results[BestIt][11]);
        YAML::Node Origin = YAML::LoadFile(Setting::Get().ProjectPath + "/Models/Models.yaml");
        Origin[Desc.Name] = Desc.Serialize();
        std::ofstream ofs(Setting::Get().ProjectPath + "/Models/Models.yaml");
        YAML::Emitter Out;
        Out << Origin;
        ofs << Out.c_str();
        ofs.close();

        // delete epoch_XXX.pt in weights folder to save space?
        for (auto entry : std::filesystem::directory_iterator(
                 Setting::Get().ProjectPath + "/Models/" + ModelName + "/weights/"))
        {
            if (entry.path().filename().u8string().find("epoch") != std::string::npos)
            {
                std::filesystem::remove(entry.path());
            }
        }

        
        ModelName.clear();
    }

    void Train::UpdateTrainScript()
    {
        
        const bool IsChoose6Model = bTrainOnExisitingModel?
            Utils::Contains(Model6Names, SelectedExistingModel.ModelType) : Utils::Contains(Model6Indices, SelectedModelIdx);

        SetPathScript = "cd /d " + Setting::Get().YoloV7Path;
        TrainScript = "";
        if (IsChoose6Model)
            TrainScript += Setting::Get().PythonPath + "/python train_aux.py";
        else
            TrainScript += Setting::Get().PythonPath + "/python train.py";
        TrainScript += " --weights ";
        if (bTrainOnExisitingModel)
        {
            if (bTrainOnExternalModel && strlen(ExternalModelPath) != 0)
            {
                TrainScript += ExternalModelPath;
            }
            else
            {
                TrainScript += Setting::Get().ProjectPath + "/Models/" + SelectedExistingModel.Name + "/weights/best.pt";
            }
        }
        else
        {
            if (bApplyTransferLearning)
                TrainScript += std::string(ModelOptions[SelectedModelIdx]) + "_training.pt";
            else
                TrainScript += "''";
        }
        TrainScript += " --cfg cfg/training/" + std::string(ModelOptions[SelectedModelIdx]) + ".yaml";
        TrainScript += " --data " + Setting::Get().ProjectPath + "/Data/" + SelectedTrainingSet.Name + "/" +
            SelectedTrainingSet.Name + ".yaml";
        if (IsChoose6Model)
            TrainScript += " --hyp data/hyp.scratch.p6.yaml";
        else if (SelectedModelIdx == 4)
            TrainScript += " --hyp data/hyp.scratch.tiny.yaml";
        else
            TrainScript += " --hyp data/hyp.scratch.p5.yaml";
        TrainScript += " --epochs " + std::to_string(Epochs);
        TrainScript += " --batch-size " + std::to_string(BatchSize);
        TrainScript += " --img-size " + std::to_string(TrainImgSize) + " " + std::to_string(TestImgSize);
        TrainScript += " --workers " + std::to_string(MaxWorkers);
        std::string temp;
        int i = 0;
        for (bool b : Devices)
        {
            if (b) temp += std::to_string(i) + ",";
            i++;
        }
        TrainScript += " --device " + temp.substr(0, temp.size() - 1);
        if (ApplyEvolve) TrainScript += " --evolve";
        if (ApplyAdam) TrainScript += " --adam";
        if (ApplyCacheImage) TrainScript += " --cache-images";
        if (ApplyWeightedImageSelection) TrainScript += " --image-weights";
        if (ApplyMultiScale) TrainScript += " --multi-scale";
        TrainScript += " --project " + Setting::Get().ProjectPath + "/Models";
        TrainScript += " --name " + std::string(ModelName);

        TensorBoardHostCommand = "";
        TensorBoardHostCommand += Setting::Get().PythonPath + "/Scripts/tensorboard --logdir " +
            Setting::Get().ProjectPath + "/Models";
    }
}
