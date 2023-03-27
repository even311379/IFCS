#pragma once
#include <array>
#include <filesystem>
#include <future>
#include <map>
#include "Panel.h"

namespace IFCS
{
    class DataBrowser : public Panel
    {
    public:
        MAKE_SINGLETON(DataBrowser)
        void Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose = true) override;
        // void LoadOtherFrame(bool IsNext); // ture for next, false for previous
        // void LoadFrame(int FrameNumber); // if frame num < 0 .. load first extracted frame
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
        }

        // All frame/image loading should be here!!
        int VideoStartFrame;
        int CurrentFrame;
        int VideoEndFrame;
        unsigned int LoadedFramePtr;
        std::map<int, cv::Mat> VideoFrames;
        void DisplayFrame(int NewFrameNum, const std::string& ParticularClip = "");
        void DisplayImage();
        void PrepareVideo(bool& InLoadingStatus);
        bool IsImageDisplayed = false;
        void LoadingVideoBlock(bool& InLoadingStatus, int CurrentFrameNum, const std::string& ParticularClip="");
        std::vector<std::future<void>> LoadingVideoTasks;
        bool ForceCloseLoading = false;
        void MatToGL(const cv::Mat& Frame);
        void MoveFrame(int NewFrame);
        
        

        const std::array<std::string, 6> AcceptedClipsFormat = {".mp4", ".mov", ".wmv", ".avi", ".flv", ".mkv"};
        const std::array<std::string, 3> AcceptedImageFormat = {".jpg", ".jpeg", ".png"};
        
        bool NeedReviewedOnly;

        EAssetType LastSelectedAssetType = EAssetType::Clip;
        std::string SelectedTrainingSet;
        std::string SelectedModel;
        std::string SelectedDetection;
        bool ShouldViewDetail = false;
        
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

        FTrainingSetDescription TrainingSetDescription;
        FModelDescription ModelDescription;
        FDetectionDescription DetectionDescription;
    };
}
