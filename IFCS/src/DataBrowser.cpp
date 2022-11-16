#include "DataBrowser.h"
#include "Log.h"
#include "Style.h"
#include "Utils.h"
#include "Setting.h"

#include "IconFontCppHeaders/IconsFontAwesome5.h"
#include <spdlog/spdlog.h>
#include <shellapi.h>
#include <yaml-cpp/yaml.h>
#include "opencv2/opencv.hpp"
#include "backends/imgui_impl_glfw.h"
#include <GLFW/glfw3.h>

#include "Annotation.h"


// TODO: opencv may crash sometimes when double click frame number? resize issue?

namespace IFCS
{
    void DataBrowser::Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose)
    {
        Panel::Setup(InName, InShouldOpen, InFlags, InCanClose);
        SelectedTrainingSet = "";
        SelectedModel = "";
        SelectedDetection = "";
        ShouldUpdateData = true;
    }

    std::vector<std::string> DataBrowser::GetAllClips() const
    {
        std::vector<std::string> Out;
        for (const auto& p : std::filesystem::recursive_directory_iterator(
                 Setting::Get().ProjectPath + std::string("/Clips")))
        {
            if (!std::filesystem::is_directory(p))
            {
                bool pass = false;
                for (const std::string& Format : AcceptedClipsFormat)
                {
                    if (Utils::InsensitiveStringCompare(p.path().extension().string(), Format))
                    {
                        pass = true;
                        break;
                    }
                }
                if (pass)
                {
                    std::string full_path = p.path().u8string();
                    std::replace(full_path.begin(), full_path.end(), '\\', '/');
                    Out.push_back(full_path);
                }
            }
        }
        return Out;
    }

    std::vector<std::string> DataBrowser::GetAllImages() const
    {
        std::vector<std::string> Out;
        for (const auto& p : std::filesystem::recursive_directory_iterator(
                 Setting::Get().ProjectPath + std::string("/Images")))
        {
            if (!std::filesystem::is_directory(p))
            {
                bool pass = false;
                for (const std::string& Format : AcceptedImageFormat)
                {
                    if (Utils::InsensitiveStringCompare(p.path().extension().string(), Format))
                    {
                        pass = true;
                        break;
                    }
                }
                if (pass)
                {
                    std::string full_path = p.path().u8string();
                    std::replace(full_path.begin(), full_path.end(), '\\', '/');
                    Out.push_back(full_path);
                }
            }
        }
        return Out;
    }

    // TODO: finish this...
    std::vector<std::string> DataBrowser::GetAllFolders() const
    {
        std::vector<std::string> Out;
        return Out;
    }

    void DataBrowser::LoadVideoFrame(int FrameNumber)
    {
        MatToGL(VideoFrames[FrameNumber]);
    }

    void DataBrowser::RenderContent()
    {
        if (!Setting::Get().ProjectIsLoaded) return;
        // Inside child window to have independent scroll
        const std::string ClipFolderPath = Setting::Get().ProjectPath + "/Clips";
        const std::string ImageFolderPath = Setting::Get().ProjectPath + "/Images";
        ImGui::PushFont(Setting::Get().TitleFont);
        ImGui::Text("Data Browser");
        ImGui::PopFont();
        ImVec2 ChildWindowSize = ImGui::GetContentRegionAvail();
        if (IsViewingSomeDetail())
            ChildWindowSize.y *= 0.60f;
        else
            ChildWindowSize.y *= 0.97f;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("DataBrowserWindow", ChildWindowSize, false, window_flags);
        {
            if (ImGui::TreeNodeEx((std::string(ICON_FA_VIDEO) + " Clips").c_str(),
                                  ImGuiTreeNodeFlags_DefaultOpen))
            {
                RecursiveClipTreeNodeGenerator(ClipFolderPath, 0);
                if (!HasAnyClip)
                {
                    if (ImGui::Button("Open Folder", ImVec2(-1, 0)))
                    {
                        ShellExecuteA(NULL, "open", (Setting::Get().ProjectPath + "/Clips").c_str(), NULL, NULL,
                                      SW_SHOWDEFAULT);
                    }
                }
                // TODO: add some hint to add things in content browser if nothing here...
                ImGui::Checkbox("Show not ready only?", &NeedReviewedOnly);

                ImGui::TreePop();
            }

            if (ImGui::TreeNode((std::string(ICON_FA_IMAGE) + " Images").c_str()))
            {
                RecursiveImageTreeNodeGenerator(ImageFolderPath, 0);
                if (!HasAnyImage)
                {
                    if (ImGui::Button("Open Folder", ImVec2(-1, 0)))
                    {
                        ShellExecuteA(NULL, "open", (Setting::Get().ProjectPath + "/Images").c_str(), NULL, NULL,
                                      SW_SHOWDEFAULT);
                    }
                }
                ImGui::TreePop();
            }
            if (ImGui::TreeNode((std::string(ICON_FA_TH) + " Training Sets").c_str()))
            {
                CreateSeletable_TrainingSets();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode((std::string(ICON_FA_MICROCHIP) + " Models").c_str()))
            {
                CreateSeletable_Models();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode((std::string(ICON_FA_WAVE_SQUARE) + " Predictions").c_str()))
            {
                CreateSeletable_Detections();
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();
        if (IsViewingSomeDetail())
        {
            RenderDetailWidget();
        }
        else
        {
            if (ImGui::Button(LOCTEXT("Common.OpenProjectFolder"), ImVec2(-1, 0)))
            {
                ShellExecuteA(NULL, "open", Setting::Get().ProjectPath.c_str(), NULL, NULL,
                              SW_SHOWDEFAULT);
            }
        }

    }


    // TODO: selected clip/image widget and

    // Copy and modify from https://discourse.dearimgui.org/t/how-to-mix-imgui-treenode-and-filesystem-to-print-the-current-directory-recursively/37
    void DataBrowser::RecursiveClipTreeNodeGenerator(const std::filesystem::path& Path, unsigned int Depth)
    {
        for (const auto& entry : std::filesystem::directory_iterator(Path))
        {
            if (std::filesystem::is_directory(entry.path()))
            {
                using namespace std::string_literals;
                std::string s = ICON_FA_FOLDER + " "s + entry.path().filename().u8string().c_str();
                if (ImGui::TreeNodeEx(s.c_str(), 0))
                {
                    RecursiveClipTreeNodeGenerator(entry, Depth + 1);
                    ImGui::TreePop();
                }
            }
            else
            {
                bool pass = false;
                for (const std::string& Format : AcceptedClipsFormat)
                {
                    if (Utils::InsensitiveStringCompare(entry.path().extension().string(), Format))
                    {
                        pass = true;
                        break;
                    }
                }
                if (!pass) return;
                HasAnyClip = true;
                if (Depth > 0)
                {
                    ImGui::Indent();
                }
                std::string ClipName = entry.path().filename().u8string();
                std::string FullClipPath = entry.path().u8string();
                std::replace(FullClipPath.begin(), FullClipPath.end(), '\\', '/');
                if (ImGui::Selectable(ClipName.c_str(), SelectedClipInfo.ClipPath == FullClipPath,
                                      ImGuiSelectableFlags_AllowDoubleClick))
                {
                    SelectedImageInfo.ImagePath = ""; // Deselect image...
                    SelectedClipInfo.ClipPath = FullClipPath;
                    if (Setting::Get().ActiveWorkspace == EWorkspace::Data)
                    {
                        Annotation::Get().PrepareVideo();
                    }
                    else // only update info
                    {
                        cv::VideoCapture cap(SelectedClipInfo.ClipPath);
                        int Width = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
                        int Height = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
                        float FPS = (float)cap.get(cv::CAP_PROP_FPS);
                        int FrameCount = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
                        SelectedClipInfo.Update(FrameCount, FPS, Width, Height);
                        cap.release();
                    }
                }
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    char Send[128];
                    strcpy(Send, FullClipPath.c_str());
                    ImGui::SetDragDropPayload("FolderOrClip", &Send, sizeof(Send));
                    // for test now
                    ImGui::Text(Send);
                    ImGui::EndDragDropSource();
                }
                if (Setting::Get().ActiveWorkspace == EWorkspace::Data && SelectedClipInfo.ClipPath == FullClipPath)
                {
                    ImGui::Indent();
                    ImGui::Indent();
                    for (const auto& [k, v] : Annotation::Get().GetAnnotationToDisplay())
                    {
                        if (NeedReviewedOnly && v.IsReady) continue;
                        char buff[128];
                        const char* Icon = v.IsReady? ICON_FA_CHECK : "";
                        snprintf(buff, sizeof(buff), "%d ...... (%d) %s", k, v.Count, Icon);
                        if (ImGui::Selectable(buff, k == Annotation::Get().CurrentFrame))
                        {
                            Annotation::Get().MoveFrame(k);
                        }
                    }
                    ImGui::Unindent();
                    ImGui::Unindent();
                }
                if (Depth > 0)
                {
                    ImGui::Unindent();
                }
            }
        }
    }

    // TODO: long long list of files will make it lag... need to fix? ... very very hard to fix... ignore it!
    void DataBrowser::RecursiveImageTreeNodeGenerator(const std::filesystem::path& Path, unsigned Depth)
    {
        for (const auto& entry : std::filesystem::directory_iterator(Path))
        {
            if (std::filesystem::is_directory(entry.path()))
            {
                using namespace std::string_literals;
                std::string s = ICON_FA_FOLDER + " "s + entry.path().filename().u8string().c_str();
                if (ImGui::TreeNodeEx(s.c_str(), 0))
                {
                    RecursiveImageTreeNodeGenerator(entry, Depth + 1);
                    ImGui::TreePop();
                }
            }
            else
            {
                bool pass = false;
                for (const std::string& Format : AcceptedImageFormat)
                {
                    if (Utils::InsensitiveStringCompare(entry.path().extension().string(), Format))
                    {
                        pass = true;
                        break;
                    }
                }
                if (!pass) return;
                HasAnyImage = true;
                if (Depth > 0)
                {
                    ImGui::Indent();
                }
                std::string ImageName = entry.path().filename().u8string();
                std::string FullImagePath = entry.path().u8string();
                std::replace(FullImagePath.begin(), FullImagePath.end(), '\\', '/');
                // TODO:: display how many annos in this image?
                if (ImGui::Selectable(ImageName.c_str(), SelectedImageInfo.ImagePath == FullImagePath,
                                      ImGuiSelectableFlags_AllowDoubleClick))
                {
                    SelectedClipInfo.ClipPath = "";
                    SelectedImageInfo.ImagePath = FullImagePath;
                    if (Setting::Get().ActiveWorkspace == EWorkspace::Data)
                    {
                        Annotation::Get().DisplayImage();
                    }
                    else // only update info...
                    {
                        cv::Mat Img = cv::imread(SelectedImageInfo.ImagePath);
                        SelectedImageInfo.Update(Img.cols, Img.rows);
                    }
                }
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    char Send[128];
                    strcpy(Send, FullImagePath.c_str());
                    ImGui::SetDragDropPayload("FolderOrClip", &Send, sizeof(Send));
                    // for test now
                    ImGui::Text(Send);
                    ImGui::EndDragDropSource();
                }
                if (Depth > 0)
                {
                    ImGui::Unindent();
                }
            }
        }
    }

    void DataBrowser::CreateSeletable_TrainingSets()
    {
        YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/TrainingSets.yaml");
        for (YAML::const_iterator it = Data.begin(); it != Data.end(); ++it)
        {
            std::string Name = it->first.as<std::string>();
            if (ImGui::Selectable(Name.c_str(), Name == SelectedTrainingSet))
            {
                SelectedTrainingSet = Name;
                TrainingSetDescription = FTrainingSetDescription(Name, it->second);
                SelectedModel = "";
                SelectedDetection = "";
            }
            // TODO: block the drag source for now...
            // if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            // {
            //     char Send[128];
            //     strcpy(Send, Name.c_str());
            //     ImGui::SetDragDropPayload("TrainingSet", &Send, sizeof(Send));
            //     ImGui::Text(Send);
            //     ImGui::EndDragDropSource();
            // }
        }
    }

    void DataBrowser::CreateSeletable_Models()
    {
        YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Models/Models.yaml");
        for (YAML::const_iterator it = Data.begin(); it != Data.end(); ++it)
        {
            std::string Name = it->first.as<std::string>();
            if (ImGui::Selectable(Name.c_str(), Name == SelectedModel))
            {
                SelectedTrainingSet = "";
                SelectedModel = Name;
                ModelDescription = FModelDescription(Name, it->second);
                SelectedDetection = "";
            }
            // TODO: drag source??
        }
    }

    void DataBrowser::CreateSeletable_Detections()
    {
        YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Detections/Detections.yaml");
        for (YAML::const_iterator it = Data.begin(); it != Data.end(); ++it)
        {
            std::string Name = it->first.as<std::string>();
            if (ImGui::Selectable(Name.c_str(), Name == SelectedDetection))
            {
                SelectedTrainingSet = "";
                SelectedModel = "";
                SelectedDetection = Name;
                DetectionDescription = FDetectionDescription(Name, it->second);
            }
            // TODO: drag source??
        }
    }

    void DataBrowser::RenderDetailWidget()
    {
        ImGui::BeginChild("AssetDetail");
        {
            float ContentWidth = ImGui::GetContentRegionAvail().x;
            ImVec2 Pos = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(Pos, Pos + ImVec2(ContentWidth, 5.f),
                                                      ImGui::ColorConvertFloat4ToU32(
                                                          Style::BLUE(400, Setting::Get().Theme)), 12.f);
            ImGui::Dummy(ImVec2(0, 2.f));
            if (!SelectedTrainingSet.empty())
            {
                ImGui::PushFont(Setting::Get().TitleFont);
                ImGui::Text("%s", TrainingSetDescription.Name.c_str());
                ImGui::PopFont();
            }
            else if (!SelectedModel.empty())
            {
                ImGui::PushFont(Setting::Get().TitleFont);
                ImGui::Text("%s", ModelDescription.Name.c_str());
                ImGui::PopFont();
            }
            else if (!SelectedDetection.empty())
            {
                ImGui::PushFont(Setting::Get().TitleFont);
                ImGui::Text("%s", DetectionDescription.Name.c_str());
                ImGui::PopFont();
            }
            ImGui::SameLine(ContentWidth - 32);
            if (ImGui::Button(ICON_FA_TIMES, ImVec2(32, 0)))
            {
                DeselectAll();
            }
            // content
            if (!SelectedTrainingSet.empty())
            {
                TrainingSetDescription.MakeDetailWidget();
            }
            if (!SelectedModel.empty())
            {
                ModelDescription.MakeDetailWidget();
            }
            if (!SelectedDetection.empty())
            {
                DetectionDescription.MakeDetailWidget();
            }
        }
        ImGui::EndChild();
    }

    void DataBrowser::MatToGL(const cv::Mat& Frame)
    {
        glGenTextures(1, &LoadedFramePtr);
        glBindTexture(GL_TEXTURE_2D, LoadedFramePtr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); // Same
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Frame.cols, Frame.rows, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, Frame.data);
    }


    ImVec2 DataBrowser::GetBtnSize()
    {
        return {ImGui::GetFont()->FontSize * 3, ImGui::GetFont()->FontSize * 1.5f};
    }

    bool DataBrowser::IsViewingSomeDetail() const
    {
        if (!SelectedTrainingSet.empty()) return true;
        if (!SelectedModel.empty()) return true;
        if (!SelectedDetection.empty()) return true;
        return false;
    }

    void DataBrowser::DeselectAll()
    {
        // SelectedFrame = -1;
        SelectedTrainingSet = "";
        SelectedModel = "";
        SelectedDetection = "";
    }
}
