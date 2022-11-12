#include "TrainingSetGenerator.h"

#include <fstream>
#include <numeric>
#include <random>

#include "DataBrowser.h"
#include "Setting.h"
#include "Style.h"

#include "backends/imgui_impl_glfw.h"
#include <GLFW/glfw3.h>
#include <yaml-cpp/node/node.h>

#include "Annotation.h"
#include "CategoryManagement.h"
#include "imgui_internal.h"
#include "ImguiNotify/font_awesome_5.h"
#include "Implot/implot.h"

#include "opencv2/opencv.hpp"
#include "yaml-cpp/yaml.h"


#define MAKE_PREVIEW_IMAGE_VAR(n)\
    ImGui::PushID(n); \
    ImGui::Image((void*)(intptr_t)Var##n, PreviewImgSize); \
    LastImage_x2 = ImGui::GetItemRectMax().x; \
    NextImage_x2 = LastImage_x2 + style.ItemSpacing.x + PreviewImgSize.x; \
    if (n + 1 < PreviewCount && NextImage_x2 < window_visible_x2) \
        ImGui::SameLine(); \
        ImGui::PopID();

#define MAKE_GUI_IMG(n, f)\
    cv::Mat Mat##n = GenerateAugentationImage(Lena, ##f);\
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
        ImGui::PushFont(Setting::Get().TitleFont);
        ImGui::Text("Options");
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
        if (ImGui::TreeNode("Augmentation"))
        {
            RenderAugmentationWidget();
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
        RenderExportWidget();

        ImGui::Unindent();
    }

    void TrainingSetGenerator::SyncCategoyData(std::vector<UUID> InCatIDs)
    {
        for (UUID UID : InCatIDs)
        {
            if (!CategoriesChecker.count(UID))
            {
                CategoriesChecker[UID] = true;
            }
        }
        UpdateExportInfo();
    }

    // TODO: should be perfect, but test is still needed...
    void TrainingSetGenerator::GenerateTrainingSet()
    {
        if (std::strlen(NewTrainingSetName) == 0) return;
        // create folders and description files
        const std::string ProjectPath = Setting::Get().ProjectPath;
        std::filesystem::create_directories(ProjectPath + "/Data/" + NewTrainingSetName);
        std::string SplitName[3] = {"train", "valid", "test"};
        std::string TypeName[2] = {"images", "labels"};
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
            IC.assign(IncludedGenClips.begin(), IncludedGenClips.end());
            Desc.IncludeClips = IC;
            std::vector<std::string> II;
            II.assign(IncludedImageFolders.begin(), IncludedImageFolders.end());
            Desc.IncludeImageFolders = II;
            Desc.Size[0] = NewSize[0];
            Desc.Size[1] = NewSize[1];
            Desc.Split[0] = SplitPercent[0] / 100;
            Desc.Split[1] = SplitPercent[1] / 100;
            Desc.Split[2] = SplitPercent[2] / 100;
            Desc.NumDuplicates = AugmentationDuplicates;
            Desc.TotalImagesExported = TotalExportImages;
            Desc.AppliedAugmentationDescription = MakeAugmentationDescription();
            Data.push_back(Desc.Serialize());
            YAML::Emitter Out;
            Out << Data;
            ofs << Out.c_str();
        }
        ofs.close();

        //write data file for yolo
        ofs.open(ProjectPath + "/Data/" + NewTrainingSetName + "/" + NewTrainingSetName + ".yaml");
        if (ofs.is_open())
        {
            ofs << "train: " << ProjectPath + "/Data/" + NewTrainingSetName + "/train/images" << std::endl;
            ofs << "val: " << ProjectPath + "/Data/" + NewTrainingSetName + "/valid/images" << std::endl;
            ofs << "test: " << ProjectPath + "/Data/" + NewTrainingSetName + "/test/images" << std::endl << std::endl;
            size_t S = CategoriesToExport.size();
            ofs << "nc: " << S << std::endl;
            ofs << "names: [";
            size_t i = 0;
            for (const auto& [Cat, N] : CategoriesToExport)
            {
                ofs << "'" << Cat << "'";
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
        for (const std::string& Clip : IncludedGenClips)
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
                // should check category in each frame
                for (YAML::const_iterator A = Node.begin(); A != Node.end(); ++A)
                {
                    if (CategoriesChecker[A->second["CategoryID"].as<uint64_t>()])
                        Annotations.emplace_back(A->second);
                }
                if (Annotations.empty()) continue;

                if (std::find(TrainingIdx.begin(), TrainingIdx.end(), N) != TrainingIdx.end())
                {
                    // gen original version
                    GenerateImgTxt(Cap, FrameNum, Annotations, GenName.c_str(), true, "Train");

                    // gen augment version
                    for (int i = 0; i < AugmentationDuplicates - 1; i++)
                    {
                        GenerateImgTxt(Cap, FrameNum, Annotations, (GenName + "_aug_" + std::to_string(i)).c_str(),
                                       false, "Train");
                    }
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

    // TODO: for clip only...
    void TrainingSetGenerator::GenerateImgTxt(cv::VideoCapture& Cap, int FrameNum,
                                              const std::vector<FAnnotation>& InAnnotations, const char* GenName,
                                              bool IsOriginal, const char* SplitName)
    {
        const std::string OutImgName = Setting::Get().ProjectPath + "/Data/" + NewTrainingSetName + "/" + SplitName +
            "/images/" + GenName + ".jpg";
        const std::string OutTxtName = Setting::Get().ProjectPath + "/Data/" + NewTrainingSetName + "/" + SplitName +
            "/labels/" + GenName + ".txt";
        cv::Mat Frame;
        Cap.set(cv::CAP_PROP_POS_FRAMES, FrameNum);
        Cap >> Frame;
        cv::resize(Frame, Frame, cv::Size(NewSize[0], NewSize[1]));
        cv::cvtColor(Frame, Frame, cv::COLOR_BGR2RGB);
        FAnnotationShiftData ShiftData;
        if (!IsOriginal)
        {
            Frame = GenerateAugentationImage(Frame, ShiftData);
        }
        cv::imwrite(OutImgName, Frame);
        std::ofstream ofs;
        ofs.open(OutTxtName);
        if (ofs.is_open())
        {
            for (auto& A : InAnnotations)
            {
                ofs << A.GetExportTxt(CategoryExportID[A.CategoryID], ShiftData).c_str() << std::endl;
            }
        }
        ofs.close();
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
        FAnnotationShiftData ASD;
        MAKE_GUI_IMG(0, ASD)
        MAKE_GUI_IMG(1, ASD)
        MAKE_GUI_IMG(2, ASD)
        MAKE_GUI_IMG(3, ASD)
        MAKE_GUI_IMG(4, ASD)
        MAKE_GUI_IMG(5, ASD)
        MAKE_GUI_IMG(6, ASD)
        MAKE_GUI_IMG(7, ASD)
        MAKE_GUI_IMG(8, ASD)
    }

    cv::Mat TrainingSetGenerator::GenerateAugentationImage(cv::Mat InMat, FAnnotationShiftData& OutShift)
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
                OutShift.IsFlipX = true;
            }
        }
        if (bApplyFlipY)
        {
            if (FlipYPercent >= Utils::RandomIntInRange(0, 100))
            {
                cv::flip(Img, Img, 1);
                OutShift.IsFlipY = true;
            }
        }
        if (bApplyRotation)
        {
            int RotationAmount = Utils::RandomIntInRange(-MaxRotationAmount, MaxRotationAmount);
            OutShift.RotationDegree = RotationAmount;
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
        YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/Annotations.yaml");
        auto CatData = CategoryManagement::Get().Data;
        CategoriesToExport.clear();
        CategoryExportID.clear();
        int i = 0;
        for (const auto& [UID, Check] : CategoriesChecker)
        {
            if (Check)
            {
                CategoriesToExport[CatData[UID].DisplayName] = 0;
                CategoryExportID[UID] = i;
                i++;
            }
        }
        NumIncludedFrames = 0;
        NumIncludedAnnotation = 0;
        for (const auto& C : IncludedGenClips)
        {
            for (YAML::const_iterator Frame = Data[C].begin(); Frame != Data[C].end(); ++Frame)
            {
                YAML::Node ANode = Frame->second.as<YAML::Node>();
                bool ContainsExportedCategory = false;
                for (YAML::const_iterator A = ANode.begin(); A != ANode.end(); ++A)
                {
                    UUID CID = A->second["CategoryID"].as<uint64_t>();
                    if (CategoriesChecker[CID])
                    {
                        CategoriesToExport[CatData[CID].DisplayName] += 1;
                        NumIncludedAnnotation++;
                        ContainsExportedCategory = true;
                    }
                }
                if (ContainsExportedCategory) NumIncludedFrames += 1;
            }
        }
        TotalExportImages = NumIncludedFrames + NumIncludedImages;
        SplitImgs[0] = int(SplitPercent[0] * 0.01f * TotalExportImages);
        SplitImgs[1] = int(SplitPercent[1] * 0.01f * TotalExportImages);
        SplitImgs[2] = TotalExportImages - SplitImgs[0] - SplitImgs[1]; // to prevent lacking due to round off...
    }

    void TrainingSetGenerator::RenderDataSelectWidget()
    {
        ImGui::Text("Select which folders / clips to include in this training set:");
        const ImVec2 FrameSize = ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 9);
        float HalfWidth = ImGui::GetContentRegionAvail().x * 0.5f;
        const size_t RelativeClipNameOffset = Setting::Get().ProjectPath.size() + 7;
        ImGui::BeginChildFrame(ImGui::GetID("Included Contents"), FrameSize, ImGuiWindowFlags_NoMove);
        for (const std::string& Clip : IncludedGenClips)
        {
            std::string s = Clip.substr(RelativeClipNameOffset);
            ImGui::Text(s.c_str());
        }
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
        sprintf(AddClipPreviewTitle, "%s Add Clip", ICON_FA_PLUS);
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
                if (ImGui::Selectable(AllClips[i].substr(RelativeClipNameOffset).c_str(), false)) // no need to show selected
                {
                    IncludeGenClip(AllClips[i]);
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
            ImGui::Selectable("Not supported yet...");
            // TODO: leave include folder for now...
            /*std::vector<std::string> AllFolders = DataBrowser::Get().GetAllFolders();
            auto NewEnd = std::remove_if(AllFolders.begin(), AllFolders.end(), [this](const std::string& C)
            {
                return std::find(IncludedImageFolders.begin(), IncludedImageFolders.end(), C) !=
                    IncludedImageFolders.end();
            });
            AllFolders.erase(NewEnd, AllFolders.end());
            for (size_t i = 0; i < AllFolders.size(); i++)
            {
                if (ImGui::Selectable(AllFolders[i].c_str(), false)) // no need to show selected
                {
                    IncludeGenFolder(AllFolders[i]);
                }
            }*/
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
    }

    void TrainingSetGenerator::RenderResizeWidget()
    {
        // Just use cv::linear to interpolate... no need to expose other options...
        static const char* AspectRatioOptions[] = {"16x9", "4x3", "Custom"};
        if (ImGui::Combo("Aspect Ratio", &SelectedResizeAspectRatio, AspectRatioOptions,
                         IM_ARRAYSIZE(AspectRatioOptions)))
        {
            switch (SelectedResizeAspectRatio)
            {
            case 0: // 16x9
                NewSize[1] = NewSize[0] * 9 / 16;
                break;
            case 1: // 4x3
                NewSize[1] = NewSize[0] * 3 / 4;
                break;
            default: break;
            }
        }
        int OldSize[2] = {NewSize[0], NewSize[1]};
        if (ImGui::InputInt2("New Size", NewSize))
        {
            const bool OtherIdx = OldSize[0] != NewSize[0];
            const bool ChangedIdx = !OtherIdx;
            switch (SelectedResizeAspectRatio)
            {
            case 0: // 16x9
                NewSize[OtherIdx] = ChangedIdx ? NewSize[ChangedIdx] * 16 / 9 : NewSize[ChangedIdx] * 9 / 16;
                break;
            case 1: // 4x3
                NewSize[OtherIdx] = ChangedIdx ? NewSize[ChangedIdx] * 4 / 3 : NewSize[ChangedIdx] * 3 / 4;
                break;
            default: break;
            }
        }
    }

    void TrainingSetGenerator::RenderCategoryWidget()
    {
        auto CatData = CategoryManagement::Get().Data;
        for (auto& [UID, Should] : CategoriesChecker)
        {
            if (ImGui::Checkbox(CatData[UID].DisplayName.c_str(), &Should))
            {
                UpdateExportInfo();
            }
        }

        if (ImGui::Button("Add all catogories"))
        {
            for (auto& [UID, Should] : CategoriesChecker)
            {
                Should = true;
            }
            UpdateExportInfo();
        }

        const size_t NumBars = CategoriesToExport.size();
        if (NumBars > 1)
        {
            std::vector<ImVec4> Colors;
            std::vector<std::vector<ImU16>> BarData;
            std::vector<const char*> CatNames;
            std::vector<double> Positions;
            size_t i = 0;
            for (const auto& [Cat, N] : CategoriesToExport)
            {
                std::vector<ImU16> Temp(NumBars, 0);
                Temp[i] = (ImU16)N;
                CatNames.emplace_back(Cat.c_str());
                BarData.emplace_back(Temp);
                Colors.emplace_back(CategoryManagement::Get().GetColorFrameDisplayName(Cat));
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
                // TODO: is the order important? So hard to match so far...
                for (size_t j = 0; j < NumBars; j++)
                    ImPlot::PlotBars(CatNames[j], BarData[j].data(), (int)NumBars, 0.5, 1,
                                     ImPlotBarGroupsFlags_Horizontal);
                ImPlot::EndPlot();
            }
            ImPlot::PopColormap();
            ImGui::TextWrapped("HINT! Imbalanced data may result in bad model! You should prevent it! ... somehow...");
        }
        // CategoryManagement::Get().DrawSummaryBar(IncludedID);
        // ImGui::Checkbox("Apply SMOTE", &bApplySMOTE);
        // Utils::AddSimpleTooltip("SMOTE: Synthetic Minority Oversampling Technique, if your categories "
        //     "are highly imbalanced, you should apply SMOTE to handle it! (Undev yet)");
    }

    void TrainingSetGenerator::RenderAugmentationWidget()
    {
        ImGui::Checkbox("Should apply image augmentation?", &bApplyImageAugmentation);
        if (!bApplyImageAugmentation)
            return;

        if (ImGui::InputInt("Augmentation Duplicates", &AugmentationDuplicates, 1, 5))
        {
            if (AugmentationDuplicates < 1) AugmentationDuplicates = 1;
            if (AugmentationDuplicates > 100) AugmentationDuplicates = 100;
        }
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
                ImGui::DragIntRange2("Noise Mean", &NoiseMeanMin, &NoiseMeanMax, 1, 0, 50);
                ImGui::DragIntRange2("Noise Std", &NoiseStdMin, &NoiseStdMax, 1, 0, 50);
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Hue"))
        {
            ImGui::Checkbox("Apply Hue Shift", &bApplyHue);
            if (bApplyHue)
            {
                ImGui::DragInt("Max Random Hue (+ -)", &HueAmount, 1, 1, 60, "%d %");
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Rotation"))
        {
            ImGui::Checkbox("Add random rotation?", &bApplyRotation);
            if (bApplyRotation)
            {
                ImGui::DragInt("Max Random Rotation degree Amount", &MaxRotationAmount, 1, 1, 15, "%d deg");
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
        if (ImGui::TreeNode("Preview"))
        {
            if (ImGui::Button((std::string(ICON_FA_SYNC) + " Update augementation preview").c_str()))
            {
                UpdatePreviewAugmentations();
            }
            ImGui::SameLine();
            // TODO: allow to change aug preview image...
            static const char* PreviewImageFile = "Lena.png";
            ImGui::Button("Change Preview Image?");
            ImGui::SameLine(0, 20);
            ImGui::Text("(Current: %s)", PreviewImageFile);
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
            ImGui::TreePop();
        }
    }

    void TrainingSetGenerator::RenderSummary()
    {
        const ImVec2 FrameSize = ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 9);
        ImGui::BeginChildFrame(ImGui::GetID("ExportSummary"), FrameSize, ImGuiWindowFlags_NoMove);
        ImGui::BulletText("Total images to export: %d", TotalExportImages);
        ImGui::BulletText("Train / Valid / Test split: %.0f : %.0f : %.0f", SplitPercent[0], SplitPercent[1],
                          SplitPercent[2]);
        ImGui::BulletText("Resize to: %d x %d", NewSize[0], NewSize[1]);
        ImGui::BulletText("Augmentation:");
        ImGui::Indent();
        ImGui::Text("%s", MakeAugmentationDescription().c_str());

        ImGui::Unindent();
        ImGui::BulletText("Category: Undex yet...");
        ImGui::Separator();
        ImGui::Text("Total Generation: %d  =  %d x (1 + %d) + %d + %d",
                    SplitImgs[0] * (1 + AugmentationDuplicates) + SplitImgs[1] + SplitImgs[2], SplitImgs[0],
                    AugmentationDuplicates,
                    SplitImgs[1], SplitImgs[2]);
        ImGui::EndChildFrame();
    }

    void TrainingSetGenerator::RenderExportWidget()
    {
        ImGui::SetNextItemWidth(240.f);
        ImGui::InputText("Training Set Name", NewTrainingSetName, IM_ARRAYSIZE(NewTrainingSetName));
        ImGui::SameLine();
        if (ImGui::Button("Generate", ImVec2(-1, 0)))
        {
            GenerateTrainingSet();
        }
    }

    void TrainingSetGenerator::RenderSplitWidget()
    {
        ImGuiWindow* Win = ImGui::GetCurrentWindow();
        ImVec2 CurrentPos = ImGui::GetCursorScreenPos();
        ImU32 Color = ImGui::ColorConvertFloat4ToU32(Style::BLUE(600, Setting::Get().Theme));
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
            SplitImgs[2] = TotalExportImages - SplitImgs[0] - SplitImgs[1];
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
        Win->DrawList->AddText(TxtPos1, Color, "Train");
        Win->DrawList->AddText(TxtPos2, Color, "Valid");
        Win->DrawList->AddText(TxtPos3, Color, "Test");
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

    std::string TrainingSetGenerator::MakeAugmentationDescription()
    {
        std::string Out;
        if (bApplyBlur)
        {
            char buf[64];
            sprintf(buf, "Random blur up to %d /n", MaxBlurAmount);
            Out += buf;
        }
        if (bApplyNoise)
        {
            char buf[64];
            sprintf(buf, "Random noise (mean: %d ~ %d ,std: %d ~ %d./n", NoiseMeanMin, NoiseMeanMax, NoiseStdMin,
                    NoiseStdMax);
            Out += buf;
        }
        if (bApplyHue)
        {
            char buf[64];
            sprintf(buf, "Random Hue shift for -%d ~ %d", HueAmount, HueAmount);
            Out += buf;
        }
        if (bApplyFlipX)
        {
            char buf[64];
            sprintf(buf, "%d %% images will flip in x axis.", FlipXPercent);
            Out += buf;
        }
        if (bApplyFlipY)
        {
            char buf[64];
            sprintf(buf, "%d %% images will flip in y axis.", FlipYPercent);
            Out += buf;
        }
        if (bApplyRotation)
        {
            char buf[64];
            sprintf(buf, "Random rotation -%d ~ %d", MaxRotationAmount, MaxRotationAmount);
            Out += buf;
        }
        return Out;
    }
}
