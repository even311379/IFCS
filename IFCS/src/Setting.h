#pragma once
#include "Common.h"
// #include <unordered_map>



namespace IFCS 
{
    /*
     * Meyers Singleton
     * Ref from: https://shengyu7697.github.io/cpp-singleton-pattern/
     */
    class Setting
    {
    public:
        static Setting& Get()
        {
            static Setting setting;
            return setting;
        }

        Setting(Setting const&) = delete;
        void operator=(Setting const&) = delete;

        char* ActiveWks = "Annoation";
        ESupportedLanguage CurrentLanguage = ESupportedLanguage::English;
        // void RegisterFont(std::string FontName, ImFont* NewFont);
        // ImFont* GetFont(std::string FontName);
        
    private:
        // std::unordered_map<std::string, ImFont*> RegisteredFont; 
        Setting() {}
        
    };
    
}
