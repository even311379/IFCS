#include "Setting.h"

#include <filesystem>
#include <fstream>
#include <regex>
#include <spdlog/spdlog.h>

#include "yaml-cpp/yaml.h"

#include "Style.h"
#include "Application.h"
#include "Modals.h"
#include "Annotation.h"
#include "CategoryManagement.h"
#include "DataBrowser.h"
#include "Deploy.h"
#include "Train.h"
#include "Detection.h"



namespace IFCS
{
    Setting::Setting()
    {
    }


    /*
     * Modal might fail due tp ID stack issue... use specified boolean to control it could help!
     */

    void Setting::LoadEditorIni()
    {
        YAML::Node EditorIni = YAML::LoadFile("Config/Editor.ini");
        if (!EditorIni["LastOpenProject"])
        {
            Style::ApplyTheme();
            return;
        }
        if (!EditorIni["RecentProjects"])
        {
            Style::ApplyTheme();
            return;
        }
        ProjectPath = EditorIni["LastOpenProject"].as<std::string>();
        ProjectPath = std::regex_replace(ProjectPath, std::regex("/$"), "");
        if (!std::filesystem::exists(ProjectPath))
        {
            Style::ApplyTheme();
            ProjectPath = "";
            return;
        }
            
        if (EditorIni["RecentProjects"].IsSequence())
        {
            for (std::size_t i = 0; i < EditorIni["RecentProjects"].size(); i++)
            {
                if (std::filesystem::exists(EditorIni["RecentProjects"][i].as<std::string>()))
                {
                    auto RecentProject = EditorIni["RecentProjects"][i].as<std::string>();
                    RecentProject = std::regex_replace(RecentProject, std::regex("/$"), "");
                    RecentProjects.insert(RecentProject);
                }
            }
        }
        LoadUserIni();

// TODO: get linux alternative for this
#if defined _WINDOWS
        // should move to a better position for these code?
        /// Get total memory for that machine
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        SystemMemorySize = status.ullTotalPhys / 1024 / 1024 / 1024.f;
#endif        
        ///
    }

    void Setting::LoadUserIni()
    {
        YAML::Node UserIni = YAML::LoadFile(ProjectPath + std::string("/IFCSUser.ini"));
        Project = UserIni["Project"].as<std::string>();
        PreferredLanguage = static_cast<ESupportedLanguage>(UserIni["PreferredLanguage"].as<int>());
        ActiveWorkspace = static_cast<EWorkspace>(UserIni["ActiveWorkspace"].as<int>());
        Theme = static_cast<ETheme>(UserIni["Theme"].as<int>());
        if (UserIni["Font"])
        {
            Font = UserIni["Font"].as<std::string>();
        }
        PythonPath = UserIni["PythonPath"].as<std::string>();
        YoloV7Path = UserIni["YoloV7Path"].as<std::string>();
        if (UserIni["bEnableGuideLine"])
        {
            bEnableGuideLine = UserIni["bEnableGuideLine"].as<bool>();
            const std::vector<float> VGuidelineColor = UserIni["GuideLineColor"].as<std::vector<float>>();
            GuidelineColor[0] = VGuidelineColor[0];
            GuidelineColor[1] = VGuidelineColor[1];
            GuidelineColor[2] = VGuidelineColor[2];
        }
        if (UserIni["bEnableAutoSave"])
        {
            bEnableAutoSave = UserIni["bEnableAutoSave"].as<bool>();
            AutoSaveInterval = UserIni["AutoSaveInterval"].as<int>();
        }
        if (UserIni["CoresToUse"])
        {
            CoresToUse = UserIni["CoresToUse"].as<int>();
            MaxCachedFramesSize = UserIni["MaxCachedFrameSize"].as<int>();
        }
        Style::ApplyTheme(Theme);
        Modals::Get().Sync();
        ProjectIsLoaded = true;
    }


