#pragma once

#include "Panel.h"

namespace IFCS
{
    class Deploy : public Panel
    {
    public:
        MAKE_SINGLETON(Deploy)
        void LoadConfigFile(const std::string& ConfigFilePath);
        void SaveConfigFile(const std::string& ConfigFilePath);
        // void SetExternalModelFile(const std::string& NewModelFile);
        void GenerateRunScript(const std::string& ScriptsLocation);

    protected:
        void RenderContent() override;
        
    private:
        void RenderInputOutput();
        void RenderConfigureTask();
        void RenderNotification();
        std::string OutputDir;
        FDeploymentData Data;
        std::vector<std::string> InternalModelOptions;

        
    };
}
