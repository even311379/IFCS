#pragma once
#include <string>

/*
 * defines common enum structs here...
 */


// the value is the column position in csv...
enum class ESupportedLanguage 
{
    English = 1,
    TraditionalChinese = 2,
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
