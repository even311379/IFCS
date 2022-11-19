#include "Train.h"
#include "Setting.h"

#include <fstream>
#include <future>
#include <chrono>
#include <numeric>
#include <random>
#include <thread>
#include <string>

#include "CategoryManagement.h"
#include "DataBrowser.h"
#include "yaml-cpp/yaml.h"
#include "ImFileDialog/ImFileDialog.h"
#include "IconFontCppHeaders/IconsFontAwesome5.h"
#include "misc/cpp/imgui_stdlib.h"
#include "Modals.h"
#include "Style.h"

#include "shellapi.h"
#include "Implot/implot.h"

// TODO: model comparison!!!

namespace IFCS
{
    static std::string RunResult;

    static const char* ModelOptions[] = {
        "yolov7", "yolov7-d6", "yolov7-e6", "yolov7-e6e", "yolov7-tiny", "yolov7-w6", "yolov7x"
    };

    static const char* HypOptions[] = {
        "p5", "p6", "tiny", "custom"
    };

    void Train::Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose)
    {
        Panel::Setup(InName, InShouldOpen, InFlags, InCanClose);
        UpdateTrainScript();
    }

    void Train::RenderContent()
    {
        if (ImGui::BeginTabBar("TrainTabs"))
        {
            if (ImGui::BeginTabItem("Training Set"))
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
                if (ImGui::TreeNode("Train / Valid / Test Split"))
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
                RenderTrainingOptions();
                RenderTrainingScript();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        if (IsTraining)
        {
            if (TrainingFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
            {
                TrainLog += Utils::GetCurrentTimeString(true) + " Training complete!\n";

                // TODO: write model info to yaml?
                // need to calculate best iter and save its metrics
                /*
            fi = fitness(np.array(results).reshape(1, -1))               
            def fitness(x):
                # Model fitness as a weighted combination of metrics
                w = [0.0, 0.0, 0.1, 0.9]  # weights for [P, R, mAP@0.5, mAP@0.5:0.95]
                return (x[:, :4] * w).sum(1)
                 */
                IsTraining = false;
            }
        }
    }

    void Train::RenderDataSelectWidget()
    {
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
                IncludedClips.push_back(ClipToInclude);
                UpdateGenerartorInfo();
            }
            if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("ImageFolder"))
            {
                std::string ImageFolderToInclude = std::string((const char*)Payload->Data);
                IncludedImageFolders.push_back(ImageFolderToInclude);
                UpdateGenerartorInfo();
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
        auto CatData = CategoryManagement::Get().Data;
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

        int C = 0;
        for (auto [s, i] : CategoriesExportCounts)
            C += i;
        const size_t NumBars = CategoriesExportCounts.size();
        if (C > 0 && NumBars > 1)
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


    float SplitPercent[3] = {70, 30, 0};
    int SplitImgs[3];

    void Train::RenderSplitWidget()
    {
        if (TotalExportImages == 0)
        {
            ImGui::Text("No training image to export!");
            return;
        }
        static float SplitControlPos1 = 0.7f;
        static float SplitControlPos2 = 1.f;
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
            (SplitControlPos1 + (SplitControlPos2 - SplitControlPos1) * 0.5f) * AvailWidth - TxtWidth2 * 0.5f, TxtPosY);
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

    char NewTrainingSetName[64];

    void Train::RenderTrainingSetExportWidget()
    {
        ImGui::SetNextItemWidth(240.f);
        ImGui::InputText("Training Set Name", NewTrainingSetName, IM_ARRAYSIZE(NewTrainingSetName));
        ImGui::SameLine();
        if (ImGui::Button("Generate", ImVec2(240, 0)))
        {
            GenerateTrainingSet();
        }
    }

    void Train::UpdateGenerartorInfo()
    {
        YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/Annotations.yaml");
        auto CatData = CategoryManagement::Get().Data;
        auto CatChecker = CategoryManagement::Get().GeneratorCheckData;
        CategoriesExportCounts.clear();
        NumIncludedFrames = 0;
        NumIncludedAnnotations = 0;
        NumIncludedImages = 0;
        for (const auto& C : IncludedClips)
        {
            for (YAML::const_iterator Frame = Data[C].begin(); Frame != Data[C].end(); ++Frame)
            {
                YAML::Node ANode = Frame->second.as<YAML::Node>();
                bool ContainsExportedCategory = false;
                for (YAML::const_iterator A = ANode.begin(); A != ANode.end(); ++A)
                {
                    if (A->first.as<std::string>() == "UpdateTime" || A->first.as<std::string>() == "IsReady")
                        continue;

                    UUID CID = A->second["CategoryID"].as<uint64_t>();
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
                if (ContainsExportedCategory) NumIncludedFrames += 1;
            }
        }
        // handle images
        std::vector<std::string> IncludedImgs;
        for (const auto& IF : IncludedImageFolders)
        {
            for (const auto& p : std::filesystem::directory_iterator(IF))
            {
                if (p.is_directory()) continue;
                std::string FullPath = p.path().u8string();
                std::replace(FullPath.begin(), FullPath.end(), '\\', '/');
                IncludedImgs.push_back(FullPath);
            }
        }

        for (const auto& Img : IncludedImgs)
        {
            if (!Data[Img]) continue;
            YAML::Node ANode = Data[Img][0];
            bool ContainsExportedCategory = false;
            for (YAML::const_iterator A = ANode.begin(); A != ANode.end(); ++A)
            {
                if (A->first.as<std::string>() == "UpdateTime" || A->first.as<std::string>() == "IsReady")
                    continue;

                UUID CID = A->second["CategoryID"].as<uint64_t>();
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
        for (const auto& [CID, Count] : CategoriesExportCounts)
        {
            char buf[64];
            sprintf(buf, "%s(%.2f %%) ", CatData[CID].DisplayName.c_str(),
                    (float)Count / (float)NumIncludedAnnotations * 100.f);
            CatExportSummary += buf;
        }
    }

    std::unordered_map<UUID, size_t> CategoryExportedID;
    
    void Train::GenerateTrainingSet()
    {
        // TODO: gen image folders is still missing...
        // TODO: is category info generated???
        
        if (std::strlen(NewTrainingSetName) == 0) return;
        // create folders and description files
        const std::string ProjectPath = Setting::Get().ProjectPath;
        std::filesystem::create_directories(ProjectPath + "/Data/" + NewTrainingSetName);
        std::string SplitName[3] = {"train", "valid", "test"};
        std::string TypeName[2] = {"images", "labels"};
        CategoryExportedID.clear();
        size_t i = 0;
        for (const auto& [CID, V] : CategoriesExportCounts )
        {
            CategoryExportedID[CID] = i;
            i++;
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
        ofs.open(ProjectPath + "/Data/TrainingSets.yaml");
        if (ofs.is_open())
        {
            YAML::Node Data = YAML::LoadFile(ProjectPath + "/Data/TrainingSets.yaml");
            FTrainingSetDescription Desc;
            Desc.Name = NewTrainingSetName;
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
            Data.push_back(Desc.Serialize());
            YAML::Emitter Out;
            Out << Data;
            ofs << Out.c_str();
        }
        ofs.close();

        std::unordered_map<UUID, FCategory> CatData = CategoryManagement::Get().Data;
        std::unordered_map<UUID, bool> CatCheckData = CategoryManagement::Get().GeneratorCheckData;
        

        //write data file for yolo
        ofs.open(ProjectPath + "/Data/" + NewTrainingSetName + "/" + NewTrainingSetName + ".yaml");
        if (ofs.is_open())
        {
            ofs << "train: " << ProjectPath + "/Data/" + NewTrainingSetName + "/train/images" << std::endl;
            ofs << "val: " << ProjectPath + "/Data/" + NewTrainingSetName + "/valid/images" << std::endl;
            ofs << "test: " << ProjectPath + "/Data/" + NewTrainingSetName + "/test/images" << std::endl << std::endl;
            size_t S = CategoriesExportCounts.size();
            ofs << "nc: " << S << std::endl;
            ofs << "names: [";
            size_t i = 0;
            for (const auto& [CID, N] : CategoriesExportCounts)
            {
                ofs << "'" << CatData[CID].DisplayName << "'";
                if (i < S - 1)
                    ofs << ", ";
                i++;
            }
            ofs << "]" << std::endl;
        }
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
            for (YAML::const_iterator it = Data[Clip].begin(); it != Data[Clip].end(); ++it)
            {
                if (it->second.size() == 0) continue;
                int FrameNum = it->first.as<int>();
                std::string GenName = Clip.substr(ProjectPath.size() + 1) + "_" + std::to_string(FrameNum);
                std::replace(GenName.begin(), GenName.end(), '/', '-');
                std::vector<FAnnotation> Annotations;
                auto Node = it->second.as<YAML::Node>();
                // should check category in each image
                for (YAML::const_iterator A = Node.begin(); A != Node.end(); ++A)
                {
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
        cv::cvtColor(Frame, Frame, cv::COLOR_BGR2RGB);
        cv::imwrite(OutImgName, Frame);
        std::ofstream ofs;
        ofs.open(OutTxtName);
        if (ofs.is_open())
        {
            for (auto& A : InAnnotations)
            {
                ofs << A.GetExportTxt(CategoryExportedID[A.CategoryID]).c_str() << std::endl;
            }
        }
        ofs.close();
    }

    void Train::RenderTrainingOptions()
    {
        // model select
        if (ImGui::Combo("Choose Model", &SelectedModelIdx, ModelOptions, IM_ARRAYSIZE(ModelOptions)))
        {
            UpdateTrainScript();
            if (SelectedModelIdx == 4)
                bApplyTransferLearning = false;
        }

        if (ImGui::BeginCombo("Choose Training Set", SelectedTrainingSet.Name.c_str()))
        {
            YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/TrainingSets.yaml");
            // for (size_t i = 0; i < Data.size(); i++)
            for (YAML::const_iterator it = Data.begin(); it != Data.end(); ++it)
            {
                auto Name = it->first.as<std::string>();
                if (ImGui::Selectable(Name.c_str(), Name == SelectedTrainingSet.Name))
                {
                    SelectedTrainingSet = FTrainingSetDescription(it->first.as<std::string>(), it->second);
                    UpdateTrainScript();
                }
            }

            ImGui::EndCombo();
        }

        // apply transfer learning?
        if (ImGui::Checkbox("Apply Transfer learning?", &bApplyTransferLearning))
        {
            UpdateTrainScript();
        }
        ImGui::SameLine();
        ImGui::Text("(Tiny not supported)");
        // TODO: allow to selection custom pre-trained weight?

        // hyp options
        if (ImGui::Combo("Hyper parameter", &SelectedHypIdx, HypOptions, IM_ARRAYSIZE(HypOptions)))
        {
            UpdateTrainScript();
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
        if (ImGui::InputInt("Image Width", &ImageSize[0], 32, 128))
        {
            if (ImageSize[0] < 64) ImageSize[0] = 64;
            if (ImageSize[0] > 1280) ImageSize[0] = 1280;
            UpdateTrainScript();
        }
        if (ImGui::InputInt("Image Height", &ImageSize[1], 32, 128))
        {
            if (ImageSize[1] < 64) ImageSize[1] = 64;
            if (ImageSize[1] > 1280) ImageSize[1] = 1280;
            UpdateTrainScript();
        }
        // model name
        if (ImGui::InputText("Model name", &ModelName))
        {
            // TODO: check if the name is used...
            UpdateTrainScript();
        }
    }

    void Train::RenderTrainingScript()
    {
        if (SelectedTrainingSet.Name.empty() || ModelName.empty())
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Training set or model name not set yet!");
        }
        else
        {
            ImGui::Text("About to run: ");
            ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 32);
            if (ImGui::Button(ICON_FA_COPY))
            {
                ImGui::SetClipboardText((SetPathScript + "\n" + TrainScript).c_str());
            }
            ImGui::BeginChildFrame(ImGui::GetID("TrainScript"), ImVec2(0, ImGui::GetTextLineHeight() * 4));
            ImGui::TextWrapped((SetPathScript + "\n" + TrainScript).c_str());
            ImGui::EndChildFrame();
            if (ImGui::Button("Start Training", ImVec2(-1, 0)))
            {
                Training();
            }
            if (!TrainLog.empty())
            {
                ImGui::Text("Training Log:");
                ImGui::BeginChildFrame(ImGui::GetID("TrainLog"), ImVec2(0, ImGui::GetTextLineHeight() * 3));
                ImGui::TextWrapped(TrainLog.c_str());
                ImGui::EndChildFrame();
            }
        }
        ImGui::Text("Check progress with tensorboard:");
        if (ImGui::Button("Host & Open tensorboard", ImVec2(-1, 0)))
        {
            OpenTensorBoard();
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
            const char host[150] =
                " K:/Python/python-3.10.8-embed-amd64/Scripts/tensorboard --logdir L:/IFCS_DEV_PROJECTS/Great/Models";
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

    void Train::UpdateTrainScript()
    {
        SetPathScript = "cd " + Setting::Get().YoloV7Path;
        TrainScript = "";
        TrainScript += Setting::Get().PythonPath + "/python train.py";
        TrainScript += " --weights ";
        if (bApplyTransferLearning)
            TrainScript += std::string(ModelOptions[SelectedModelIdx]) + "_training.pt";
        else
            TrainScript += "''";
        TrainScript += " --cfg cfg/training/" + std::string(ModelOptions[SelectedModelIdx]) + ".yaml";
        TrainScript += " --data " + Setting::Get().ProjectPath + "/Data/" + SelectedTrainingSet.Name + "/" +
            SelectedTrainingSet.Name + ".yaml";
        TrainScript += " --hyp data/hyp.scratch." + std::string(HypOptions[SelectedHypIdx]) + ".yaml";
        TrainScript += " --epochs " + std::to_string(Epochs);
        TrainScript += " --batch-size " + std::to_string(BatchSize);
        TrainScript += " --img-size " + std::to_string(ImageSize[0]) + " " + std::to_string(ImageSize[1]);
        TrainScript += " --workers 1 --device 0";
        TrainScript += " --project " + Setting::Get().ProjectPath + "/Models";
        TrainScript += " --name " + std::string(ModelName);

        TensorBoardHostCommand = "";
        TensorBoardHostCommand += Setting::Get().PythonPath + "/Scripts/tensorboard --logdir " +
            Setting::Get().ProjectPath + "/Models";
    }
}
