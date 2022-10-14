#include "TrainingSetGenerator.h"
#include "DataBrowser.h"
#include "Setting.h"

#include "imgui_impl_glfw.h"
#include <GLFW/glfw3.h>
#include <yaml-cpp/node/node.h>

#include "imgui_internal.h"
#include "ImguiNotify/font_awesome_5.h"
#include "Implot/implot.h"

#include "opencv2/opencv.hpp"
#include "Spectrum/imgui_spectrum.h"
#include "yaml-cpp/yaml.h"


#define MAKE_PREVIEW_IMAGE_VAR(n)\
    ImGui::PushID(n); \
    ImGui::Image((void*)(intptr_t)Var##n, PreviewImgSize); \
    LastImage_x2 = ImGui::GetItemRectMax().x; \
    NextImage_x2 = LastImage_x2 + style.ItemSpacing.x + PreviewImgSize.x; \
    if (n + 1 < PreviewCount && NextImage_x2 < window_visible_x2) \
        ImGui::SameLine(); \
        ImGui::PopID();

#define MAKE_GUI_IMG(n)\
    cv::Mat Mat##n = GenerateAugentationImage(Lena);\
    glGenTextures(1, &Var##n);\
    glBindTexture(GL_TEXTURE_2D, Var##n);\
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);\
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);\
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);\
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);\
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, Mat##n.data);

