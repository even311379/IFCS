#pragma once
#include <vector>
#include "Utils.h"

#include "Panel.h"

namespace IFCS
{

    /*
     * TODO: focus on recent only
     * make this singleton??
     */

    
    class LogPanel : public Panel
    {
    public:
        MAKE_SINGLETON(LogPanel)
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
