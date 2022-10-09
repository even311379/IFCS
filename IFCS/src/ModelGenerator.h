#pragma once
#include "Panel.h"

namespace IFCS
{
    class ModelGenerator : public Panel
    {
    public:
        MAKE_SINGLETON(ModelGenerator)
    protected:
        void RenderContent() override;
    };
}
