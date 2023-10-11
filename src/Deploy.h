#pragma once

#include "Panel.h"
#include "IFCS_Types.h"

namespace IFCS
{
    class Deploy : public Panel
    {
    public:
        MAKE_SINGLETON(Deploy)
        void LoadConfigFile(const std::string& ConfigFilePath);
        void SaveConfigFile(const std::string& ConfigFilePath);

    protected:
        void RenderContent() override;
        
    private:
        void RenderInputOutput();
        void RenderConfigureTask();
        void RenderNotification();
        FDeploymentData Data;

        
    };
}
