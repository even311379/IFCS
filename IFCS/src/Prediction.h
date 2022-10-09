#pragma once
#include "Panel.h"
#include "Utils.h"

namespace IFCS
{
    class Prediction : public Panel
    {
    public:
        MAKE_SINGLETON(Prediction)
    protected:
        void RenderContent() override;
    };
    
}
