#pragma once
#include "Panel.h"

namespace IFCS 
{
    class Annotation : public Panel
    {
    public:
        MAKE_SINGLETON(Annotation)
    protected:
        void RenderContent() override;
        
    private:
        EAnnotationEditMode CurrentMode = EAnnotationEditMode::Add;
    };
    
}
