#pragma once
#include "Common.h"
#include <set>
// #include <unordered_map>

struct ImFont;

namespace IFCS 
{
    /*
     * Meyers Singleton
     * Ref from: https://shengyu7697.github.io/cpp-singleton-pattern/
     */
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


        void RenderModal();
        
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
        bool IsChoosingFolder;

        void SetWorkspace(EWorkspace NewWorkspace);

        bool IsModalOpen;
        std::string PythonPath;
        std::string YoloV7Path;
        char TempPythonPath[128];
        char TempYoloV7Path[128];
        char TempPythonEnv[128];
        bool IsEnvSet() const;
        bool JustSetup = false;
    private:
        // modal  vars
        int ThemeToUse = 0;
        int LanguageToUse = 0;
        int AppSizeToUse = 0;
        int CustomWidth = 1280;
        int CustomHeight = 720;
        float WidgetResizeScale = 1.f;
        float GlobalFontScaling = 1.f;
        
        
        
        // void DownloadYoloV7();
        // void CreateEnv();
    };
   // TODO: last edit frame and clip?     
    
}