namespace IFCS
{
    void TrainingSetGenerator::RenderContent()
    {
        const ImVec2 ChildWindowSize = {ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.92f};
        constexpr ImGuiWindowFlags Flags = ImGuiWindowFlags_NoScrollbar;
        ImGui::BeginChild("All", ChildWindowSize, false, Flags);
        {
            const ImVec2 HalfWindowSize = {ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetContentRegionAvail().y};
            ImGui::BeginChild("Options", HalfWindowSize);
            {
                ImGui::PushFont(Setting::Get().TitleFont);
                ImGui::Text("Options");
                ImGui::PopFont();
                if (ImGui::TreeNode("Data Select"))
                {
                    ImGui::Text("Select which folders / clips to include in this training set:");
                    const ImVec2 FrameSize = ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 9);
                    float HalfWidth = ImGui::GetContentRegionAvail().x * 0.5f;
                    ImGui::BeginChildFrame(ImGui::GetID("Included Contents"), FrameSize, ImGuiWindowFlags_NoMove);
                    for (const std::string& Clip : IncludedGenClips)
                        ImGui::Text(Clip.c_str());
                    ImGui::EndChildFrame();
                    if (ImGui::BeginDragDropTarget()) // for the previous item? 
                    {
                        if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("FolderOrClip"))
                        {
                            std::string ClipToInclude = std::string((const char*)Payload->Data);
                            IncludeGenClip(ClipToInclude);
                        }
                    }
                    char AddClipPreviewTitle[128];
                    snprintf(AddClipPreviewTitle, sizeof(AddClipPreviewTitle), "%s Add Clip", ICON_FA_PLUS);
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
                    if (ImGui::BeginCombo("##AddClip", AddClipPreviewTitle))
                    {
                        std::vector<std::string> AllClips = DataBrowser::Get().GetAllClips();
                        auto NewEnd = std::remove_if(AllClips.begin(), AllClips.end(), [this](const std::string& C)
                        {
                            return std::find(IncludedGenClips.begin(), IncludedGenClips.end(), C) != IncludedGenClips.
                                end();
                        });
                        AllClips.erase(NewEnd, AllClips.end());
                        for (size_t i = 0; i < AllClips.size(); i++)
                        {
                            if (ImGui::Selectable(AllClips[i].c_str(), false)) // no need to show selected
                            {
                                IncludeGenClip(AllClips[i]);
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();
                    char AddFolderPreviewTitle[128];
                    snprintf(AddFolderPreviewTitle, sizeof(AddFolderPreviewTitle), "%s Add Folder", ICON_FA_PLUS);
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    if (ImGui::BeginCombo("##AddFolder", AddFolderPreviewTitle))
                    {
                        std::vector<std::string> AllFolders = DataBrowser::Get().GetAllFolders();
                        auto NewEnd = std::remove_if(AllFolders.begin(), AllFolders.end(), [this](const std::string& C)
                        {
                            return std::find(IncludedGenFolders.begin(), IncludedGenFolders.end(), C) !=
                                IncludedGenFolders.end();
                        });
                        AllFolders.erase(NewEnd, AllFolders.end());
                        // TODO: leave include folder for now...
                        for (size_t i = 0; i < AllFolders.size(); i++)
                        {
                            if (ImGui::Selectable(AllFolders[i].c_str(), false)) // no need to show selected
                            {
                                IncludeGenFolder(AllFolders[i]);
                            }
                        }
                        ImGui::EndCombo();
                    }
                    if (ImGui::Button("Add All", ImVec2(HalfWidth, ImGui::GetFontSize() * 1.5f)))
                    {
                        for (const auto& c : DataBrowser::Get().GetAllClips())
                            IncludedGenClips.insert(c);
                        UpdateExportInfo();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Clear All", ImVec2(HalfWidth, ImGui::GetFontSize() * 1.5f)))
                    {
                        IncludedGenClips.clear();
                        UpdateExportInfo();
                    }
                    ImGui::BulletText("Included Clips: %d", (int)IncludedGenClips.size());
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(HalfWidth);
                    ImGui::BulletText("Included frames: %d", NumIncludedFrames);
                    ImGui::BulletText("Included Images: %d", NumIncludedImages);
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(HalfWidth);
                    ImGui::BulletText("Included Annotations: %d", NumIncludedAnnotation);

                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Train / Test Split"))
                {
                    DrawSplitWidget();
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Resize"))
                {
                    // ImGui::Text("Add crop and pan???");
                    const char* ResizeRules[] = {"Stretch", "Fit"};
                    ImGui::Combo("Resize Rule", &SelectedResizeRule, ResizeRules, IM_ARRAYSIZE(ResizeRules));
                    ImGui::SetNextItemWidth(130.f);
                    ImGui::InputInt2("New Size", NewSize);
                    ImGui::Button("Preview");
                    ImGui::Text("Add preview directly here??");
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Augmentation"))
                {
                    if (ImGui::TreeNode("Blur"))
                    {
                        ImGui::Checkbox("Add Blur?", &bApplyBlur);
                        if (bApplyBlur)
                        {
                            if (ImGui::SliderInt("Max Blur Amount", &MaxBlurAmount, 1, 25))
                            {
                                // force odd
                                if (MaxBlurAmount % 2 == 0)
                                    MaxBlurAmount -= 1;
                            }
                        }
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Noise"))
                    {
                        ImGui::Checkbox("Add Noise?", &bApplyNoise);
                        if (bApplyNoise)
                        {
                            ImGui::SliderInt("Noise Mean Min", &NoiseMeanMin, 0, 50, "%d %");
                            ImGui::SliderInt("Noise Mean Max", &NoiseMeanMax, 0, 50, "%d %");
                            ImGui::SliderInt("Noise Std Min", &NoiseStdMin, 0, 50, "%d %");
                            ImGui::SliderInt("Noise Std Max", &NoiseStdMax, 0, 50, "%d %");
                        }
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Hue"))
                    {
                        ImGui::Checkbox("Apply Hue Shift", &bApplyHue);
                        if (bApplyHue)
                        {
                            ImGui::SliderInt("Max Random Hue (+ -)", &HueAmount, 1, 60, "%d %");
                        }
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Rotation"))
                    {
                        ImGui::Checkbox("Add random rotation?", &bApplyRotation);
                        if (bApplyRotation)
                        {
                            ImGui::SliderInt("Max Random Rotation degree Amount", &MaxRotationAmount, 1, 15, "%d deg");
                        }
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Flip"))
                    {
                        ImGui::Checkbox("Add Random flip X", &bApplyFlipX);
                        if (bApplyFlipX)
                            ImGui::DragInt("FlipX_Percent", &FlipXPercent, 1, 5, 50);
                        ImGui::Checkbox("Add random flip y?", &bApplyFlipY);
                        if (bApplyFlipY)
                            ImGui::DragInt("FlipY_Percent", &FlipYPercent, 1, 5, 50);
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Category To Export"))
                {
                    ImGui::Checkbox("Apply default categories", &bApplyDefaultCategories);
                    // TODO: skip the impl for now...
                    if (!bApplyDefaultCategories)
                    {
                        ImGui::Text("some category merging tools");
                    }
                    // TODO: add implot to check category imbalance
                    // ImPlot::
                    ImGui::Checkbox("Apply SMOTE", &bApplySMOTE);
                    Utils::AddSimpleTooltip("SMOTE: Synthetic Minority Oversampling Technique, if your categories "
                        "are highly imbalanced, you should apply SMOTE to handle it! (Undev yet)");

                    ImGui::TreePop();
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild("Summary&Preview", HalfWindowSize);
            {
                ImGui::PushFont(Setting::Get().TitleFont);
                ImGui::Text("Summary & Preview");
                ImGui::PopFont();
                const ImVec2 FrameSize = ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 9);
                ImGui::BeginChildFrame(ImGui::GetID("ExportSummary"), FrameSize, ImGuiWindowFlags_NoMove);
                ImGui::BulletText("Total images to export: %d", TotalExportImages);
                ImGui::BulletText("Train / Test / Valid split: %.0f : %.0f : %.0f", SplitPercent[0], SplitPercent[1], SplitPercent[2]);
                ImGui::BulletText("Resize to: %d x %d", NewSize[0], NewSize[1]);
                ImGui::BulletText("Augmentation: %d", NumIncludedAnnotation);
                ImGui::Indent();
                if (bApplyBlur) ImGui::Text("Random blur up to %d", MaxBlurAmount);
                if (bApplyNoise) ImGui::Text("Random noise (mean: %d ~ %d ,std: %d ~ %d.", NoiseMeanMin, NoiseMeanMax, NoiseStdMin, NoiseStdMax);
                if (bApplyHue) ImGui::Text("Random Hue shift for -%d ~ %d", HueAmount, HueAmount);
                if (bApplyFlipX) ImGui::Text("%d %% images will flip in x axis.", FlipXPercent);
                if (bApplyFlipY) ImGui::Text("%d %% images will flip in y axis.", FlipYPercent); 
                if (bApplyRotation) ImGui::Text("Random rotation -%d ~ %d", MaxRotationAmount, MaxRotationAmount);
                ImGui::Unindent();
                ImGui::BulletText("Category: Undex yet...");
                ImGui::Separator();
                ImGui::Text("About to generate... the final export info");
                ImGui::EndChildFrame();
                if (ImGui::TreeNode("Augmentation Preview"))
                {
                    if (ImGui::Button((std::string(ICON_FA_SYNC) + " Update augementation preview").c_str()))
                    {
                        UpdatePreviewAugmentations();
                    }
                    ImGui::Separator();
                    ImGui::Text("Original");
                    ImVec2 PreviewImgSize(128, 128);
                    ImGui::Image((void*)(intptr_t)Origin, PreviewImgSize);
                    ImGui::Text("Variants");
                    ImGuiStyle& style = ImGui::GetStyle();
                    int PreviewCount = 10;
                    float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                    float LastImage_x2, NextImage_x2;
                    MAKE_PREVIEW_IMAGE_VAR(0)
                    MAKE_PREVIEW_IMAGE_VAR(1)
                    MAKE_PREVIEW_IMAGE_VAR(2)
                    MAKE_PREVIEW_IMAGE_VAR(3)
                    MAKE_PREVIEW_IMAGE_VAR(4)
                    MAKE_PREVIEW_IMAGE_VAR(5)
                    MAKE_PREVIEW_IMAGE_VAR(6)
                    MAKE_PREVIEW_IMAGE_VAR(7)
                    MAKE_PREVIEW_IMAGE_VAR(8)
                    MAKE_PREVIEW_IMAGE_VAR(9)
                    ImGui::TreePop();
                }
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ExportWidgetGropWidth) * 0.5f);
        ImGui::BeginGroup();
        {
            ImGui::Text("New training set name: ");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120.f);
            ImGui::InputText("##Training Set Name", NewTrainingSetName, IM_ARRAYSIZE(NewTrainingSetName));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60.f);
            ImGui::SliderInt("Duplicate Times", &DuplicateTimes, 1, 100);
            ImGui::SameLine();
            if (ImGui::Button("Generate"))
            {
                GenerateTrainingSet();
            }
        }
        ImGui::EndGroup();
        ExportWidgetGropWidth = ImGui::GetItemRectSize().x;
    }

    void TrainingSetGenerator::GenerateTrainingSet()
    {
        // create folders and description files


        // random choose valid, train, test based on options

        // implement resize and export them

        // Should always keep origin version
        // generate augmentation duplicates
        // for each image name...
    }

    void TrainingSetGenerator::UpdatePreviewAugmentations()
    {
        cv::Mat Lena = cv::imread("Resources/Lena.png"); // should be BGR by default?
        cv::resize(Lena, Lena, cv::Size(128, 128)); // origin is 512 * 512
        cv::cvtColor(Lena, Lena, cv::COLOR_BGR2RGBA);
        // TODO: maybe I can come up with some good idea to prevent reload Lena for Origin
        glGenTextures(1, &Origin);
        glBindTexture(GL_TEXTURE_2D, Origin);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); // Same
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Lena.cols, Lena.rows, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, Lena.data);

        MAKE_GUI_IMG(0)
        MAKE_GUI_IMG(1)
        MAKE_GUI_IMG(2)
        MAKE_GUI_IMG(3)
        MAKE_GUI_IMG(4)
        MAKE_GUI_IMG(5)
        MAKE_GUI_IMG(6)
        MAKE_GUI_IMG(7)
        MAKE_GUI_IMG(8)
        MAKE_GUI_IMG(9)
    }

    cv::Mat TrainingSetGenerator::GenerateAugentationImage(cv::Mat InMat)
    {
        cv::Mat Img = InMat;

        // Gaussuain blur
        if (bApplyBlur)
        {
            cv::Mat BlurImg;
            int RandBlurAmount = Utils::RandomIntInRange(1, MaxBlurAmount);
            if (RandBlurAmount % 2 == 0)
                RandBlurAmount -= 1;
            cv::GaussianBlur(Img, BlurImg, cv::Size(RandBlurAmount, RandBlurAmount), 5, 0);
            Img = BlurImg;
        }
        // Random noise
        if (bApplyNoise)
        {
            cv::Mat Noise(Img.size(), Img.type());
            // float m = (10,12,34);
            // TODO: how to choose mean std?
            int RandMean = Utils::RandomIntInRange(NoiseMeanMin, NoiseMeanMax);
            int RandStd = Utils::RandomIntInRange(NoiseStdMin, NoiseStdMax);
            cv::randn(Noise, RandMean, RandStd);
            Img += Noise;
        }
        // hue adjustment
        if (bApplyHue)
        {
            // TODO: hue shift is wrong!!
            const int AdjAmount = Utils::RandomIntInRange(HueAmount * -1, HueAmount);
            cv::Mat HSVImg;
            cv::cvtColor(Img, HSVImg, cv::COLOR_RGBA2BGR);
            cv::cvtColor(HSVImg, HSVImg, cv::COLOR_BGR2HSV);
            for (int y = 0; y < HSVImg.rows; y++)
            {
                for (int x = 0; x < HSVImg.cols; x++)
                {
                    Img.at<cv::Vec3b>(y, x)[0] = cv::saturate_cast<uchar>(Img.at<cv::Vec3b>(y, x)[0] + AdjAmount);
                }
            }
            cv::cvtColor(HSVImg, Img, cv::COLOR_HSV2BGR);
            cv::cvtColor(Img, Img, cv::COLOR_BGR2RGBA);
        }
        // TODO: these operation will also change annotation positioning...
        if (bApplyFlipX)
        {
            if (FlipXPercent >= Utils::RandomIntInRange(0, 100))
            {
                cv::flip(Img, Img, 0);
            }
        }
        if (bApplyFlipY)
        {
            if (FlipYPercent >= Utils::RandomIntInRange(0, 100))
            {
                cv::flip(Img, Img, 1);
            }
        }
        if (bApplyRotation)
        {
            int RotationAmount = Utils::RandomIntInRange(-MaxRotationAmount, MaxRotationAmount);
            cv::Mat ForRotation = cv::getRotationMatrix2D(cv::Point(Img.rows / 2, Img.cols / 2), RotationAmount, 1);
            cv::warpAffine(Img, Img, ForRotation, Img.size());
        }
        return Img;
    }

    void TrainingSetGenerator::IncludeGenClip(const std::string& InClip)
    {
        IncludedGenClips.insert(InClip);
        UpdateExportInfo();
    }

    // TODO: skip for now...
    void TrainingSetGenerator::IncludeGenFolder(const std::string& InFolder)
    {
    }

    void TrainingSetGenerator::UpdateExportInfo()
    {
        YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Annotation.yaml"));
        NumIncludedFrames = 0;
        NumIncludedAnnotation = 0;
        for (const auto& C : IncludedGenClips)
        {
            NumIncludedFrames += Data[C].size();
            for (YAML::const_iterator it=Data[C].begin(); it!=Data[C].end(); ++it)
                NumIncludedAnnotation += it->second.size();
        }
        // TODO: add images code here...
        TotalExportImages = NumIncludedFrames + NumIncludedImages;
    }

    void TrainingSetGenerator::DrawSplitWidget()
    {
        ImGuiWindow* Win = ImGui::GetCurrentWindow();
        ImVec2 CurrentPos = ImGui::GetCursorScreenPos();
        ImU32 Color = ImGui::ColorConvertFloat4ToU32(Spectrum::BLUE(600, Setting::Get().Theme == ETheme::Light));
        const float AvailWidth = ImGui::GetContentRegionAvail().x * 0.95f;
        ImVec2 RectStart = CurrentPos + ImVec2(0, 17);
        ImVec2 RectEnd = CurrentPos + ImVec2(AvailWidth, 22);
        Win->DrawList->AddRectFilled(RectStart, RectEnd, Color, 20.f);
        const ImVec2 ControlPos1 = CurrentPos + ImVec2(SplitControlPos1 * AvailWidth - 6.f, 5);
        Win->DrawList->AddRectFilled(ControlPos1, ControlPos1 + ImVec2(12.f, 25), Color, 6.f);
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
            SplitImgs[2] = int(SplitPercent[2] * 0.01f * TotalExportImages);
        }
        const ImVec2 ControlPos2 = CurrentPos + ImVec2(SplitControlPos2 * AvailWidth - 6.f, 5);
        Win->DrawList->AddRectFilled(ControlPos2, ControlPos2 + ImVec2(12.f, 25), Color, 6.f);
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
            SplitImgs[2] = int(SplitPercent[2] * 0.01f * TotalExportImages);
        }
        // Add text in middle of the control
        float TxtWidth1 = ImGui::CalcTextSize("Train").x;
        float TxtWidth2 = ImGui::CalcTextSize("Test").x;
        float TxtWidth3 = ImGui::CalcTextSize("Valid").x;
        const float TxtPosY = 35.f;
        ImVec2 TxtPos1 = CurrentPos + ImVec2((SplitControlPos1 * AvailWidth - TxtWidth1) * 0.5f, TxtPosY);
        ImVec2 TxtPos2 = CurrentPos + ImVec2(
            (SplitControlPos1 + (SplitControlPos2 - SplitControlPos1) * 0.5f) * AvailWidth - TxtWidth2 * 0.5f, TxtPosY);
        ImVec2 TxtPos3 = CurrentPos + ImVec2((SplitControlPos2 + (1.f - SplitControlPos2) * 0.5f) * AvailWidth - TxtWidth3 * 0.5f,
                                             TxtPosY);
        Win->DrawList->AddText(TxtPos1, Color, "Train");
        Win->DrawList->AddText(TxtPos2, Color, "Test");
        Win->DrawList->AddText(TxtPos3, Color, "Valid");
        ImGui::Dummy({0, 50});

        // draw the percent
        const float CachedSplitPercent[3] = {SplitPercent[0], SplitPercent[1], SplitPercent[2]};
        if (ImGui::InputFloat3("Train/Test/Valid (%)", SplitPercent, "%.0f%%"))
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
        if (ImGui::InputInt3("Train/Test/Valid (Images)", SplitImgs))
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
}
