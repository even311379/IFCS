#pragma once
#include "Common.h"
#include <set>
// #include <unordered_map>

struct ImFont;

namespace IFCS 
{

    class Setting
    {
    public:
        Setting();
        
        static Setting& Get()
        {
            static Setting setting;
            return setting;
        }

        Setting(Setting const&) = delete;
        void operator=(Setting const&) = delete;

        void LoadEditorIni();
        void LoadUserIni();
        void Save();
        void Tick1();
        void CreateStartup();
        void StartFromPreviousProject();
        std::string ProjectPath;
        std::set<std::string> RecentProjects;
        std::string Project;
        EWorkspace ActiveWorkspace = EWorkspace::Data;
        ESupportedLanguage PreferredLanguage = ESupportedLanguage::English;
        ETheme Theme = ETheme::Light;
        bool ProjectIsLoaded = false;

        ImFont* DefaultFont;
        ImFont* TitleFont;

        void SetWorkspace(EWorkspace NewWorkspace);

        std::string PythonPath;
        std::string YoloV7Path;
        bool IsEnvSet() const;
        bool JustSetup = false;

        // void DownloadYoloV7();
        // void CreateEnv();
    };
    
}
