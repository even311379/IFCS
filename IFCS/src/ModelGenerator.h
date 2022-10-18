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
    private:
        char EnvName[64];
        const char ModelOptions[128] = "yolov7\0yolov7-d6\0yolov7-e6\0yolov7-e6e\0yolov7-tiny\0yolov7-w6\0yolov7x\0\0";
        int SelectedModelIndex;
        bool CheckWeightHasDownloaded();
        void Training();

        std::vector<std::string> AvailTrainingSets;
        size_t SelectedSetIdx;
        int Epoch;
        int BatchSize;
        int ImageWidth;
        int ImageHeight;
        std::array<int, 2> MaxImageSize;
    };
}
