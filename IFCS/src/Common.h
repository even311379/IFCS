#pragma once
#include <string>

#include "imgui.h"
#include "UUID.h"

/*
 * defines common enum structs here...
 */

enum class EWorkspace : uint8_t
{
    Data = 0,
    Train = 1,
    Predict = 2,
};
    

// the value is the column position in csv...
enum class ESupportedLanguage : uint8_t
{
    English = 0,
    TraditionalChinese = 1,
};
    
enum class ETheme : uint8_t
{
    Light = 0,
    Dark = 1,
};

enum class ELogLevel : uint8_t
{
    Info = 0,
    Warning = 1,
    Error = 2,
};

struct FLogData
{
    ELogLevel LogLevel = ELogLevel::Info;
    std::string TimeString;
    std::string Message;
    bool ShouldDisplay;
};

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


struct FCategory
{
    IFCS::UUID uid;
    std::string DisplayName;
    ImVec4 Color;
    IFCS::UUID parent_uid = 0;
    bool Visibility = true;
    

    // info
    int UsedCountInThisFrame = 0;
    int TotalUsedCount = 100;

    FCategory(std::string InName, ImVec4 InColor, IFCS::UUID InParent, bool InVisibility = true)
        : DisplayName(InName), Color(InColor), parent_uid(InParent), Visibility(InVisibility)
    {}
    inline bool HasParent() const { return parent_uid != 0; }
};
