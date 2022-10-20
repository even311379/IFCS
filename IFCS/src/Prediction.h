#pragma once
#include "Panel.h"
#include "Utils.h"
#include "ImguiNotify/font_awesome_5.h"

namespace IFCS
{
    struct FAnalysisResult
    {
        std::string CategoryDisplayName;
        ImVec4 Color;
        std::vector<float> MeanNums;
        std::vector<int> PassNums;
    };


    class Prediction : public Panel
    {
    public:
        MAKE_SINGLETON(Prediction)
    protected:
        void RenderContent() override;
    private:
        int SelectedModelIdx;
        int SelectedClipIdx;
        float Confidence = 0.25f;
        void MakePrediction();

        
        int SelectedPrediction;
        char* PlayIcon = ICON_FA_PLAY;
        bool IsPlaying = false;
        int CurrnetPlayPosition = 0;
        int MaxDisplayNums = 10;
        int PlayOffset = 0;


        void DrawPlayRange();
        float PlayTimePos1 = 0.f;
        float PlayTimePos2 = 1.f;
        float FishWayStartP1[2];
        float FishWayStartP2[2];
        float FishWayEndP1[2];
        float FishWayEndP2[2];
        int PUFS = 1; // per unit frame size
        int TotalClipFrameSize = 100;

        void Analysis();
    public:
        void Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose = true) override;
    private:
        std::vector<FAnalysisResult> Results;

        // void Sparkline(const char* id, const float* values, int count, float min_v, float max_v, int offset,
        //                const ImVec4& col, const ImVec2& size);
    };
}
