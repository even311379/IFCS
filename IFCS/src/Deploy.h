#pragma once

#include "Panel.h"

namespace IFCS
{
    class Deploy : public Panel
    {
    public:
        MAKE_SINGLETON(Deploy)
        void SetInputPath(const char* NewPath);
        void SetOutputPath(const char* NewPath);
        void SetReferenceImagePath(const char* NewPath);
        void LoadConfigFile(const std::string& ConfigFilePath);
        void SaveConfigFile(const std::string& ConfigFilePath);
        void SetExternalModelFile(const std::string& NewModelFile);
        void GenerateRunScript(const std::string& ScriptsLocation);

    protected:
        void RenderContent() override;
        
    private:
        void RenderAutomation();
        
        int CurrentRefImgIndex = 0;
        char ReferenceImagePath[255] = "";
        std::vector<std::string> ReferenceImages;
        unsigned int ReferenceImagePtr;
        
        void CleanRegisteredCameras();
        
        size_t CameraIndex_FT = 0;
        void RenderFeasibilityTest();
        void RenderAddingProgress(ImVec2 StartPos);
        cv::Mat ImgData;
        void UpdateReferenceImage();
        ImColor GetAverageColor(const std::array<float, 4>& InXYWH);
        void RenderTestZones(ImVec2 StartPos);
        bool IsTestZonesPassRefImg();
        bool IsColorInRange(ImVec4 TestColor, ImVec4 LowerColor, ImVec4 UpperColor);

        size_t CameraIndex_DP = 0;
        void RenderDataPipeline();
        void RenderSMSNotification();

        FDeploymentData Data;
        
    };
}
