#pragma once
#include <set>
#include <opencv2/videoio.hpp>
#include <opencv2/core/mat.hpp>
#include "Panel.h"


namespace IFCS
{

    
    class TrainingSetGenerator : public Panel
    {
    protected:
        void RenderContent() override;
    public:
        MAKE_SINGLETON(TrainingSetGenerator)
    private:
        void GenerateTrainingSet();
        // void GenerateImgTxt(const std::string& Path, int FrameNum, const char* GenName, bool IsOriginal = true,
        //                     const char* SplitName = "Train", bool IsFromClip = true);
        void GenerateImgTxt(cv::VideoCapture& Cap, int FrameNum, const std::vector<FAnnotation>& InAnnotations, const char* GenName, bool IsOriginal = true,
                            const char* SplitName = "Train");
        void UpdatePreviewAugmentations();
        cv::Mat GenerateAugentationImage(cv::Mat InMat, FAnnotationShiftData& OutShift);
        void IncludeGenClip(const std::string& InClip);
        void IncludeGenFolder(const std::string& InFolder);
        std::set<std::string> IncludedGenClips;
        std::set<std::string> IncludedImageFolders;
        int NumIncludedFrames = 0;
        int NumIncludedImages = 0; // leave for future update...
        int TotalExportImages = 1;
        int NumIncludedAnnotation = 0;
        void UpdateExportInfo();

        void RenderDataSelectWidget();
        // TODO: Need to study train / valid / test again. make sure I understand 100% where they are used in deep learning?
        void RenderSplitWidget();
        void RenderResizeWidget();
        void RenderCategoryWidget();
        void RenderAugmentationWidget();
        void RenderSummary();
        void RenderExportWidget();
        
        float SplitControlPos1 = 0.80f;
        float SplitControlPos2 = 0.90f;
        float SplitPercent[3] = {80.f, 10.f, 10.f};
        int SplitImgs[3] = {0, 0, 0};

        int SelectedResizeRule = 0;
        int NewSize[2] = {720, 405};

        bool bApplyImageAugmentation;
        bool bApplyBlur;
        int MaxBlurAmount = 1;
        bool bApplyNoise;
        int NoiseMeanMin = 1;
        int NoiseMeanMax = 3;
        int NoiseStdMin = 3;
        int NoiseStdMax = 5;
        bool bApplyHue;
        int HueAmount = 1;
        bool bApplyRotation;
        int MaxRotationAmount;
        bool bApplyFlipX;
        int FlipXPercent = 10;
        bool bApplyFlipY;
        int FlipYPercent = 10;
        std::string MakeAugmentationDescription();

        char NewTrainingSetName[64];
        int AugmentationDuplicates = 1;


        // augmentation preview
        unsigned int Origin;
        unsigned int Var0;
        unsigned int Var1;
        unsigned int Var2;
        unsigned int Var3;
        unsigned int Var4;
        unsigned int Var5;
        unsigned int Var6;
        unsigned int Var7;
        unsigned int Var8;


        bool bApplyDefaultCategories = true;
        bool bApplySMOTE = false;
        std::unordered_map<UUID, int> CategoryExportData;
        std::vector<std::string> ExportedCategories;
    };
}
