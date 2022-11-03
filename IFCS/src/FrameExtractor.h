#pragma once
#include <vector>
#include "Panel.h"


namespace IFCS
{
    
    class FrameExtractor : public IFCS::Panel
    {
    public:
        MAKE_SINGLETON(FrameExtractor)
        void OnFrameLoaded();
        FClipInfo ClipInfo;
    protected:
        void RenderContent() override;

    private:
        // copied info
        bool ClipInfoIsLoaded = false;
        
        std::vector<float> Regions;
        bool JustDeleteRegion;
        // due to float or int issue?... it' true float to int is the cause... let's stick on float... 

        // timeline from https://github.com/ocornut/imgui/issues/76#issuecomment-287739415 as template and modified a lot
        bool BeginTimeline(int opt_exact_num_rows=0);
        bool TimelineEvent(int N);
        void EndTimeline();
        void TimelineControlButtons();
        void ProcessingVideoPlay();
        
        int TimelineRows;
        const float TimelineDisplayStart = 0.f; // in pixel? .. nothing to do with real time stamp
        const float TimelineDisplayEnd = 100.f; // in pixel? just an arbitrary value...
        int temp_TimelineIndex = -1;
        float TimelineZoom = 1.0f;  // 1 ~ 50
        float TimelinePan = 0.0f; // 0.0 ~ 1.0f

        void CheckZoomInput();

        bool IsPlaying = false;
        bool JustPlayed = false;
        bool JustPaused = false;
        int CurrentFrameStart;
        int CurrentFrameRange;
        int CurrentFrame;
        int CurrentFrameEnd;
        void UpdateCurrentFrameInfo(); // update above 4 param considering pan and zoom...
        float PlayProgress = 0.f; //0 ~ 100

        // video actions:
        int NumFramesToExtract = 1;

        // extraction
        uint8_t ExtractionMode;
        // std::string ExtractionOptions;
        void PerformExtraction();
    };
}
