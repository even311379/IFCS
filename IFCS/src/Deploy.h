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
    protected:
        void RenderContent() override;
        
    private:
        char OutputPath[255] = "";
        char InputPath[255] = "";
        std::vector<std::string> SelectedCameras = {"", ""};
        std::vector<tm> SelectedDates = {tm()};
        void RenderAutomation();
        int CurrentRefImgIndex = 0;
        char ReferenceImagePath[255] = "";
        std::vector<std::string> ReferenceImages;
        unsigned int ReferenceImagePtr;
        std::map<std::string, std::vector<FFeasibleZone>> FeasibleZones;
        void RenderFeasibilityTest();
        void UpdateReferenceImage();
        ImVec4 GetAverageColor(const std::array<float, 4>& InXYWH);
        void RenderTestZones(ImVec2 StartPos);
        void RenderAddingProgress(ImVec2 StartPos);
        void RenderDataPipeline();
        void RenderSMSNotification();
        void GenerateScript();
        
    };
}
