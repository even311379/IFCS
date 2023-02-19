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

        // setting options
        // appearance
        ETheme Theme = ETheme::Light;
        // editor
        int CoresToUse = 4;
        int MaxCachedFramesSize = 1500;
        bool bEnableAutoSave = true;
        int AutoSaveInterval = 30; // sec
        bool bEnableGuideLine = true;
        float GuidelineColor[3] = {0.f, 0.f, 0.f};
        // Core Path
        std::string PythonPath;
        std::string YoloV7Path;

        std::string ProjectPath;
        std::set<std::string> RecentProjects;
        std::string Project;
        EWorkspace ActiveWorkspace = EWorkspace::Data;
        ESupportedLanguage PreferredLanguage = ESupportedLanguage::English;
        bool ProjectIsLoaded = false;
        ImFont* DefaultFont;
        
        ImFont* TitleFont;
        ImFont* H1;
        ImFont* H2;
        ImFont* H3;
        void SetWorkspace(EWorkspace NewWorkspace);

        bool IsEnvSet() const;
        bool JustSetup = false;

        void SetupApp(class Application* InApp)
        {
            App = InApp;
        }

        float GetSystemMemorySize() const
        {
            return SystemMemorySize;
        }

    private:
        class Application* App;
        float SystemMemorySize;
    };
}
