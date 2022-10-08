#pragma once
#include <vector>
#include "Panel.h"
#include "ImguiNotify/font_awesome_5.h"

// forward declare with namesapce...
namespace cv
{
    class Mat;
}

namespace IFCS
{
    class FrameExtractor : public IFCS::Panel
    {
    public:
        MAKE_SINGLETON(FrameExtractor)
        void LoadData();
        // regions is specific to this panel... should store it here...
        // and also copy clip info here~... no need to put it in loop
    protected:
        void RenderContent() override;

    private:
        // copied info
        FClipInfo ClipInfo;
        std::vector<int> Regions;

        // timeline from https://github.com/ocornut/imgui/issues/76#issuecomment-287739415 as template and modified a lot
        bool BeginTimeline(int opt_exact_num_rows=0);
        bool TimelineEvent(int N);
        void EndTimeline();
        void TimelineControlButtons();
        
        int TimelineRows;
        float TimelineDisplayStart = 0.f; // in pixel? .. nothing to do with real time stamp
        float TimelineDisplayEnd = 100.f; // in pixel? just an arbitrary value...
        int temp_TimelineIndex = -1;
        
        float TimelineZoom = 1.0f;  // 1 ~ 50
        float TimelinePan = 0.0f; // 0.0 ~ 1.0f

        void CheckZoomInput();

        char* PlayIcon = ICON_FA_PLAY;
        bool IsPlaying = false;
        float PlayProgress = 0.f; //0 ~ 100

        // video actions:
        void PlayClip();
        int NumFramesToExtract;

        // extraction
        int SelectedExtractionOption;
        std::string ExtractionOptions;
        void PerformExtraction();
    };
}
