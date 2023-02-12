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
        void Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose = true) override;
        int CurrentFrame;
        void DisplayFrame(int NewFrameNum);
        void DisplayImage();
        void PrepareVideo();
        bool IsImage = false;
        std::map<int, FAnnotationToDisplay> GetAnnotationToDisplay();
        void MoveFrame(int NewFrame);
        void ClearData()
        {
            Data_Img = FAnnotationSave();
            Data.clear();
        };

        void SetApp(class Application* InApp)
        {
            App = InApp;
        }
        bool NeedSaveFile = false;
        void SaveDataFile();
        void LoadDataFile();
        void SaveData();
        void GrabData();

        std::map<std::string, std::map<int, FAnnotationSave>> Data;
        std::map<int, FAnnotationSave> Data_Frame;
        FAnnotationSave Data_Img;
        
    protected:
        void RenderContent() override;
    private:
        void LoadingVideoBlock();
        void RenderSelectCombo();
        void RenderAnnotationWork();
        void RenderVideoControls();
        void RenderAnnotationToolbar();

        class Application* App;
        
        // annotation controls
        EAnnotationEditMode EditMode = EAnnotationEditMode::Add;
        ImVec2 PanAmount;
        int ZoomLevel = 0;
        float GetZoom(int InZoomLevel);
        float GetZoom();
        std::array<float, 4> MouseRectToXYWH(ImVec2 RectMin, ImVec2 RectMax);
        std::array<float, 4> TransformXYWH(const std::array<float, 4> InXYWH);
        void ResizeImpl(EBoxCorner WhichCorner, const ImVec2& InPos, ImU32 InColor, bool IsDragging, FAnnotation& Ann);


    private:
        // video play controls
        bool IsLoadingVideo;
        std::array<std::future<void>, 4> Tasks;
        
    };
}
