#pragma once
#include "Panel.h"

namespace IFCS
{
    class TrainingSetGenerator : public Panel
    {
    protected:
        void RenderContent() override;
    public:
        MAKE_SINGLETON(TrainingSetGenerator)
    };
    
}
