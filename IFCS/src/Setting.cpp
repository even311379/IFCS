#include "Setting.h"

#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

#include "yaml-cpp/yaml.h"

namespace IFCS
{
    Setting::Setting()
    {
    }

    void Setting::LoadEditorIni()
    {
        YAML::Node EditorIni = YAML::LoadFile("Config/Editor.ini");
        if (!EditorIni["LastOpenProject"]) return;
        if (!EditorIni["RecentProjects"]) return;
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
        YAML::Node UserIni = YAML::LoadFile(ProjectPath + std::string("/IFCSUser.ini"));
        Project = UserIni["Project"].as<std::string>();
        PreferredLanguage = static_cast<ESupportedLanguage>(UserIni["PreferredLanguage"].as<int>());
        ActiveWorkspace = static_cast<EWorkspace>(UserIni["ActiveWorkspace"].as<int>());
        Theme = static_cast<ETheme>(UserIni["Theme"].as<int>());
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
        user_out << YAML::BeginMap;
        user_out << YAML::Key << "Project";
        user_out << YAML::Value << Project;
        user_out << YAML::Key << "ActiveWorkspace";
        user_out << YAML::Value << static_cast<int>(ActiveWorkspace);
        user_out << YAML::Key << "PreferredLanguage";
        user_out << YAML::Value << static_cast<int>(PreferredLanguage);
        user_out << YAML::Key << "Theme";
        user_out << YAML::Value << static_cast<int>(Theme);

        // save as local file
        std::ofstream fout_2(ProjectPath + std::string("/IFCSUser.ini"));
        fout_2 << user_out.c_str();
        ProjectIsLoaded = true;
    }

    void Setting::CreateStartup()
    {
        RecentProjects.insert(ProjectPath);
        std::filesystem::create_directories(ProjectPath); // TODO: can not use chinese folder name
        std::filesystem::create_directories(ProjectPath + std::string("/training_clips")); // lack of ..."i"...tra_ning ... waste me 2 hours...
        std::filesystem::create_directories(ProjectPath + std::string("/training_images"));
        std::filesystem::create_directories(ProjectPath + std::string("/models")); // TODO: check what to create in later dev cycle
        std::filesystem::create_directories(ProjectPath + std::string("/prediction"));
        Save();
    }
}
