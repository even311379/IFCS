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

    protected:
        void RenderContent() override;
        
    private:
        void RenderAutomation();
        
        int CurrentRefImgIndex = 0;
        char ReferenceImagePath[255] = "";
        std::vector<std::string> ReferenceImages;
        unsigned int ReferenceImagePtr;
        
        void CleanRegisteredCameras();
        
        void RenderFeasibilityTest();
        void RenderAddingProgress(ImVec2 StartPos);
        cv::Mat ImgData;
        void UpdateReferenceImage();
        ImColor GetAverageColor(const std::array<float, 4>& InXYWH);
        void RenderTestZones(ImVec2 StartPos);
        bool IsTestZonesPassRefImg();
        bool IsColorInRange(ImVec4 TestColor, ImVec4 LowerColor, ImVec4 UpperColor);

        // std::map<std::string, FModelParameters> ModelParameters;
        void RenderDataPipeline();

        // TODO: use pre-defined receiver groups?
        // std::map<std::string, std::string> ReceiverGroups; //receiever name -> group name
        void RenderSMSNotification();
        
        void GenerateScript();

        FDeploymentData Data;
        
    };
}
