#pragma once
#include "Panel.h"


struct ImGuiWindow;
namespace IFCS 
{
    
    class Annotation : public Panel
    {
    public:
        MAKE_SINGLETON(Annotation)
        void Save();
        void Load();
    protected:
        void RenderContent() override;
        void PostRender() override;
        
    private:
        // TODO: make save when quit app
        bool IsCurrentFrameModified = false; // use this to notify user?
        // TODO: I can implement undo system with array of data...
        std::unordered_map<UUID, FAnnotation> Data;
        
        EAnnotationEditMode CurrentMode = EAnnotationEditMode::Add;
        void ResizeImpl(ImGuiWindow* WinPtr,const EBoxCorner& WhichCorner,const ImVec2& InPos, const ImU32& InColor,
            const bool& IsDragging, const UUID& InID);
        void Reassign(UUID ID);
        void Trash(UUID ID);
        UUID ToTrashID = 0;
        ImVec2 PanAmount;
        int ZoomLevel = 0;
        float GetZoom();
        static float GetZoom(int InZoom);
        bool IsAdding = false;
        ImVec2 AddPointStart;
        ImVec2 WorkStartPos;

        void GetAbsRectMinMax(ImVec2 p0, ImVec2 p1, ImVec2& OutMin, ImVec2& OutMax);
        std::array<float, 4> MouseRectToXYWH(ImVec2 RectMin, ImVec2 RectMax);
        std::array<float, 4> TransformXYWH(const std::array<float, 4>& InXYWH);
        
    };
    
}
