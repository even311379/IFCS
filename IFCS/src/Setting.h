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

        
        void LoadEditorIni();
        void LoadUserIni();
        void Save();
        void CreateStartup();
        std::string ProjectPath;
        std::set<std::string> RecentProjects;
        std::string Project;
        EWorkspace ActiveWorkspace = EWorkspace::Data;
        ESupportedLanguage PreferredLanguage = ESupportedLanguage::English;
        ETheme Theme = ETheme::Light;
        bool ProjectIsLoaded = false;

        ImFont* DefaultFont;
        ImFont* TitleFont;
        
    private:
        // std::unordered_map<std::string, ImFont*> RegisteredFont;
        // load setting vars from init file
        
        
    };
    
}
