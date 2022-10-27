﻿#pragma once
#include <future>
#include <map>

#include "Panel.h"
#include "Style.h"
#include "Utils.h"

namespace cv
{
    class Mat;
}

namespace IFCS
{
    struct FAnalysisResult
    {
        std::string CategoryDisplayName;
        ImVec4 Color;
        std::vector<float> MeanNums;
        std::vector<int> PassNums;
    };

    struct FLabelData
    {
        int CatID;
        float X, Y, Width, Height, Conf;
        FLabelData() = default;
        FLabelData(const int& InCatID, const float& InX, const float& InY, const float& InWidth, const float& InHeight,
            const float& InConf)
            : CatID(InCatID), X(InX), Y(InY), Width(InWidth), Height(InHeight), Conf(InConf)
        {}
    };

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
        float ClipFPS = 30.0f;
        unsigned int LoadedFramePtr;
        void RenderDetectionBox(ImVec2 StartPos);
        void UpdateFrame(int FrameNumber, bool UpdateClipInfo = false);
        void ProcessingVideoPlay();
        void FrameToGL(cv::Mat Data, int FrameCount);
        bool IsPlaying = false;
        bool JustPlayed = false;
        bool JustPaused = false;
        
        // int SelectedDetection;
        
        int CurrnetPlayPosition = 0;
        int MaxDisplayNums = 10;
        int PlayOffset = 0;

        void DrawPlayRange();
        int StartFrame = 1;
        int EndFrame = 100;
        int CurrentFrame = 50;
        void RenderAnaylysisWidgets_Pass();
        void RenderAnaylysisWidgets_InScreen();
        bool IsVertical = false;
        float FishWayPos[2] = {0.2f, 0.8f};
        ImVec4 FishWayHintColor = Style::RED();
        // float FishWayStartP1[2] = {0, 0};
        // float FishWayStartP2[2] = {0, 1};
        // float FishWayEndP1[2] = {1, 0};
        // float FishWayEndP2[2] = {1, 1};
        // int PUFS = 1; // per unit frame size
        int TotalClipFrameSize = 100;

        void Analysis();
    public:
        void Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose = true) override;
    private:
        std::vector<FAnalysisResult> Results;

        // void Sparkline(const char* id, const float* values, int count, float min_v, float max_v, int offset,
        //                const ImVec4& col, const ImVec2& size);

        // int VW;
        // int VH;
    };
}
