#pragma once
#include "Panel.h"
#include <future>
#include "Common.h"

namespace IFCS
{
    struct FAnnotationToDisplay
    {
        FAnnotationToDisplay()=default;
        FAnnotationToDisplay(const int& InCount, const bool& InReady)
            :Count(InCount), IsReady(InReady) {}
        int Count;
        bool IsReady;
    };
    
    class Annotation : public Panel
    {
    public:
        MAKE_SINGLETON(Annotation)
        int CurrentFrame;
        void DisplayFrame(int NewFrameNum);
        void DisplayImage();
        void PrepareVideo();
        bool IsImage = false;
        std::map<int, FAnnotationToDisplay> GetAnnotationToDisplay();
        void MoveFrame(int NewFrame);
    protected:
        void RenderContent() override;
    private:
        void RenderSelectCombo();
        void RenderAnnotationWork();
        void RenderVideoControls();
        void RenderAnnotationToolbar();
        void SaveData();
        void LoadData();
        
        // data for for whole clip, also supports for image
        std::map<int, std::unordered_map<UUID, FAnnotation>> Data;
        std::map<int, FCheck> CheckData;

        
        // annotation controls
        EAnnotationEditMode EditMode = EAnnotationEditMode::Add;
        ImVec2 PanAmount;
        int ZoomLevel = 0;
        float GetZoom(int InZoomLevel);
        float GetZoom();
        std::array<float, 4> MouseRectToXYWH(ImVec2 RectMin, ImVec2 RectMax);
        std::array<float, 4> TransformXYWH(const std::array<float, 4> InXYWH);
        void ResizeImpl(EBoxCorner WhichCorner, const ImVec2& InPos, ImU32 InColor, bool IsDragging, FAnnotation& Ann);

        // video play controls
        bool IsLoadingVideo;
        std::array<std::future<void>, 4> Tasks;
        
        
        
    };
    
}
