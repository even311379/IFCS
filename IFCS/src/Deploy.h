#pragma once

#include "Panel.h"

namespace IFCS
{
    class Deploy : public Panel
    {
    public:
        MAKE_SINGLETON(Deploy)
        
    protected:
        void RenderContent() override;
        
    private:
        std::vector<std::string> SelectedCameras = {"", ""};
        std::vector<tm> SelectedDates = {tm()};
        void RenderAutomation();
        void RenderFeasibilityTest();
        void RenderDataPipeline();
        void RenderSMSNotification();
        void GenerateConfiguration();
        
    };
}
