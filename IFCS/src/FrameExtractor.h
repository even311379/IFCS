#pragma once
#include <vector>

#include "Panel.h"
#include "ImguiNotify/font_awesome_5.h"

namespace IFCS
{
    class FrameExtractor : public IFCS::Panel
    {
    public:
        MAKE_SINGLETON(FrameExtractor)
        void LoadFrame1AsThumbnail(const std::string& file_path);
    protected:
        void RenderContent() override;


    private:
        bool HasLoadAnyThumbnail = false;
        unsigned int Thumbnail;
        ImVec2 VideoSize;

        // timeline from https://github.com/ocornut/imgui/issues/76#issuecomment-287739415
        bool BeginTimeline(const char* str_id, int num_visible_rows=5,int opt_exact_num_rows=0);
        bool TimelineEvent(const char* str_id, float values[2], bool keep_range_constant=false);
        void EndTimeline(int num_vertical_grid_lines=5.f,float current_time=0.f,ImU32 timeline_running_color=IM_COL32(0,128,0,200));
        void TimelineControlButtons();

        int TimelineMax;
        int TimelineMin;
        
        float TimelineZoom = 1.0f;
        float TimelinePan = 0.0f;

        char* PlayIcon = ICON_FA_PLAY;
        bool IsPlaying = false;
        float PlayProgress = 0.f;
        std::vector<float> regions;

        // video info:
        int Resolution[2];
        float FrameRate;
        int FrameCount;
    };
}
