# pragma once


#include <string>
#include <spdlog/spdlog.h>
#include "Common.h"


/*
 * TODO: as long as I convert string to c_str() inside this utility function rather then in ImGui implementation...
 * the encoding just fucked up... no way to fix it...
 */

namespace IFCS
{
    namespace Utils
    {
        char* GetCurrentTimeString();

        typedef std::vector<std::vector<std::string>> CSV;

        CSV LoadCSV(const char* FilePath, bool bContainsHeader = true);

        const std::string GetLocText(const char* LocID, ESupportedLanguage language);

        const std::string GetLocText(const char* LocID);

        void AddSimpleTooltip(const char* Desc, float WrapSize = 35.0f);

        template <typename C, typename T>
        bool Contains(C&& c, T e)
        {
            return std::find(std::begin(c), std::end(c), e) != std::end(c);
        };

        std::vector<std::string> Split(const std::string& str, const char& delimiter);

        ImVec4 RandomPickColor(bool IsLight = true);

        int RandomIntInRange(int Min, int Max);
        bool RandomBool();
    }
}

#define MAKE_SINGLETON(class_name) \
    class_name()=default; \
    static class_name& Get() \
    { \
        static class_name singleton; \
        return singleton; \
    }\
    class_name(class_name const&) = delete;\
    void operator=(class_name const&) = delete;\
    

#define LOCTEXT(key) Utils::GetLocText(key).c_str()
