#include "DataBrowser.h"

#include "Setting.h"

#include "ImguiNotify/font_awesome_5.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <shellapi.h>
#include <yaml-cpp/yaml.h>
#include "opencv2/opencv.hpp"
#include "imgui_impl_glfw.h"
#include <GLFW/glfw3.h>

#include "Annotation.h"
#include "FrameExtractor.h"
#include "Log.h"
#include "Utils.h"
#include "Implot/implot.h"


namespace IFCS
{
    // TODO: incorrect edge behavior
    void DataBrowser::LoadOtherFrame(bool IsNext)
    {
        std::queue<int> q;
        bool PrepareToStop = false;
        for (const auto& [k, v] : FramesData)
        {
            int FrameNum = k;
            q.push(FrameNum);
            if (q.size() > 3) q.pop();
            if (PrepareToStop) break;
            if (SelectedFrame == FrameNum)
                PrepareToStop = true;
        }
        if (q.size() == 2) // in case that selected frame is the first or last frame;
            return;
        Annotation::Get().Save();
        if (IsNext) SelectedFrame = q.back();
        else SelectedFrame = q.front();
        LoadFrame(SelectedFrame);
        Annotation::Get().Load();
        ImGui::SetWindowFocus("Annotation");
    }

    void DataBrowser::LoadFrame(int FrameNumber)
    {
        AnyFrameLoaded = true;

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
            cv::cvtColor(frame, frame, cv::COLOR_BGR2RGBA);
            cap.release();
        }
        glGenTextures(1, &LoadedFramePtr);
        glBindTexture(GL_TEXTURE_2D, LoadedFramePtr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        // This is required on WebGL for non power-of-two textures
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); // Same
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame.cols, frame.rows, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, frame.data);
        MakeFrameTitle();
        FrameExtractor::Get().OnFrameLoaded();
    }

    // TODO: remove project path for better looking?
    std::vector<std::string> DataBrowser::GetAllClips() const
    {
        std::vector<std::string> Out;
        for (const auto& p : std::filesystem::recursive_directory_iterator(
                 Setting::Get().ProjectPath + std::string("/TrainingClips")))
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

    void DataBrowser::RenderContent()
    {
        if (!Setting::Get().ProjectIsLoaded) return;
        // Inside child window to have independent scroll
        ImGui::Text("Data Browser");
        ImVec2 ChildWindowSize = ImGui::GetContentRegionAvail();
        if (IsViewingSomeDetail)
            ChildWindowSize.y *= 0.75f;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        window_flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
        ImGui::BeginChild("DataBrowserWindow", ChildWindowSize, false, window_flags);
        {
            if (ImGui::TreeNodeEx((std::string(ICON_FA_VIDEO) + " Training Clips").c_str(),
                                  ImGuiTreeNodeFlags_DefaultOpen))
            {
                std::string ClipFolderPath = Setting::Get().ProjectPath + "/TrainingClips";
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::Button(LOCTEXT("Common.OpenFolderHere")))
                    {
                        ShellExecuteA(NULL, "open", ClipFolderPath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                RecursiveClipTreeNodeGenerator(ClipFolderPath, 0);
                // TODO: add some hint to add things in content browser if nothing here...
                ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));
                ImGui::Checkbox("Need review only?", &NeedReviewedOnly);
                ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));
                ImGui::TreePop();
            }

            if (ImGui::TreeNode((std::string(ICON_FA_IMAGE) + " Training Images").c_str()))
            {
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::Button(LOCTEXT("Common.OpenFolderHere")))
                    {
                        ShellExecuteA(NULL, "open", (Setting::Get().ProjectPath + "/TrainingImages").c_str(), NULL,
                                      NULL, SW_SHOWDEFAULT);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                // TODO: add from raw image as very end...leave it when all major feature is done...
                for (int i = 0; i < 100; i++)
                {
                    ImGui::Selectable("img");
                }
                ImGui::TreePop();
            }
            if (ImGui::TreeNode((std::string(ICON_FA_TH) + " Training Sets").c_str()))
            {
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::Button(LOCTEXT("Common.OpenFolderHere")))
                    {
                        ShellExecuteA(NULL, "open", (Setting::Get().ProjectPath + "/Data").c_str(), NULL, NULL,
                                      SW_SHOWDEFAULT);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                CreateSeletable_TrainingSets();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode((std::string(ICON_FA_MICROCHIP) + " Models").c_str()))
            {
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::Button(LOCTEXT("Common.OpenFolderHere")))
                    {
                        ShellExecuteA(NULL, "open", (Setting::Get().ProjectPath + "/Models").c_str(), NULL, NULL,
                                      SW_SHOWDEFAULT);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                CreateSeletable_Models();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode((std::string(ICON_FA_WAVE_SQUARE) + " Predictions").c_str()))
            {
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::Button(LOCTEXT("Common.OpenFolderHere")))
                    {
                        ShellExecuteA(NULL, "open", (Setting::Get().ProjectPath + "/Predictions").c_str(), NULL, NULL,
                                      SW_SHOWDEFAULT);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                CreateSeletable_Predictions();
                ImGui::TreePop();
            }
        }

        ImGui::EndChild();
        // // TODO: filter options... leave it when all major feature is done...
        // Should I add filter? to filter where that's not labeled yet? or show need review? 
        // ImGui::Checkbox("TT", &Test);
        // ImGui::SameLine();
        // ImGui::Checkbox("TT", &Test);
        // ImGui::InputText("Filter", FilterText, IM_ARRAYSIZE(FilterText));

        // every 10 second or the first time update all required data?
        if (Utils::FloatCompare(0.0f, TimePassed, 0.000001f) || int(TimePassed + 1) % 11 == 0)
        {
            UpdateData();
            TimePassed = 0.f;
        }
        TimePassed += 1.0f / ImGui::GetIO().Framerate;
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
                    // Open and setup frame extraction
                    if (ImGui::IsMouseDoubleClicked(0) && Setting::Get().ActiveWorkspace == EWorkspace::Data)
                    {
                        // char buff[128];
                        // snprintf(buff, sizeof(buff), "open %s to start frame extraction?", FullClipName.c_str());
                        // LogPanel::Get().AddLog(ELogLevel::Info, buff);
                        ImGui::SetWindowFocus("Frame Extractor");
                        LoadFrame(0);
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
        // if (Tick % )
        // YAML::LoadFile(Setting::Get().ProjectPath + );
        // ImGui::Text("frame rate: %.2f", ImGui::GetIO().Framerate);
    }

    void DataBrowser::CreateSeletable_Models()
    {
    }

    void DataBrowser::CreateSeletable_Predictions()
    {
    }

    void DataBrowser::UpdateData()
    {
        FramesData.clear();
        YAML::Node Node = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Annotation.yaml"))[
            SelectedClipInfo.ClipPath];
        for (YAML::const_iterator it = Node.begin(); it != Node.end(); ++it)
        {
            FramesData.insert({it->first.as<int>(), it->second.size()});
        }
        spdlog::info("Data is updated...");
    }

    void DataBrowser::MakeFrameTitle()
    {
        float i = 1.0f;
        while (SelectedClipInfo.FrameCount > pow(10, i))
        {
            i += 1.0f;
        }
        int FrameCountDigits = int(i) - 1;
        i = 1.0f;
        while (SelectedFrame > pow(10, i))
        {
            i += 1.0f;
        }
        int CurrentFrameDigits = int(i) - 1;
        std::string temp = std::to_string(SelectedFrame);
        for (int i = 0; i < FrameCountDigits - CurrentFrameDigits; i++)
        {
            temp = std::string("0") + temp;
        }
        FrameTitle = SelectedClipInfo.GetClipFileName() + "......(" + temp + "/" + std::to_string(
            SelectedClipInfo.FrameCount) + ")";
    }

    ImVec2 DataBrowser::GetBtnSize()
    {
        return {ImGui::GetFont()->FontSize * 3, ImGui::GetFont()->FontSize * 1.5f};
    }
}
