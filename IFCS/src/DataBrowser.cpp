#include "DataBrowser.h"

#include "Setting.h"

#include "ImguiNotify/font_awesome_5.h"
#include <spdlog/spdlog.h>
#include <shellapi.h>
#include <yaml-cpp/yaml.h>
#include "opencv2/opencv.hpp"
#include "backends/imgui_impl_glfw.h"
#include <GLFW/glfw3.h>

#include "Annotation.h"
#include "FrameExtractor.h"
#include "Log.h"
#include "Style.h"
#include "Utils.h"

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

    void DataBrowser::LoadOtherFrame(bool IsNext)
    {
  
        int NewFrame = -1;
        for (auto it=FramesData.begin();it!=FramesData.end();++it)
        {
            if (it->first == SelectedFrame)
            {
                if (it==FramesData.begin() && !IsNext) return;
                if (it==std::prev(FramesData.end()) && IsNext) return;
                if (IsNext)
                    NewFrame = std::next(it)->first;
                else
                    NewFrame = std::prev(it)->first;
                break;    
            }
        }
        Annotation::Get().Save();
        SelectedFrame = NewFrame;
        LoadFrame(SelectedFrame);
        Annotation::Get().Load();
        ImGui::SetWindowFocus("Annotation");
        ShouldUpdateData = true;
    }

    void DataBrowser::LoadFrame(int FrameNumber)
    {
        if (FrameNumber < 0)
        {
            if (FramesData.empty()) return;
            FrameNumber = FramesData.begin()->first;
            SelectedFrame = FrameNumber;
            Annotation::Get().Load();
        }
        cv::Mat frame;
        {
            cv::VideoCapture cap(SelectedClipInfo.ClipPath);
            if (!cap.isOpened())
            {
                LogPanel::Get().AddLog(ELogLevel::Error, "Fail to load video");
                return;
            }
            SelectedClipInfo.Width = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
            SelectedClipInfo.Height = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
            SelectedClipInfo.FPS = (float)cap.get(cv::CAP_PROP_FPS);
            SelectedClipInfo.FrameCount = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
            cap.set(cv::CAP_PROP_POS_FRAMES, FrameNumber);
            cap >> frame;
            cv::resize(frame, frame, cv::Size(1280, 720)); // 16 : 9
            cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
            cap.release();
        }
        glGenTextures(1, &LoadedFramePtr);
        glBindTexture(GL_TEXTURE_2D, LoadedFramePtr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); // Same
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame.cols, frame.rows, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, frame.data);
        // MakeFrameTitle();
        FrameExtractor::Get().OnFrameLoaded();
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
                    if (p.path().string().find(Format) != std::string::npos)
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

    bool DataBrowser::MakeClipSelectCombo()
    {
        bool Changed = false;
        size_t RelOffset = Setting::Get().ProjectPath.size() + 7;
        if (ImGui::BeginCombo("##ClipSelect", SelectedClipInfo.GetRelativePath().c_str()))
        {
            for (const std::string& C : GetAllClips())
            {
                if (ImGui::Selectable(C.substr(RelOffset).c_str()))
                {
                    SelectedClipInfo.ClipPath = C;
                    Changed = true;
                    UpdateData();
                }
            }
            ImGui::EndCombo();
        }
        return Changed;
    }

    void DataBrowser::MakeFrameSelectCombo()
    {
        if (FramesData.empty())
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "No frame extracted for this clip!");
            return;
        }
        if (ImGui::BeginCombo("##FrameSelect", std::to_string(SelectedFrame).c_str()))
        {
            for (const auto& [F, S] : FramesData)
            {
                if (ImGui::Selectable(std::to_string(F).c_str()))
                {
                    Annotation::Get().Save();
                    SelectedFrame = F;
                    LoadFrame(SelectedFrame);
                    Annotation::Get().Load();
                    ShouldUpdateData = true;
                }
            }
            ImGui::EndCombo();
        }
    }

    void DataBrowser::RenderContent()
    {
        if (!Setting::Get().ProjectIsLoaded) return;
        // Inside child window to have independent scroll
        const std::string ClipFolderPath = Setting::Get().ProjectPath + "/Clips";
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
                ImGui::Checkbox("Need review only?", &NeedReviewedOnly);

                ImGui::TreePop();
            }

            if (ImGui::TreeNode((std::string(ICON_FA_IMAGE) + " Images").c_str()))
            {
                // TODO: add from raw image as very end...leave it when all major feature is done...
                // for (int i = 0; i < 100; i++)
                // {
                //     ImGui::Selectable("img");
                // }
                ImGui::TextColored(Style::RED(400, Setting::Get().Theme),
                                   "Image is not supported yet!\nIt's not the major workflow.");
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
        if (ShouldUpdateData)
        {
            UpdateData();
            ShouldUpdateData = false;
        }


        // TODO: this is pretty bad... still event triggered reset data is much better! (next frameupdate?)
        /*// every 10 second or the first time update all required data?
        if (Utils::FloatCompare(0.0f, TimePassed, 0.000001f) || int(TimePassed + 1) % 11 == 0)
        {
            UpdateData();
            TimePassed = 0.f;
        }
        TimePassed += 1.0f / ImGui::GetIO().Framerate;*/
    }


    // TODO: crashed when click around frames? in release mode?

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
                    if (entry.path().string().find(Format) != std::string::npos)
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
                std::string FullClipName = entry.path().u8string();
                std::replace(FullClipName.begin(), FullClipName.end(), '\\', '/');
                if (ImGui::Selectable(ClipName.c_str(), SelectedClipInfo.ClipPath == FullClipName,
                                      ImGuiSelectableFlags_AllowDoubleClick))
                {
                    SelectedClipInfo.ClipPath = FullClipName;
                    // TODO: very unintuitive to understand how to start?
                    // Open and setup frame extraction
                    if (ImGui::IsMouseDoubleClicked(0) && Setting::Get().ActiveWorkspace == EWorkspace::Data)
                    {
                        // char buff[128];
                        // snprintf(buff, sizeof(buff), "open %s to start frame extraction?", FullClipName.c_str());
                        // LogPanel::Get().AddLog(ELogLevel::Info, buff);
                        ImGui::SetWindowFocus("Frame Extractor");
                        LoadFrame(0);
                        ShouldUpdateData = true;
                    }
                }
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    char Send[128];
                    strcpy(Send, FullClipName.c_str());
                    ImGui::SetDragDropPayload("FolderOrClip", &Send, sizeof(Send));
                    // for test now
                    ImGui::Text(Send);
                    ImGui::EndDragDropSource();
                }
                if (SelectedClipInfo.ClipPath == FullClipName && Setting::Get().ActiveWorkspace == EWorkspace::Data)
                // only show frames in data workspace
                {
                    ImGui::Indent();
                    ImGui::Indent();
                    for (const auto& [k, v] : FramesData)
                    {
                        const int FrameCount = k;
                        const int N_Annotations = int(v);
                        char buff[128];
                        snprintf(buff, sizeof(buff), "%d ...... (%d)", FrameCount, N_Annotations);
                        // open and setup annotation panel
                        if (ImGui::Selectable(buff, FrameCount == SelectedFrame))
                        {
                            // TODO: reload frames node here will crash... where should I move it?
                            // TODO: crash if no image load yet (if open extractor first and it is fine...)
                            Annotation::Get().Save();
                            SelectedFrame = FrameCount;
                            SelectedClipInfo.ClipPath = FullClipName;
                            LoadFrame(SelectedFrame);
                            Annotation::Get().Load();
                            ImGui::SetWindowFocus("Annotation");
                            ShouldUpdateData = true;
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
            // name
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

    void DataBrowser::UpdateData()
    {
        FramesData.clear();
        YAML::Node Node = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Annotations.yaml"))[
            SelectedClipInfo.ClipPath];
        for (YAML::const_iterator it = Node.begin(); it != Node.end(); ++it)
        {
            FramesData.insert({it->first.as<int>(), it->second.size()});
        }
        IsSelectedFrameReviewed = false;
        ReviewTime = "";
        YAML::Node ReviewData = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/Reviews.yaml");
        if (ReviewData[SelectedClipInfo.ClipPath])
        {
            if (ReviewData[SelectedClipInfo.ClipPath][SelectedFrame])
            {
                ReviewTime = ReviewData[SelectedClipInfo.ClipPath][SelectedFrame].as<std::string>();
                IsSelectedFrameReviewed = true;
            }
        }
        spdlog::info("Data is updated...");
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
