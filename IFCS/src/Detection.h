#pragma once
#include <future>
#include <map>

#include "Panel.h"
#include "Style.h"
#include "Utils.h"

namespace cv
{
    class Mat;
}

namespace YAML
{
    class Node;
}

namespace IFCS
{
    struct FCategoryData
    {
        std::string CatName;
        ImVec4 CatColor;
        FCategoryData()=default;
        FCategoryData(const std::string& InCatName, const ImVec4& InColor)
            : CatName(InCatName), CatColor(InColor)
        {}
    };
    
    class Detection : public Panel
    {
    public:
        MAKE_SINGLETON(Detection)
        void Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose = true) override;
        void ClearCacheIndividuals();
    protected:

        void RenderContent() override;
    private:
        // detection params
        char SelectedModel[128];
        char SelectedClip[128];
        int ImageSize = 640;
        float Confidence = 0.25f;
        float IOU = 0.45f;
        std::string DetectionName;
        bool IsDetectionNameUsed;
        bool IsDetecting;
        std::future<void> DetectFuture;
        bool ReadyToDetect();
        void UpdateDetectionScript();
        std::string SetPathScript;
        std::string DetectScript;
        std::string SaveToProjectScript;
        void MakeDetection();
        std::string DetectionLog;

        // analysis param
        char SelectedDetection[128];
        int SelectedAnalysisType = 0;
        std::string DetectionClip;
        std::map<int, std::vector<FLabelData>> LoadedLabels;
        std::vector<FCategoryData> Categories; // exported format...
        // clip info 
        float ClipFPS = 30.0f;
        int ClipWidth;
        int ClipHeight;
        unsigned int LoadedFramePtr;
        void RenderDetectionBox(ImVec2 StartPos);
        void UpdateFrame(int FrameNumber, bool UpdateClipInfo = false);
        void ProcessingVideoPlay();
        void UpdateFrameImpl(cv::Mat Data);
        bool IsPlaying = false;
        bool JustPlayed = false;
        bool JustPaused = false;
        
        // int SelectedDetection;
        
        int CurrnetPlayPosition = 0;
        int PlayOffset = 0;

        void DrawPlayRange();
        int StartFrame = 1;
        int EndFrame = 100;
        int CurrentFrame = 1;
        int TotalClipFrameSize = 100;
        bool DisplayHelperLines = true;
        ImVec4 HintColor = Style::RED();
        void RenderAnaylysisWidgets_Pass();
        bool IsVertical = false;
        // todo: prevent these two value the same...
        // maybe one point is enough? no... Using two points also contains the idea of ROI...
        float FishWayPos[2] = {0.2f, 0.8f};
        bool IsIndividualDataLatest = false;
        bool IsSizeSimilar(const FLabelData& Label1, const FLabelData& Label2);
        void TrackIndividual();
        void GenerateCachedIndividualImages();
        std::vector<FIndividualData> IndividualData;
        std::map<std::string, int> TotalPass;
        std::map<std::string, int> CurrentPass;
        void UpdateAccumulatedPasses();
        int GetMaxPass();
        
        // TODO: how to store the pass data
        void RenderAnaylysisWidgets_InScreen();
        bool bSetROI = false;
        float RoiRegion[4] = {0.1f, 0.1f, 0.9f, 0.9f};
        std::map<std::string, std::vector<int>> InScreenRoiData;
        void UpdateRoiScreenData();
        std::map<std::string, int> CurrentCatCount;
        void UpdateCurrentCatCount();
        void Analysis(const std::string& DName, const YAML::Node& DataNode);
        bool IsAnalyzing = false;
        std::future<void> AnalyzeFuture;
    };
}
