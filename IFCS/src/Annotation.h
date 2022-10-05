#pragma once
#include "Panel.h"

namespace IFCS 
{
    class Annotation : public Panel
    {
    public:
        MAKE_SINGLETON(Annotation)
        unsigned int ImageID;
    protected:
        void RenderContent() override;
        
    private:
        void Save();
        void Load();
        std::unordered_map<UUID, FAnnotation> Data;
        
        EAnnotationEditMode CurrentMode = EAnnotationEditMode::Add;
        // void Add();
        // void Edit();
        // void Reassign();
        ImVec2 PanAmount;
        int ZoomLevel = 0;
        float GetZoom();
        float GetZoom(int InZoom);
        bool IsAdding = false;
        ImVec2 AddPointStart;

        const ImVec2 WorkArea= {1280, 720};
        ImVec2 WorkStartPos;

        void GetAbsRectMinMax(ImVec2 p0, ImVec2 p1, ImVec2& OutMin, ImVec2& OutMax);
        std::array<float, 4> MouseRectToXYWH(ImVec2 RectMin, ImVec2 RectMax);
        std::array<float, 4> TransformXYWH(const std::array<float, 4>& InXYWH);
        
        // some style controls...
        float ButtonsOffset = 0;
        
    };
    
}
