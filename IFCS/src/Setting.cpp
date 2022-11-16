#include "Setting.h"
#include "Style.h"
#include "Modals.h"

#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

#include "Annotation.h"
#include "CategoryManagement.h"
#include "DataBrowser.h"
#include "Train.h"
#include "Panel.h"
#include "Detection.h"
#include "TrainingSetGenerator.h"
#include "yaml-cpp/yaml.h"

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
        if (EditorIni["RecentProjects"].IsSequence())
        {
            for (std::size_t i = 0; i < EditorIni["RecentProjects"].size(); i++)
            {
                RecentProjects.insert(EditorIni["RecentProjects"][i].as<std::string>());
            }
        }
        LoadUserIni();
    }

    void Setting::LoadUserIni()
    {
        if (ProjectPath == "null") return;
        YAML::Node UserIni = YAML::LoadFile(ProjectPath + std::string("/IFCSUser.ini"));
        Project = UserIni["Project"].as<std::string>();
        PreferredLanguage = static_cast<ESupportedLanguage>(UserIni["PreferredLanguage"].as<int>());
        ActiveWorkspace = static_cast<EWorkspace>(UserIni["ActiveWorkspace"].as<int>());
        Theme = static_cast<ETheme>(UserIni["Theme"].as<int>());
        PythonPath = UserIni["PythonPath"].as<std::string>();
        YoloV7Path = UserIni["YoloV7Path"].as<std::string>();
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

        std::ofstream fout("Config/Editor.ini");
        fout << editor_out.c_str();

        // TODO: maybe use enum string?   or not?
        // save user.init
        YAML::Emitter user_out;
        YAML::Node OutNode;
        OutNode["Project"] = Project;
        OutNode["ActiveWorkspace"] = static_cast<int>(ActiveWorkspace);
        OutNode["PreferredLanguage"] = static_cast<int>(PreferredLanguage);
        OutNode["Theme"] = static_cast<int>(Theme);
        OutNode["PythonPath"] = PythonPath;
        OutNode["YoloV7Path"] = YoloV7Path;
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
        LoadUserIni();
    }

    void Setting::SetWorkspace(EWorkspace NewWorkspace)
    {
        Annotation::Get().SetVisibility(false);
        CategoryManagement::Get().SetVisibility(false);
        TrainingSetGenerator::Get().SetVisibility(false);
        Train::Get().SetVisibility(false);
        Detection::Get().SetVisibility(false);
        DataBrowser::Get().SetVisibility(true);
        ActiveWorkspace = NewWorkspace;
        switch (ActiveWorkspace)
        {
        case EWorkspace::Data:
            Annotation::Get().SetVisibility(true);
            TrainingSetGenerator::Get().SetVisibility(true);
            CategoryManagement::Get().SetVisibility(true);
            BGPanel::Get().SetDataWksNow = true;
            break;
        case EWorkspace::Train:
            Train::Get().SetVisibility(true);
            BGPanel::Get().SetTrainWksNow = true;
            break;
        case EWorkspace::Detect:
            Detection::Get().SetVisibility(true);
            BGPanel::Get().SetPredictWksNow = true;
            break;
        }
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
