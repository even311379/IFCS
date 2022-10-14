#pragma once
#include <set>
#include <opencv2/core/mat.hpp>

#include "Panel.h"

// namespace cv
// {
//     class Mat;    
// }

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
        void UpdatePreviewAugmentations();
        cv::Mat GenerateAugentationImage(cv::Mat InMat);
        void IncludeGenClip(const std::string& InClip);
        void IncludeGenFolder(const std::string& InFolder);
        std::set<std::string> IncludedGenClips;
        std::set<std::string> IncludedGenFolders;
        int NumIncludedFrames = 0;
        int NumIncludedImages = 0; // leave for future update...
        int TotalExportImages = 1;
        int NumIncludedAnnotation = 0;
        void UpdateExportInfo();

        void DrawSplitWidget();
        float SplitControlPos1 = 0.75f;
        float SplitControlPos2 = 0.90f;
        float SplitPercent[3] = {75.f, 15.f, 10.f};
        int SplitImgs[3] = {0, 0, 0};

        int SelectedResizeRule = 0;
        int NewSize[2] = {640, 360};

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

        char NewTrainingSetName[64];
        int DuplicateTimes = 1;

        // for visual settup
        float ExportWidgetGropWidth = 0.f;

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
        unsigned int Var9;


        bool bApplyDefaultCategories = true;
        bool bApplySMOTE = false;
    };
}
