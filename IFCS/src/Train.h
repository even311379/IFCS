#pragma once
#include "Panel.h"

namespace IFCS
{
    
    class Train : public Panel
    {
    public:
        MAKE_SINGLETON(Train)

    protected:
        void RenderContent() override;
    private:
        int SelectedModelIndex;
        bool CheckWeightHasDownloaded();
        void Training();

        
        int Epoch;
        int BatchSize;
        int ImageSize[2];
    };
}
