#include "DataBrowser.h"

#include "Setting.h"

#include "ImguiNotify/font_awesome_5.h"
#include <iostream>
#include <spdlog/spdlog.h>

#include "Log.h"
#include "Utils.h"


void IFCS::DataBrowser::RenderContent()
{
    if (!Setting::Get().ProjectIsLoaded) return;

    // Inside child window to have independent scroll
    ImGui::Text("Data Browser");
    ImVec2 ChildWindowSize = {ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.92f};
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
    window_flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
    ImGui::BeginChild("DataBrowserWindow", ChildWindowSize, false, window_flags);
    {
        if (ImGui::TreeNodeEx((std::string(ICON_FA_VIDEO) + "    Video Clips").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::Button(LOCTEXT("Common.OpenFolderHere")))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            // using namespace std::string_literals;
            // char buff[128];
            // sprintf(buff, "%s/training_clips", Setting::Get().ProjectPath.c_str());
            // spdlog::error(buff);

            
            std::string clips_folder_path = Setting::Get().ProjectPath + std::string("/training_clips");
            RecursiveClipTreeNodeGenerator(clips_folder_path, 0);
            ImGui::TreePop();
        }

        // TODO: add from raw image as very end...
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

ImVec2 IFCS::DataBrowser::GetBtnSize()
{
    return {ImGui::GetFont()->FontSize * 3, ImGui::GetFont()->FontSize * 1.5f};
}

// Copy and modify from https://discourse.dearimgui.org/t/how-to-mix-imgui-treenode-and-filesystem-to-print-the-current-directory-recursively/37
void IFCS::DataBrowser::RecursiveClipTreeNodeGenerator(const std::filesystem::path& path, unsigned int depth)
{
    spdlog::error("here??000");
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        spdlog::error("here??111");
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (std::filesystem::is_directory(entry.path()))
        {
            using namespace std::string_literals;
            std::string s = ICON_FA_FOLDER + " "s + entry.path().filename().string().c_str();
            if (ImGui::TreeNodeEx(s.c_str(), node_flags))
            {
                RecursiveClipTreeNodeGenerator(entry, depth + 1);
                ImGui::TreePop();
            }
        }
        else
        {
            // if (!Utils::Contains(AcceptedClipsFormat, entry.path().string())) return;
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
            if (depth > 0)
            {
                ImGui::Indent();
            }
            std::string file_name = entry.path().filename().string();
            if (ImGui::Selectable(file_name.c_str(), selected_video==file_name, ImGuiSelectableFlags_AllowDoubleClick))
            {
                selected_video = file_name;
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    char buff[128];
                    sprintf(buff, "open %s to start frame extraction?", file_name.c_str());
                    LogPanel::Get().AddLog(ELogLevel::Info, buff);
                    // TODO: Add related events...
                }
            }
            if (selected_video == file_name)
            {
                ImGui::Indent();ImGui::Indent();
                // TODO: grab data somewhere
                for (int i = 0; i < 5; i++)
                {
                    if (ImGui::Selectable("frame", false, ImGuiSelectableFlags_AllowDoubleClick))
                    {
                        selected_video = file_name;
                        // TODO: do something to update selection hint
                        if (ImGui::IsMouseDoubleClicked(0))
                        {
                            LogPanel::Get().AddLog(ELogLevel::Warning, "Try to open this frame to edit?");
                        }
                    }
                }
                ImGui::Unindent();ImGui::Unindent();
            }
            if (depth > 0)
            {
                ImGui::Unindent();
            }
        }
    }
}
