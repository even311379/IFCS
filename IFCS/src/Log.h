#pragma once
#include <vector>
#include "Common.h"

#include "Panel.h"

namespace IFCS
{

    /*
     * TODO: focus on recent only, sync with setting singleton?? 
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
