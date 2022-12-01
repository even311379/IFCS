#pragma once
#include "Panel.h"
#include <future>
#include "Common.h"

namespace IFCS
{

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
        void ClearData()
        {
            Data_Img.clear();
            Data.clear();
        };
        
    protected:
        void RenderContent() override;
    private:
        void LoadingVideoBlock();
        void RenderSelectCombo();
        void RenderAnnotationWork();
        void RenderVideoControls();
        void RenderAnnotationToolbar();
        void SaveData();
        void LoadData();
        
        // data for for whole clip
        std::map<int, std::unordered_map<UUID, FAnnotation>> Data;
        std::map<int, FCheck> CheckData;

        // data for single image
        std::unordered_map<UUID, FAnnotation> Data_Img;
        std::string UpdateTime_Img;
        bool IsReady_Img;
        

        
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