    void Setting::Save()
    {
        // save editor.ini
        YAML::Emitter editor_out;
        editor_out << YAML::BeginMap;
        editor_out << YAML::Key << "LastOpenProject";
        editor_out << YAML::Value << ProjectPath;
        editor_out << YAML::Key << "RecentProjects";
        editor_out << YAML::BeginSeq;
        for (const auto& p : RecentProjects)
        {
            editor_out << p;
        }
        editor_out << YAML::EndSeq;
        editor_out << YAML::Key << "Window";
        editor_out << YAML::BeginSeq;
        editor_out << App->WindowPosX;
        editor_out << App->WindowPosY;
        editor_out << App->WindowWidth;
        editor_out << App->WindowHeight;
        editor_out << YAML::EndSeq;

        std::ofstream fout("Config/Editor.ini");
        fout << editor_out.c_str();

        // save user.init
        YAML::Emitter user_out;
        YAML::Node OutNode;
        OutNode["Project"] = Project;
        OutNode["ActiveWorkspace"] = static_cast<int>(ActiveWorkspace);
        OutNode["PreferredLanguage"] = static_cast<int>(PreferredLanguage);
        OutNode["Theme"] = static_cast<int>(Theme);
        OutNode["Font"] = Font;
        OutNode["PythonPath"] = PythonPath;
        OutNode["YoloV7Path"] = YoloV7Path;
        OutNode["bEnableGuideLine"] = bEnableGuideLine;
        std::vector<float> VGuideLineColor;
        VGuideLineColor.push_back(GuidelineColor[0]);
        VGuideLineColor.push_back(GuidelineColor[1]);
        VGuideLineColor.push_back(GuidelineColor[2]);
        OutNode["GuideLineColor"] = VGuideLineColor;
        OutNode["bEnableAutoSave"] = bEnableAutoSave;
        OutNode["AutoSaveInterval"] = AutoSaveInterval;
        OutNode["CoresToUse"] = CoresToUse;
        OutNode["MaxCachedFrameSize"] = MaxCachedFramesSize;
        user_out << OutNode;

        // save as local file
        std::ofstream fout_2(ProjectPath + std::string("/IFCSUser.ini"));
        fout_2 << user_out.c_str();
        ProjectIsLoaded = true;
    }

    void Setting::Tick1()
    {
        SetWorkspace(ActiveWorkspace);
    }

    void Setting::CreateStartup()
    {
        RecentProjects.insert(ProjectPath);
        std::filesystem::create_directories(ProjectPath); // TODO: can not use chinese folder name??
        std::filesystem::create_directories(ProjectPath + std::string("/Clips"));
        std::filesystem::create_directories(ProjectPath + std::string("/Images"));
        std::filesystem::create_directories(ProjectPath + std::string("/Models"));
        std::filesystem::create_directories(ProjectPath + std::string("/Detections"));
        std::filesystem::create_directories(ProjectPath + std::string("/Data"));
        std::ofstream output1(ProjectPath + std::string("/Data/Annotations.yaml"));
        std::ofstream output2(ProjectPath + std::string("/Data/Categories.yaml"));
        std::ofstream output4(ProjectPath + std::string("/Data/TrainingSets.yaml"));
        std::ofstream output5(ProjectPath + std::string("/Models/Models.yaml"));
        std::ofstream output6(ProjectPath + std::string("/Detections/Detections.yaml"));
        Save();
        JustSetup = true;
    }

    void Setting::StartFromPreviousProject()
    {
        RecentProjects.insert(ProjectPath);
        EWorkspace CachedWorkspace = ActiveWorkspace;
        LoadUserIni();
        ActiveWorkspace = CachedWorkspace;
        Annotation::Get().ClearData();
        CategoryManagement::Get().UpdateCategoryStatics();
        CategoryManagement::Get().SelectedCatID = 0;
        DataBrowser::Get().PostChangeProject();
        Annotation::Get().LoadDataFile();
        App->RequestToChangeTitle = true;
        JustSetup = true;
    }

    void Setting::SetWorkspace(EWorkspace NewWorkspace)
    {
        Annotation::Get().SetVisibility(false);
        CategoryManagement::Get().SetVisibility(false);
        Train::Get().SetVisibility(false);
        Detection::Get().SetVisibility(false);
        Deploy::Get().SetVisibility(false);
        DataBrowser::Get().SetVisibility(true);
        ActiveWorkspace = NewWorkspace;
        switch (ActiveWorkspace)
        {
        case EWorkspace::Data:
            Annotation::Get().SetVisibility(true);
            CategoryManagement::Get().SetVisibility(true);
            break;
        case EWorkspace::Train:
            Train::Get().SetVisibility(true);
            break;
        case EWorkspace::Detect:
            Detection::Get().SetVisibility(true);
            break;
        case EWorkspace::Deploy:
            Deploy::Get().SetVisibility(true);
            break;
        }
        App->RequestToUpdateWorkSpace = true;
    }

    bool Setting::IsEnvSet() const
    {
        return !PythonPath.empty() && !YoloV7Path.empty();
    }

    // TODO: leave for extension if I really need it...
    /*void Setting::DownloadYoloV7()
    {
    }

    void Setting::CreateEnv()
    {
    }*/
}
