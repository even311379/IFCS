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
        // training set
        void RenderDataSelectWidget();
        void RenderCategoryWidget();
        void RenderSplitWidget();
        void RenderResizeWidget();
        void RenderSummary();
        void RenderTrainingSetExportWidget();
        void UpdateGenerartorInfo();
        bool IncludeReadyOnly = false;
        std::vector<std::string> IncludedClips;
        std::vector<std::string> IncludedImageFolders;
        std::string CatExportSummary;
        int NumIncludedFrames;
        int NumIncludedImages;
        int NumIncludedAnnotations;
        int TotalExportImages = 0;
        std::unordered_map<UUID, int> CategoriesExportCounts;
        int SelectedResizeAspectRatio;
        int ExportedImageSize[2] = {768, 432};
        void GenerateTrainingSet();
        void GenerateImgTxt(cv::VideoCapture& Cap, int FrameNum,const std::vector<FAnnotation>& InAnnotations,
            const char* GenName, bool IsOriginal, const char* SplitName);
        std::future<void> GenerateSet_Future;


        // training
        void RenderTrainingOptions();
        void RenderTrainingScript();
        int SelectedModelIdx = 0;
        FTrainingSetDescription SelectedTrainingSet;
        bool IsTraining;
        void Training();
        void OpenTensorBoard();
        void OnTrainingFinished();

        bool bApplyTransferLearning = true;
        int Epochs = 40;
        int BatchSize = 16;
        int TrainImgSize = 640;
        int TestImgSize = 640;
        bool ShouldTrackRunProgress = false;
        void UpdateTrainScript();
        std::string SetPathScript = "";
        std::string TrainScript = "";
        std::string TrainLog = "";
        std::string ModelName;
        // int SelectedHypIdx = 0;
        // advanced
        bool ApplyEvolve;
        bool ApplyAdam;
        bool ApplyCacheImage;
        bool ApplyWeightedImageSelection;
        bool ApplyMultiScale;
        int MaxWorkers = 1;
        std::array<bool, 8> Devices;

        std::string TensorBoardHostCommand = "";
        bool HasHostTensorBoard = false;
        std::future<void> TrainingFuture;
        // static std::string TestAsyncFun(int time);
    };
}
