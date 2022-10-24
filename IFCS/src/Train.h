#pragma once
#include <future>

#include "Panel.h"

namespace IFCS
{
    
    class Train : public Panel
    {
    public:
        MAKE_SINGLETON(Train)
        void Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose = true) override;

    protected:
        void RenderContent() override;
    private:
        int SelectedModelIdx = 0;
        FTrainingSetDescription SelectedTrainingSet;
        bool CheckWeightHasDownloaded();
        void Training();
        void TrackRunProgress();

        bool bApplyTransferLearning = true;
        int Epochs = 40;
        int BatchSize = 32;
        int ImageSize[2] = {640, 640};
        bool ShouldTrackRunProgress = false;
        int Tick;
        void UpdateTrainScript();
        std::string SetPathCommand = "";
        std::string TrainScript = "";
        std::string ModelName;
        int SelectedHypIdx = 0;
        
        bool ShouldCheckFuture;
        // std::future<std::string> TestFuture;
        // static std::string TestAsyncFun(int time);
    };
}
