#pragma once
#include <set>
#include <string>
#include <vector>

#include "Panel.h"

namespace IFCS
{
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

    /*
     * TODO: focus on recent only 
     */

    
    class LogPanel : public Panel
    {
    public:
        void ClearDisplay();
        void FilterData();
        void ClearData();
        void AddLog(ELogLevel Level, const char* Message);
        
    protected:
        void RenderContent() override;
    private:
        std::vector<FLogData> Data;
        bool bShowInfo = true;
        bool bShowWarning = true;
        bool bShowError = true;
        char sFilterMessage[128] = "";
    };

}
