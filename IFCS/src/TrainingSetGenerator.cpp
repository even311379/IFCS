#include "TrainingSetGenerator.h"
#include "DataBrowser.h"
#include "Setting.h"

#include "imgui_impl_glfw.h"
#include <GLFW/glfw3.h>
#include "ImguiNotify/font_awesome_5.h"
#include "Implot/implot.h"

#include "opencv2/opencv.hpp"


#define MAKE_PREVIEW_IMAGE_VAR(n)\
    ImGui::PushID(n); \
    ImGui::Image((void*)(intptr_t)Var##n, PreviewImgSize); \
    LastImage_x2 = ImGui::GetItemRectMax().x; \
    NextImage_x2 = LastImage_x2 + style.ItemSpacing.x + PreviewImgSize.x; \
    if (n + 1 < buttons_count && NextImage_x2 < window_visible_x2) \
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
                    ImGui::BeginChildFrame(ImGui::GetID("Included Contents"), FrameSize, ImGuiWindowFlags_NoMove);
                    ImGui::EndChildFrame();
                    if (ImGui::BeginDragDropTarget()) // for the previous item? 
                    {
                        if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("FolderOrClip"))
                        {
                            spdlog::info("drop something??");
                        }
                    }
                    char AddClipPreviewTitle[128];
                    snprintf(AddClipPreviewTitle, sizeof(AddClipPreviewTitle), "%s Add Clip", ICON_FA_PLUS);
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
                    if (ImGui::BeginCombo("##AddClip", AddClipPreviewTitle))
                    {
                        std::vector<std::string> AllClips = DataBrowser::Get().GetAllClips();
                        auto NewEnd = std::remove_if(AllClips.begin(), AllClips.end(), [this](const std::string& C )
                        {
                            return std::find(IncludedGenClips.begin(), IncludedGenClips.end(), C) != IncludedGenClips.end();
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
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
                    if (ImGui::BeginCombo("##AddFolder", AddFolderPreviewTitle))
                    {

                        std::vector<std::string> AllFolders = DataBrowser::Get().GetAllFolders();
                        auto NewEnd = std::remove_if(AllFolders.begin(), AllFolders.end(), [this](const std::string& C )
                        {
                            return std::find(IncludedGenFolders.begin(), IncludedGenFolders.end(), C) != IncludedGenFolders.end();
                        });
                        AllFolders.erase(NewEnd, AllFolders.end());
                        for (size_t i = 0; i < AllFolders.size(); i++)
                        {
                            if (ImGui::Selectable(AllFolders[i].c_str(), false)) // no need to show selected
                            {
                                IncludeGenFolder(AllFolders[i]);
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::BulletText("Included Clips: %d", (int)IncludedGenClips.size());
                    ImGui::BulletText("Included frames: %d", NumIncludedFrames);
                    ImGui::BulletText("Included Images: %d", NumIncludedImages);

                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Train / Test Split"))
                {
                    ImGui::SetNextItemWidth(60.f);
                    if (ImGui::DragInt("Train", &TrainSplit, 1, 0, 100, "%d %"))
                    {
                        spdlog::info("train change is triggered...");
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(60.f);
                    if (ImGui::DragInt("Test", &TestSplit, 1, 0, 100, "%d %"))
                    {
                        spdlog::info("test change is triggered...");
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(60.f);
                    if (ImGui::DragInt("Valid", &ValidSplit, 1, 0, 100, "%d %"))
                    {
                        spdlog::info("valid change is triggered...");
                    }
                    ImGui::Text("Again... summary text");
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Resize"))
                {
                    ImGui::Text("Add crop and pan???");
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
                            ImGui::SliderInt("Max Random Rotation degree Amount", &RotationAmount, 1, 15, "%d deg");
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
                int buttons_count = 20;
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
            cv::Mat ForRotation = cv::getRotationMatrix2D(cv::Point(Img.rows / 2, Img.cols / 2), RotationAmount, 1);
            cv::warpAffine(Img, Img, ForRotation, Img.size());
        }
        return Img;
    }

    void TrainingSetGenerator::IncludeGenClip(std::string InClip)
    {
    }

    void TrainingSetGenerator::IncludeGenFolder(std::string InFolder)
    {
    }
}
