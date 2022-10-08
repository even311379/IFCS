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
    void DataBrowser::RenderContent()
    {
        if (!Setting::Get().ProjectIsLoaded) return;
        spdlog::info("data browser is rendering...");
        // Inside child window to have independent scroll
        ImGui::Text("Data Browser");
        ImVec2 ChildWindowSize = {ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.92f};
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        window_flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
        ImGui::BeginChild("DataBrowserWindow", ChildWindowSize, false, window_flags);
        {
            if (ImGui::TreeNodeEx((std::string(ICON_FA_VIDEO) + "    Video Clips").c_str(),
                                  ImGuiTreeNodeFlags_DefaultOpen))
            {
                std::string clips_folder_path = Setting::Get().ProjectPath + std::string("/TrainingClips");
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::Button(LOCTEXT("Common.OpenFolderHere")))
                    {
                        ShellExecuteA(NULL, "open", clips_folder_path.c_str(), NULL, NULL, SW_SHOWDEFAULT);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                RecursiveClipTreeNodeGenerator(clips_folder_path, 0);
                ImGui::TreePop();
            }

            // TODO: add from raw image as very end...leave it when all major feature is done...
            // if (ImGui::TreeNodeEx((std::string(ICON_FA_IMAGE) + "    Raw Images").c_str()))
            // {
            //     for (int i = 0; i < 100; i++)
            //     {
            //         ImGui::Selectable("img");
            //     }
            //     ImGui::TreePop();
            // }
        }

        ImGui::EndChild();
        // TODO: filter options... leave it when all major feature is done...
        // ImGui::Button(ICON_FA_FILE_IMPORT);
        // Utils::AddSimpleTooltip(LOCTEXT("ToolbarMenu.AddWks"));
        // ImGui::SameLine();
        // ImGui::Button(ICON_FA_FOLDER_PLUS);
        // Utils::AddSimpleTooltip(LOCTEXT("ToolbarMenu.AddWks"));
        // ImGui::SameLine();
        // if (ImGui::Button(ICON_FA_FOLDER_OPEN, GetBtnSize()))
        // {
        // }
        // Utils::AddSimpleTooltip(LOCTEXT("ToolbarMenu.AddWks"));
        // ImGui::SameLine();
        // if (ImGui::Button(ICON_FA_RECYCLE, GetBtnSize()))
        // {
        // }
        // Utils::AddSimpleTooltip("Refresh folder content");
        // ImGui::Separator();
        ImGui::Checkbox("TT", &Test);
        ImGui::SameLine();
        ImGui::Checkbox("TT", &Test);
        ImGui::InputText("Filter", FilterText, IM_ARRAYSIZE(FilterText));
    }

    ImVec2 DataBrowser::GetBtnSize()
    {
        return {ImGui::GetFont()->FontSize * 3, ImGui::GetFont()->FontSize * 1.5f};
    }

    // Copy and modify from https://discourse.dearimgui.org/t/how-to-mix-imgui-treenode-and-filesystem-to-print-the-current-directory-recursively/37
    void DataBrowser::RecursiveClipTreeNodeGenerator(const std::filesystem::path& Path, unsigned int Depth)
    {
        for (const auto& entry : std::filesystem::directory_iterator(Path))
        {
            ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
            if (std::filesystem::is_directory(entry.path()))
            {
                using namespace std::string_literals;
                std::string s = ICON_FA_FOLDER + " "s + entry.path().filename().string().c_str();
                if (ImGui::TreeNodeEx(s.c_str(), node_flags))
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
                std::string file_name = entry.path().filename().string();
                std::string full_clip_path = entry.path().string();
                std::replace(full_clip_path.begin(), full_clip_path.end(), '/', '\\');
                if (ImGui::Selectable(file_name.c_str(), SelectedClipInfo.ClipPath == full_clip_path,
                                      ImGuiSelectableFlags_AllowDoubleClick))
                {
                    SelectedClipInfo.ClipPath = full_clip_path;
                    UpdateFramesNode();
                    // Open and setup frame extraction
                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        char buff[128];
                        snprintf(buff, sizeof(buff), "open %s to start frame extraction?", full_clip_path.c_str());
                        LogPanel::Get().AddLog(ELogLevel::Info, buff);
                        ImGui::SetWindowFocus("Frame Extractor");
                        LoadFrame(1);
                        FrameExtractor::Get().LoadData();
                    }
                }
                if (SelectedClipInfo.ClipPath == full_clip_path)
                {
                    ImGui::Indent();
                    ImGui::Indent();
                    // TODO: grab data somewhere
                    for (YAML::const_iterator it = FramesNode.begin(); it != FramesNode.end(); it++)
                    {
                        int FrameCount = it->first.as<int>();
                        int N_Annotations = int(it->second.as<YAML::Node>().size());
                        char buff[128];
                        snprintf(buff, sizeof(buff), "%d ...... (%d)", FrameCount, N_Annotations);
                        // open and setup annotation panel
                        if (ImGui::Selectable(buff, FrameCount == SelectedFrame))
                        {
                            // TODO: reload frames node here will crash... where should I move it?
                            // UpdateFramesNode();
                            Annotation::Get().Save();
                            SelectedFrame = FrameCount;
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

    void DataBrowser::UpdateFramesNode()
    {
        YAML::Node Node = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Annotation.yaml"));
        FramesNode = Node[SelectedClipInfo.ClipPath];
    }

    // TODO: incorrect edge behavior
    void DataBrowser::LoadOtherFrame(bool IsNext)
    {
        std::queue<int> q;
        bool PrepareToStop = false;
        for (YAML::const_iterator it = FramesNode.begin(); it != FramesNode.end(); ++it)
        {
            int FrameNum = it->first.as<int>();
            q.push(FrameNum);
            if (q.size() > 3) q.pop();
            if (PrepareToStop) break;
            if (SelectedFrame == FrameNum)
                PrepareToStop = true;
        }
        if (q.size() == 2) // in case that selected frame is the first or last frame;
            return;
        // UpdateFramesNode();
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
        LoadedFramePtr = 0;
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
}
