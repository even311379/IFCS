#pragma once
#include <array>
#include <filesystem>
#include <map>
#include "Panel.h"

namespace IFCS
{
    class DataBrowser : public Panel
    {
    public:
        MAKE_SINGLETON(DataBrowser)
        void Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose = true) override;
        void LoadOtherFrame(bool IsNext); // ture for next, false for previous
        void LoadFrame(int FrameNumber); // if frame num < 0 .. load first extracted frame
        // TODO: make access of opencv clip info centric to here...
        std::vector<std::string> GetAllClips() const;
        std::vector<std::string> GetAllImages() const;
        std::vector<std::string> GetImageFolders() const;

        FClipInfo SelectedClipInfo;
        FImageInfo SelectedImageInfo;

        bool IsSelectAnyClipOrImg() const
        {
            return !SelectedClipInfo.ClipPath.empty() || !SelectedImageInfo.ImagePath.empty();
        }

        void PostChangeProject()
        {
            SelectedClipInfo.ClipPath.clear();
            SelectedImageInfo.ImagePath.clear();
            ShouldViewDetail = false;
        };
        unsigned int LoadedFramePtr;
        std::map<int, cv::Mat> VideoFrames;
        void LoadVideoFrame(int FrameNumber);
        void MatToGL(const cv::Mat& Frame);

        const std::array<std::string, 6> AcceptedClipsFormat = {".mp4", ".mov", ".wmv", ".avi", ".flv", ".mkv"};
        const std::array<std::string, 3> AcceptedImageFormat = {".jpg", ".jpeg", ".png"};
    protected:
        void RenderContent() override;

    private:
        void RecursiveClipTreeNodeGenerator(const std::filesystem::path& Path, unsigned int Depth);
        void RecursiveImageTreeNodeGenerator(const std::filesystem::path& Path, unsigned int Depth);
        void CreateSeletable_TrainingSets();
        void CreateSeletable_Models();
        void CreateSeletable_Detections();
        void RenderDetailWidget();
        bool HasAnyClip = false;
        bool HasAnyImage = false;
        std::unordered_map<std::string, FAnnotationToDisplay> ImgAnnotationsToDisplay;
        bool NeedReviewedOnly;

        bool ShouldViewDetail = false;
        std::string SelectedTrainingSet;
        FTrainingSetDescription TrainingSetDescription;
        std::string SelectedModel;
        FModelDescription ModelDescription;
        std::string SelectedDetection;
        FDetectionDescription DetectionDescription;
    };
}
