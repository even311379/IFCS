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
        std::vector<std::string> IncludedClips;
        std::vector<std::string> IncludedImageFolders;
        std::string CatExportSummary;
        int NumIncludedFrames;
        int NumIncludedImages;
        int NumIncludedAnnotations;
        int TotalExportImages = 0;
        std::map<UUID, int> CategoriesExportCounts;
        int SelectedResizeAspectRatio;
        int ExportedImageSize[2] = {748, 432};
        void GenerateTrainingSet();


        // training
        void RenderTrainingOptions();
        void RenderTrainingScript();
        int SelectedModelIdx = 0;
        FTrainingSetDescription SelectedTrainingSet;
        bool IsTraining;
        void Training();
        void OpenTensorBoard();

        bool bApplyTransferLearning = true;
        int Epochs = 40;
        int BatchSize = 16;
        int ImageSize[2] = {748, 432};
        bool ShouldTrackRunProgress = false;
        void UpdateTrainScript();
        std::string SetPathScript = "";
        std::string TrainScript = "";
        std::string TrainLog = "";
        std::string ModelName;
        int SelectedHypIdx = 0;

        std::string TensorBoardHostCommand = "";
        bool HasHostTensorBoard = false;
        std::future<void> TrainingFuture;
        // static std::string TestAsyncFun(int time);
    };
}
