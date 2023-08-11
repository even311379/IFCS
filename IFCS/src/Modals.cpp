#include "Modals.h"

#include <thread>
#include <fstream>
#include <shellapi.h>
#include "ImGuiFileDialog/ImGuiFileDialog.h"
#include "ImguiMD/imgui_markdown.h"
#include "yaml-cpp/yaml.h"
#include "misc/cpp/imgui_stdlib.h"

#include "Style.h"
#include "Setting.h"
#include "CategoryManagement.h"
#include "DataBrowser.h"
#include "Deploy.h"
#include "Train.h"

#include <locale>
#include <codecvt>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

#include "Annotation.h"
#include "Application.h"

namespace IFCS
{
    static ImVec2 Center;

    static ImGui::MarkdownConfig mdConfig;

    void LinkCallback(ImGui::MarkdownLinkCallbackData data_)
    {
        std::string url(data_.link, data_.linkLength);
        if (!data_.isImage)
        {
            ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
    }

    void MarkdownFormatCallback(const ImGui::MarkdownFormatInfo& markdownFormatInfo_, bool start_)
    {
        // Call the default first so any settings can be overwritten by our implementation.
        // Alternatively could be called or not called in a switch statement on a case by case basis.
        // See defaultMarkdownFormatCallback definition for furhter examples of how to use it.
        ImGui::defaultMarkdownFormatCallback(markdownFormatInfo_, start_);

        ETheme ActiveTheme = Setting::Get().Theme;
        switch (markdownFormatInfo_.type)
        {
        // example: change the colour of heading level 2
        case ImGui::MarkdownFormatType::HEADING:
            {
                if (markdownFormatInfo_.level == 1)
                {
                    if (start_)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, Style::RED(700, ActiveTheme));
                    }
                    else
                    {
                        ImGui::PopStyleColor();
                    }
                }
                else if (markdownFormatInfo_.level == 2)
                {
                    if (start_)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, Style::RED(500, ActiveTheme));
                    }
                    else
                    {
                        ImGui::PopStyleColor();
                    }
                }
                else if (markdownFormatInfo_.level == 3)
                {
                    if (start_)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, Style::RED(400, ActiveTheme));
                    }
                    else
                    {
                        ImGui::PopStyleColor();
                    }
                }
                break;
            }
        case ImGui::MarkdownFormatType::EMPHASIS:
            {
                if (start_)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, Style::BLUE(400, ActiveTheme));
                }
                else
                {
                    ImGui::PopStyleColor();
                }
            }
        default:
            {
                break;
            }
        }
    }

    void Markdown(const std::string& markdown_)
    {
        // You can make your own Markdown function with your prefered string container and markdown config.
        // > C++14 can use ImGui::MarkdownConfig mdConfig{ LinkCallback, NULL, ImageCallback, ICON_FA_LINK, { { H1, true }, { H2, true }, { H3, false } }, NULL };
        mdConfig.linkCallback = LinkCallback;
        mdConfig.tooltipCallback = NULL;
        mdConfig.headingFormats[0] = {Setting::Get().TitleFont, false};
        mdConfig.headingFormats[1] = {Setting::Get().H2, false};
        mdConfig.headingFormats[2] = {Setting::Get().H3, false};
        mdConfig.userData = NULL;
        mdConfig.formatCallback = MarkdownFormatCallback;
        ImGui::Markdown(markdown_.c_str(), markdown_.length(), mdConfig);
    }

    std::string LoadMDFile(const std::string& FileName)
    {
        std::ifstream t(std::string("Resources/md/") + FileName + std::string(".md"));
        std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        return str.substr(3); // ISSUE: need to add 3 offset to remove "?" appear in first character...
    }

    void Modals::Render()
    {
        if (IsModalOpen_Welcome)
            ImGui::OpenPopup("Welcome");
        if (IsModalOpen_NewProject)
            ImGui::OpenPopup("New Project");
        if (IsModalOpen_LoadProject)
            ImGui::OpenPopup("Load Project");
        if (IsModalOpen_ImportData)
            ImGui::OpenPopup("Import Data");
        if (IsModalOpen_Setting)
            ImGui::OpenPopup("Setting");
        if (IsModalOpen_About)
            ImGui::OpenPopup("About");
        if (IsModalOpen_Tutorial)
            ImGui::OpenPopup("Tutorial");
        if (IsModalOpen_Contact)
            ImGui::OpenPopup("Contact");
        if (IsModalOpen_License)
            ImGui::OpenPopup("License");
        Center = ImGui::GetMainViewport()->GetCenter();
        RenderWelcome();
        RenderNewProject();
        RenderLoadProject();
        RenderSetting();
        RenderImportData();
        RenderDoc("About");
        RenderDoc("Tutorial");
        RenderDoc("Contact");
        RenderDoc("License");
        if (IsModalOpen_Delete)
            ImGui::OpenPopup("Delete Asset");
        RenderDeleteModal(DataBrowser::Get().LastSelectedAssetType);
        HandleFileDialogClose();
    }

    void Modals::Sync()
    {
        ThemeToUse = static_cast<int>(Setting::Get().Theme);
        strcpy(TempPythonPath, Setting::Get().PythonPath.c_str());
        strcpy(TempYoloV7Path, Setting::Get().YoloV7Path.c_str());
    }

    void Modals::RenderWelcome()
    {
        ImGui::SetNextWindowPos(Center + ImVec2(0, 200), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Welcome", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            Markdown(LoadMDFile("Welcome"));
            static bool bFromExistingProject = false;
            ImGui::Checkbox("From existing project?", &bFromExistingProject);
            if (bFromExistingProject)
            {
                ImGui::BulletText("Project Location:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(400);
                ImGui::InputText("##hidden123", TempExistingProjectLocation, IM_ARRAYSIZE(TempExistingProjectLocation),
                                 ImGuiInputTextFlags_ReadOnly);
                ImGui::SameLine();
                if (ImGui::Button("Choose Existing Project"))
                {
                    ImGuiFileDialog::Instance()->OpenDialog("ChooseExistingProjectLocationDialog", "Choose existing project location",
                        nullptr, ".", "", 1, nullptr, ImGuiFileDialogFlags_Modal);
                }
                ImGui::BeginDisabled(!CheckValidExistingProject());
                if (ImGui::Button("OK", ImVec2(ImGui::GetWindowWidth() * 0.2f, ImGui::GetFontSize() * 1.5f)))
                {
                    Setting::Get().ProjectPath = TempExistingProjectLocation;
                    Setting::Get().StartFromPreviousProject();
                    ImGui::CloseCurrentPopup();
                    IsModalOpen_Welcome = false;
                }
                ImGui::EndDisabled();
            }
            else
            {
                ImGui::BulletText("New Project Name:");
                ImGui::SameLine(300);
                ImGui::SetNextItemWidth(400);
                ImGui::InputText("##Project Name", TempProjectName, IM_ARRAYSIZE(TempProjectName));
                ImGui::BulletText("New Project Location:");
                ImGui::SameLine(300);
                ImGui::SetNextItemWidth(400);
                ImGui::InputText("##hidden", TempProjectLocation, IM_ARRAYSIZE(TempProjectLocation),
                                 ImGuiInputTextFlags_ReadOnly);
                ImGui::SameLine();
                if (ImGui::Button("Choose"))
                {
                    ImGuiFileDialog::Instance()->OpenDialog("ChooseNewProjectLocationDialog", "Choose new project location", nullptr,
                        ".", "", 1, nullptr, ImGuiFileDialogFlags_Modal);
                }
                ImGui::BeginDisabled(!CheckValidProjectName());
                if (ImGui::Button("OK", ImVec2(240, 0)))
                {
                    Setting::Get().Project = TempProjectName;
                    Setting::Get().ProjectPath = std::string(TempProjectLocation) + "/" + Setting::Get().Project;
                    Setting::Get().CreateStartup();
                    ImGui::CloseCurrentPopup();
                    IsModalOpen_Welcome = false;
                    IsModalOpen_Setting = true;
                }
                ImGui::EndDisabled();
                if (!CheckValidProjectName())
                {
                    ImGui::SameLine();
                    ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Fill in valid content");
                }
            }
            if (ImGuiFileDialog::Instance()->Display("ChooseNewProjectLocationDialog", ImGuiWindowFlags_NoCollapse, ImVec2(800, 600))) 
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    std::string RawString = Utils::ChangePathSlash(ImGuiFileDialog::Instance()->GetCurrentPath());
                    strcpy(TempProjectLocation, RawString.c_str());
                }
                ImGuiFileDialog::Instance()->Close();
            }
            if (ImGuiFileDialog::Instance()->Display("ChooseExistingProjectLocationDialog", ImGuiWindowFlags_NoCollapse, ImVec2(800, 600)))
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    std::string RawString = Utils::ChangePathSlash(ImGuiFileDialog::Instance()->GetCurrentPath());
                    strcpy(TempExistingProjectLocation, RawString.c_str());
                }
                ImGuiFileDialog::Instance()->Close();
            }
            ImGui::EndPopup();
        }
    }

    bool Modals::CheckValidProjectName()
    {
        if (strlen(TempProjectName) == 0) return false;
        if (strlen(TempProjectLocation) == 0) return false;
        // check folder name is used
        for (const auto& entry : std::filesystem::directory_iterator(TempProjectLocation))
        {
            if (entry.path().filename().u8string() == std::string(TempProjectName))
                return false;
        }
        const char InvalidChars[] = "\\/:*?\"<>|";
        for (char C : InvalidChars)
        {
            if (std::string(TempProjectName).find(C) != std::string::npos)
                return false;
        }
        return true;
    }

    bool Modals::CheckValidExistingProject()
    {
        return std::filesystem::exists(std::string(TempExistingProjectLocation) + "/IFCSUser.ini");
    }

    void Modals::RenderNewProject()
    {
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::BulletText("New Project Name:");
            ImGui::SameLine(300);
            ImGui::SetNextItemWidth(400);
            ImGui::InputText("##Project Name", TempProjectName, IM_ARRAYSIZE(TempProjectName));
            ImGui::BulletText("New Project Location:");
            ImGui::SameLine(300);
            ImGui::SetNextItemWidth(400);
            ImGui::InputText("##hidden", TempProjectLocation, IM_ARRAYSIZE(TempProjectLocation),
                             ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            if (ImGui::Button("Choose"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("ChooseNewProjectLocationDialog", "Choose new project location", nullptr, ".", "", 1, nullptr,
                    ImGuiFileDialogFlags_Modal);
            }
            ImGui::BeginDisabled(!CheckValidProjectName());
            if (ImGui::Button("OK", ImVec2(240, 0)))
            {
                Setting::Get().Project = TempProjectName;
                Setting::Get().ProjectPath = std::string(TempProjectLocation) + TempProjectName;
                Setting::Get().CreateStartup();
                ImGui::CloseCurrentPopup();
                IsModalOpen_NewProject = false;
                IsModalOpen_Setting = true;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(240, 0)))
            {
                ImGui::CloseCurrentPopup();
                IsModalOpen_NewProject = false;
            }

            if (!CheckValidProjectName())
            {
                ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Fill in valid content");
            }
            if (ImGuiFileDialog::Instance()->Display("ChooseNewProjectLocationDialog", ImGuiWindowFlags_NoCollapse, ImVec2(800, 600))) 
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    std::string RawString = Utils::ChangePathSlash(ImGuiFileDialog::Instance()->GetCurrentPath());
                    strcpy(TempProjectLocation, RawString.c_str());
                }
                ImGuiFileDialog::Instance()->Close();
            }
            ImGui::EndPopup();
        }
    }

    void Modals::RenderLoadProject()
    {
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Load Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::BulletText("Project Location:");
            ImGui::SetNextItemWidth(400);
            ImGui::InputText("##hidden123", TempExistingProjectLocation, IM_ARRAYSIZE(TempExistingProjectLocation),
                             ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            if (ImGui::Button("Choose Existing Project"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("ChooseExistingProjectLocationDialog", "Choose existing project location", nullptr, Setting::Get().ProjectPath, 1, nullptr, ImGuiFileDialogFlags_Modal);
            }
            ImGui::BeginDisabled(!CheckValidExistingProject());
            if (ImGui::Button("OK", ImVec2(240, 0)))
            {
                Setting::Get().ProjectPath = TempExistingProjectLocation;
                Setting::Get().StartFromPreviousProject();

                ImGui::CloseCurrentPopup();
                IsModalOpen_LoadProject = false;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(240, 0)))
            {
                ImGui::CloseCurrentPopup();
                IsModalOpen_LoadProject = false;
            }
            if (ImGuiFileDialog::Instance()->Display("ChooseExistingProjectLocationDialog", ImGuiWindowFlags_NoCollapse, ImVec2(800, 600)))
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    std::string RawString = Utils::ChangePathSlash(ImGuiFileDialog::Instance()->GetCurrentPath());
                    strcpy(TempExistingProjectLocation, RawString.c_str());
                }
                ImGuiFileDialog::Instance()->Close();
            }
            ImGui::EndPopup();
        }
    }

    size_t NumNewAnnotationsToImport = 0;
    std::map<std::string, std::vector<int>> ConflictedFrames; // rel file path : vector of frame count or [0] for image
    int NumConflicted = 0;
    bool IsSourceValid = false;
    std::string InvalidReason;
    char FileToImport[255];
    int ConflictProcessingMode = 0;

    void CheckIsDataValidToImport()
    {
        NumNewAnnotationsToImport = 0;
        ConflictedFrames.clear();
        NumConflicted = 0;
        IsSourceValid = true;
        YAML::Node DataToImport = YAML::LoadFile(FileToImport);
        // check if file is valid...
        if (!DataToImport["SourceProject"] || !DataToImport["Categories"] || !DataToImport["Annotations"])
        {
            IsSourceValid = false;
            InvalidReason = "Loaded data is corrupted!";
            return;
        }
        // check if category is valid
        std::vector<std::string> RegisteredCatNames;
        for (const auto& [ID, Cat] : CategoryManagement::Get().Data)
            RegisteredCatNames.push_back(Cat.DisplayName);
        for (YAML::const_iterator it = DataToImport["Categories"].begin();
             it != DataToImport["Categories"].end(); ++it)
        {
            if (!Utils::Contains(RegisteredCatNames, it->second["DisplayName"].as<std::string>()))
            {
                IsSourceValid = false;
                InvalidReason = "Source project contains mismatched category name";
                return;
            }
        }
        // check if img/clip file exists
        std::set<std::string> SourceFiles;
        for (YAML::const_iterator it = DataToImport["Annotations"].begin();
             it != DataToImport["Annotations"].end(); ++it)
        {
            SourceFiles.insert(it->first.as<std::string>());
        }
        std::vector<std::string> AllSourceFiles = DataBrowser::Get().GetAllClips();
        std::vector<std::string> AllImgFiles = DataBrowser::Get().GetAllImages();
        AllSourceFiles.insert(AllSourceFiles.end(), AllImgFiles.begin(), AllImgFiles.end());
        for (auto& f : AllSourceFiles)
        {
            f = f.substr(Setting::Get().ProjectPath.size());
        }
        for (const auto& f : SourceFiles)
        {
            if (!Utils::Contains(AllSourceFiles, f))
            {
                IsSourceValid = false;
                InvalidReason = "Some image / clip file is missing in target project!";
                return;
            }
        }
        // calculate conflicted stuff
        YAML::Node SourceAnnotation = DataToImport["Annotations"];
        YAML::Node TargetAnnotation = YAML::LoadFile(
            Setting::Get().ProjectPath + "/Data/Annotations.yaml");
        for (YAML::const_iterator s = SourceAnnotation.begin(); s != SourceAnnotation.end(); ++s)
        {
            for (YAML::const_iterator t = TargetAnnotation.begin(); t != TargetAnnotation.end(); ++t)
            {
            // if (std::string(TempProjectName).find(C) != std::string::npos)
                if (s->first.as<std::string>() == t->first.as<std::string>())
                {
                    std::string FileName = s->first.as<std::string>();
                    std::string FileExtension = FileName.substr(FileName.size() - 4);
                
                    if (Utils::Contains(DataBrowser::Get().AcceptedClipsFormat, FileExtension))
                    {
                        for (YAML::const_iterator sf = s->second.begin(); sf != s->second.end(); ++sf)
                        {
                            for (YAML::const_iterator tf = t->second.begin(); tf != t->second.end(); ++tf)
                            {
                                if (sf->first.as<int>() == tf->first.as<int>())
                                {
                                    NumConflicted += 1;
                                    std::string fn = s->first.as<std::string>();
                                    if (!ConflictedFrames.count(fn))
                                        ConflictedFrames[fn] = {sf->first.as<int>()};
                                    else
                                        ConflictedFrames[fn].push_back(sf->first.as<int>());
                                }
                            }
                        }
                    }
                    else
                    {
                        NumConflicted += 1;
                        std::string fn = s->first.as<std::string>();
                        ConflictedFrames[fn] = {0};
                    }
                }
            }
            // calc num to import
            NumNewAnnotationsToImport += s->second.size();
        }
    }

    void ProcessingDataImport()
    {
        YAML::Node SourceData = YAML::LoadFile(FileToImport);
        // remap CID
        std::map<UUID, UUID> NewCatIDMap;
        for (YAML::const_iterator it = SourceData["Categories"].begin(); it != SourceData["Categories"].end(); ++it)
        {
            for (const auto& [CID, Cat] : CategoryManagement::Get().Data)
            {
                if (it->second["DisplayName"].as<std::string>() == Cat.DisplayName)
                {
                    NewCatIDMap[it->first.as<uint64_t>()] = CID;
                }
            }
        }

        std::map<std::string, std::map<int, YAML::Node>> ToImport;
        std::string OldPath = SourceData["SourceProject"].as<std::string>();
        for (YAML::const_iterator it = SourceData["Annotations"].begin(); it != SourceData["Annotations"].end(); ++it)
        {
            // if conflicted
            std::string FileName = it->first.as<std::string>();
            std::string FileExtension = FileName.substr(FileName.size() - 4);
            if (Utils::Contains(DataBrowser::Get().AcceptedClipsFormat, FileExtension))
            {
                for (YAML::const_iterator f = it->second.begin(); f != it->second.end(); ++f)
                {
                    YAML::Node ANode = f->second;
                    for (YAML::iterator ann = ANode.begin(); ann != ANode.end(); ++ann)
                    {
                        if (ann->first.as<std::string>() == "IsReady") continue;
                        if (ann->first.as<std::string>() == "UpdateTime") continue;
                        uint64_t NewID = NewCatIDMap[ann->second["CategoryID"].as<uint64_t>()];
                        ann->second["CategoryID"] = NewID;
                    }
                    ANode["IsReady"] = false;
                    ANode["UpdateTime"] = Utils::GetCurrentTimeString();
                    ToImport[FileName][f->first.as<int>()] = ANode;
                }
            }
            else // it's just image
            {
                YAML::Node ANode = it->second[0];
                for (YAML::iterator ann = ANode.begin(); ann != ANode.end(); ++ann)
                {
                    if (ann->first.as<std::string>() == "IsReady") continue;
                    if (ann->first.as<std::string>() == "UpdateTime") continue;
                    uint64_t NewID = NewCatIDMap[ann->second["CategoryID"].as<uint64_t>()];
                    ann->second["CategoryID"] = NewID;
                }
                ANode["IsReady"] = false;
                ANode["UpdateTime"] = Utils::GetCurrentTimeString();
                ToImport[Setting::Get().ProjectPath + FileName.substr(OldPath.size())][0] = ANode;
            }
        }
        YAML::Node TargetAnnotations = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/Annotations.yaml");
        for (const auto& [FilePath, Anns] : ToImport)
        {
            for (const auto& [f, ANode] : Anns)
            {
                switch (ConflictProcessingMode)
                {
                case 0: // all
                    if (ConflictedFrames.count(FilePath))
                    {
                        if (Utils::Contains(ConflictedFrames[FilePath], f))
                        {
                            for (YAML::const_iterator a = ANode.begin(); a != ANode.end(); ++a)
                                TargetAnnotations[FilePath][f][a->first.as<std::string>()] = a->second;
                            spdlog::info("add??");
                            break;
                        }
                    }
                    TargetAnnotations[FilePath][f] = ANode;
                    break;
                case 1: // source
                    if (ConflictedFrames.count(FilePath))
                    {
                        if (Utils::Contains(ConflictedFrames[FilePath], f))
                        {
                            break;
                        }
                    }
                    TargetAnnotations[FilePath][f] = ANode;
                    break;
                case 2: // target
                    TargetAnnotations[FilePath][f] = ANode;
                    break;
                default: ;
                }
            }
        }
        YAML::Emitter Out;
        Out << TargetAnnotations;
        std::ofstream Fout(Setting::Get().ProjectPath + "/Data/Annotations.yaml");
        Fout << Out.c_str();
        CategoryManagement::Get().NeedUpdate = true;

        // TODO: need to reopen editor...
        /*
         * somehow ask program to load data file now will result in crash...
         * probably due to the data is still writing to file...
         * need someway to wait for writing file...
         */
        
        // Annotation::Get().LoadDataFile();
        // Annotation::Get().NeedSaveFile = true;
    }


    void Modals::RenderImportData()
    {
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(1920, 1080) * 0.6f);
        if (ImGui::BeginPopupModal("Import Data"))
        {
            ImVec2 ChildWindowSize = ImGui::GetContentRegionAvail();
            ChildWindowSize.y *= 0.93f;
            ImGui::BeginChild("Content", ChildWindowSize, false);
            {
                ImGui::BulletText("Choose Data to import:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(400);
                ImGui::InputText("##hidden", FileToImport, IM_ARRAYSIZE(FileToImport), ImGuiInputTextFlags_ReadOnly);
                ImGui::SameLine();
                if (ImGui::Button("Choose data to import"))
                {
                    ImGuiFileDialog::Instance()->OpenDialog("ChooseDataToImportDialogue", "Choose data to import", ".yaml",
                        Setting::Get().ProjectPath, 1, nullptr, ImGuiFileDialogFlags_Modal);
                }
                if (!IsSourceValid)
                {
                    ImGui::TextColored(Style::RED(400, Setting::Get().Theme),
                                       "Error! Something wrong in source data");
                    ImGui::Text("  %s", InvalidReason.c_str());
                }
                else
                {
                    if (NumConflicted > 0)
                    {
                        ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Conflict detected!");
                        ImGui::Text("%d frames / images are annotated in both source and target project.",
                                    NumConflicted);
                        ImGui::BeginChildFrame(ImGui::GetID("Conflicted"),
                                               ImVec2(ChildWindowSize.x, ImGui::GetTextLineHeightWithSpacing() * 6));
                        for (const auto& [S , VF] : ConflictedFrames)
                        {
                            ImGui::Text(S.c_str());
                            if (Utils::Contains(DataBrowser::Get().AcceptedClipsFormat, S.substr(S.size() - 4)))
                            {
                                std::string vs = "[";
                                for (const int& v : VF)
                                {
                                    vs += std::to_string(v) + ",";
                                }
                                vs = vs.substr(0, vs.size() - 1);
                                vs += "]";
                                ImGui::SameLine();
                                ImGui::Text(vs.c_str());
                            }
                        }
                        ImGui::EndChildFrame();
                        ImGui::BulletText("How to handle conflict?");
                        ImGui::RadioButton("Accept all", &ConflictProcessingMode, 0);
                        ImGui::SameLine();
                        ImGui::RadioButton("Accept source", &ConflictProcessingMode, 1);
                        ImGui::SameLine();
                        ImGui::RadioButton("Accept target", &ConflictProcessingMode, 2);
                    }
                    else
                    {
                        ImGui::Text("No any conflict detected!");
                    }
                    ImGui::Separator();
                    if (ConflictProcessingMode == 0)
                    {
                        ImGui::Text("About to add %d new annotations!", NumNewAnnotationsToImport);
                    }
                    else if (ConflictProcessingMode == 1)
                    {
                        ImGui::Text("About to add %d new annotations!", NumNewAnnotationsToImport - NumConflicted);
                    }
                    else
                    {
                        ImGui::Text("About to add %d new annotations!", NumNewAnnotationsToImport - NumConflicted);
                        ImGui::Text("About to change %d annotations!", NumConflicted);
                    }
                }
            }
            ImGui::EndChild();
            ImGui::Separator();
            ImGui::BeginDisabled(!IsSourceValid);
            {
                if (ImGui::Button("Import", ImVec2(240, 0)))
                {
                    ProcessingDataImport();
                    FileToImport[0] = '\0';
                    IsSourceValid = false;
                    ImGui::CloseCurrentPopup();
                    IsModalOpen_ImportData = false;
                }
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(240, 0)))
            {
                ImGui::CloseCurrentPopup();
                IsModalOpen_ImportData = false;
            }
            if (ImGuiFileDialog::Instance()->Display("ChooseDataToImportDialogue", ImGuiWindowFlags_NoCollapse, ImVec2(800, 600))) 
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    std::string RawString = Utils::ChangePathSlash(ImGuiFileDialog::Instance()->GetFilePathName());
                    strcpy(FileToImport, RawString.c_str());
                    CheckIsDataValidToImport();
                }
                ImGuiFileDialog::Instance()->Close();
            }
            ImGui::EndPopup();
        }
    }


    void Modals::RenderSetting()
    {
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Setting", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (ImGui::TreeNodeEx("Appearance", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::BulletText("Choose Theme:");
                ImGui::SameLine();
                if (ImGui::RadioButton("Light", &ThemeToUse, 0))
                {
                    Setting::Get().Theme = ETheme::Light;
                    Style::ApplyTheme(ETheme::Light);
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Dark", &ThemeToUse, 1))
                {
                    Setting::Get().Theme = ETheme::Dark;
                    Style::ApplyTheme(ETheme::Dark);
                }
                ImGui::InputTextWithHint("Font file", "Default (NotoSansCJKtc-Black.otf)", &Setting::Get().Font, ImGuiInputTextFlags_ReadOnly);
                ImGui::SameLine();
                if (ImGui::Button("..."))
                {
                    ImGuiFileDialog::Instance()->OpenDialog("SetNewFontFile", "Choose font file", ".ttf,.otf", Setting::Get().ProjectPath,
                        1, nullptr, ImGuiFileDialogFlags_Modal);
                }
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_TIMES))
                {
                    if (!Setting::Get().Font.empty())
                    {
                        Setting::Get().Font.clear();
                        NeedToRestartApp = true;
                    }
                }
                if (NeedToRestartApp)
                {
                    ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Restart to activate change in font!");
                }
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx("Editor", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Auto Save", &Setting::Get().bEnableAutoSave);
                if (Setting::Get().bEnableAutoSave)
                {
                    ImGui::SameLine();
                    ImGui::Text(" (every ");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(100.f);
                    if (ImGui::InputInt(" seconds)", &Setting::Get().AutoSaveInterval))
                    {
                        if (Setting::Get().AutoSaveInterval < 5)
                            Setting::Get().AutoSaveInterval = 5;
                        else if (Setting::Get().AutoSaveInterval > 300)
                            Setting::Get().AutoSaveInterval = 300;
                    }
                }
                ImGui::Checkbox("Enable guide line", &Setting::Get().bEnableGuideLine);
                if (Setting::Get().bEnableGuideLine)
                {
                    ImGui::ColorEdit3("Guide line color", Setting::Get().GuidelineColor);
                }
                Utils::AddSimpleTooltip("Show guide line in annotation workspace?");
                if (ImGui::TreeNode("Advanced"))
                {
                    static int NumMaxCores = std::thread::hardware_concurrency();
                    if (ImGui::InputInt("Cores to load video", &Setting::Get().CoresToUse))
                    {
                        const int CoresToUse = Setting::Get().CoresToUse;
                        if (CoresToUse < 1) Setting::Get().CoresToUse = 1;
                        if (CoresToUse > NumMaxCores) Setting::Get().CoresToUse = NumMaxCores;
                    }
                    ImGui::Text("(Max number of cores in this machine: %d)", NumMaxCores);
                    ImGui::InputInt("Cached video frame size", &Setting::Get().MaxCachedFramesSize);
                    // per frame may use 2.64 mb of memory!
                    ImGui::Text("(May use %.1f GB memory from your total %.1f GB)", Setting::Get().MaxCachedFramesSize * 2.64f / 1024.f, Setting::Get().GetSystemMemorySize());
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx("Core path", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::BulletText("Yolo v7 Environment");
                ImGui::InputText("Python path", TempPythonPath, IM_ARRAYSIZE(TempPythonPath), ImGuiInputTextFlags_ReadOnly);
                ImGui::SameLine();
                if (ImGui::Button("..."))
                {
                    ImGuiFileDialog::Instance()->OpenDialog("ChoosePythonPath", "Choose python path", nullptr, ".", 1,
                        nullptr, ImGuiFileDialogFlags_Modal);
                }
                ImGui::InputText("Yolo v7 path", TempYoloV7Path, IM_ARRAYSIZE(TempYoloV7Path),
                                 ImGuiInputTextFlags_ReadOnly);
                ImGui::SameLine();
                if (ImGui::Button("..."))
                {
                    ImGuiFileDialog::Instance()->OpenDialog("ChooseYoloV7Path", "Choose yolo V7 path", nullptr, ".", 1, nullptr, ImGuiFileDialogFlags_Modal);
                }
                ImGui::TreePop();
            }
            
            if (ImGui::Button("OK", ImVec2(ImGui::GetWindowWidth() * 0.2f, ImGui::GetFontSize() * 1.5f)))
            {
                ImGui::CloseCurrentPopup();
                IsModalOpen_Setting = false;
            }
            // handle file dialogs...
            if (ImGuiFileDialog::Instance()->Display("ChoosePythonPath", ImGuiWindowFlags_NoCollapse, ImVec2(800, 600))) 
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    std::string RawString = Utils::ChangePathSlash(ImGuiFileDialog::Instance()->GetCurrentPath());
                    strcpy(TempPythonPath, RawString.c_str());
                    Setting::Get().PythonPath = RawString;
                }
                ImGuiFileDialog::Instance()->Close();
            }
            if (ImGuiFileDialog::Instance()->Display("ChooseYoloV7Path", ImGuiWindowFlags_NoCollapse, ImVec2(800, 600))) 
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    std::string RawString = Utils::ChangePathSlash(ImGuiFileDialog::Instance()->GetCurrentPath());
                    strcpy(TempYoloV7Path, RawString.c_str());
                    Setting::Get().YoloV7Path = RawString;
                }
                ImGuiFileDialog::Instance()->Close();
            }
            if (ImGuiFileDialog::Instance()->Display("SetNewFontFile", ImGuiWindowFlags_NoCollapse, ImVec2(800, 600))) 
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    Setting::Get().Font = Utils::ChangePathSlash(ImGuiFileDialog::Instance()->GetFilePathName());
                    NeedToRestartApp = true;
                }
                ImGuiFileDialog::Instance()->Close();
            }
            ImGui::EndPopup();
        }
    }

    void Modals::RenderDoc(const char* DocName)
    {
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(1920, 1080) * 0.6f);
        if (ImGui::BeginPopupModal(DocName))
        {
            ImVec2 ChildWindowSize = ImGui::GetContentRegionAvail();
            ChildWindowSize.y *= 0.93f;
            ImGui::BeginChild("Content", ChildWindowSize, false);
            {
                Markdown(LoadMDFile(DocName));
            }
            ImGui::EndChild();
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(ImGui::GetWindowWidth() * 0.2f, ImGui::GetFontSize() * 1.5f)))
            {
                ImGui::CloseCurrentPopup();
                IsModalOpen_About = false;
                IsModalOpen_Tutorial = false;
                IsModalOpen_Contact = false;
                IsModalOpen_License = false;
            }
            ImGui::EndPopup();
        }
    }

    void Modals::RenderDeleteModal(EAssetType AssetType)
    {
        // only supports for training set, model, and detection for now
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(384, 108));
        std::string ToDeleteName;
        switch (AssetType)
        {
        case EAssetType::TrainingSet:
            ToDeleteName = DataBrowser::Get().SelectedTrainingSet;
            break;
        case EAssetType::Model:
            ToDeleteName = DataBrowser::Get().SelectedModel;
            break;
        case EAssetType::Detection:
            ToDeleteName = DataBrowser::Get().SelectedDetection;
            break;
        default: ;
        }

        if (ImGui::BeginPopupModal("Delete Asset"))
        {
            ImGui::Text("Delete %s. Are you sure?", ToDeleteName.c_str());
            ImGui::Separator();
            if (ImGui::Button("OK"))
            {
                switch (AssetType)
                {
                case EAssetType::TrainingSet:
                    {
                        // modify yaml file
                        YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/TrainingSets.yaml");
                        YAML::Node NewData;
                        
                        for (YAML::const_iterator it=Data.begin();it!=Data.end();++it)
                        {
                            std::string Name = it->first.as<std::string>();
                            if (Name == DataBrowser::Get().SelectedTrainingSet) continue;
                            NewData[Name] = it->second;
                        }
                        std::ofstream ofs;
                        ofs.open(Setting::Get().ProjectPath + "/Data/TrainingSets.yaml");
                        YAML::Emitter Out;
                        Out << NewData;
                        ofs << Out.c_str();
                        ofs.close();
                        
                        // delete contents
                        auto async_delete = [this]()
                        {
                            std::filesystem::remove_all(Setting::Get().ProjectPath + "/Data/" + DataBrowser::Get().SelectedTrainingSet);
                        };
                        auto f = std::async(std::launch::async, async_delete);
                        DataBrowser::Get().SelectedTrainingSet.clear();
                        break;
                    }
                    
                case EAssetType::Model:
                    {
                        YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Models/Models.yaml");
                        YAML::Node NewData;
                        
                        for (YAML::const_iterator it=Data.begin();it!=Data.end();++it)
                        {
                            std::string Name = it->first.as<std::string>();
                            if (Name == DataBrowser::Get().SelectedModel) continue;
                            NewData[Name] = it->second;
                        }
                        std::ofstream ofs;
                        ofs.open(Setting::Get().ProjectPath + "/Models/Models.yaml");
                        YAML::Emitter Out;
                        Out << NewData;
                        ofs << Out.c_str();
                        ofs.close();
                        
                        // delete contents
                        std::filesystem::remove_all(Setting::Get().ProjectPath + "/Models/" + DataBrowser::Get().SelectedModel);
                        DataBrowser::Get().SelectedModel.clear();
                        break;
                    }
                case EAssetType::Detection:
                    {
                        YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Detections/Detections.yaml");
                        YAML::Node NewData;
                        for (YAML::const_iterator it=Data.begin();it!=Data.end();++it)
                        {
                            std::string Name = it->first.as<std::string>();
                            if (Name == DataBrowser::Get().SelectedDetection) continue;
                            NewData[Name] = it->second;
                        }
                        std::ofstream ofs;
                        ofs.open(Setting::Get().ProjectPath + "/Detections/Detections.yaml");
                        YAML::Emitter Out;
                        Out << NewData;
                        ofs << Out.c_str();
                        ofs.close();
                        
                        // delete contents
                        auto async_delete = [this]()
                        {
                            std::filesystem::remove_all(Setting::Get().ProjectPath + "/Detections/" + DataBrowser::Get().SelectedDetection);
                        };
                        auto f = std::async(std::launch::async, async_delete);
                        DataBrowser::Get().SelectedDetection.clear();
                        break;
                    }
                default: ;
                }
                DataBrowser::Get().ShouldViewDetail = false;
                ImGui::CloseCurrentPopup();
                IsModalOpen_Delete = false;
            }
            ImGui::SameLine(0, 24);
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
                IsModalOpen_Delete = false;
            }
        }
    }

    // TODO: maybe handle file dialog display at the place where the window is summoned?
    // or only leave the dialog that are used multiple times?
    
    void Modals::HandleFileDialogClose()
    {
        if (ImGuiFileDialog::Instance()->Display("ChooseExportAnnotationDialog", ImGuiWindowFlags_NoCollapse, ImVec2(800, 600)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                // TODO: Need test.. the emitter has some issue that saved the file as raw format... why??
                std::string ExportDir = Utils::ChangePathSlash(ImGuiFileDialog::Instance()->GetCurrentPath());
                YAML::Node OutNode;
                OutNode["SourceProject"] = Setting::Get().ProjectPath;
                YAML::Node AnnNode = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/Annotations.yaml");
                YAML::Node CatNode = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/Categories.yaml");
                OutNode["Annotations"] = AnnNode;
                OutNode["Categories"] = CatNode;
                YAML::Emitter Out;
                Out << OutNode;
                std::ofstream Fout(ExportDir + "/" + Setting::Get().Project + "_Exported.yaml");
                Fout << Out.c_str();
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ChooseExternalModelPath", ImGuiWindowFlags_NoCollapse, ImVec2(800, 600))) 
        {
            
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string RawString = Utils::ChangePathSlash(ImGuiFileDialog::Instance()->GetCurrentPath());
                strcpy(Train::Get().ExternalModelPath, RawString.c_str());
            }
            ImGuiFileDialog::Instance()->Close();
        }
        if (ImGuiFileDialog::Instance()->Display("SaveDeployConfigFile", ImGuiWindowFlags_NoCollapse, ImVec2(800, 600))) 
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                Deploy::Get().SaveConfigFile(Utils::ChangePathSlash(ImGuiFileDialog::Instance()->GetCurrentPath()));
            }
            ImGuiFileDialog::Instance()->Close();
        }
        if (ImGuiFileDialog::Instance()->Display("LoadDeployConfigFile", ImGuiWindowFlags_NoCollapse, ImVec2(800, 600))) 
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                Deploy::Get().LoadConfigFile(Utils::ChangePathSlash(ImGuiFileDialog::Instance()->GetFilePathName()));
            }
            ImGuiFileDialog::Instance()->Close();
        }
    }
}
