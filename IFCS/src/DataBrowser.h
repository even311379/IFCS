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
        void LoadOtherFrame(bool IsNext); // ture for next, false for previous
        void LoadFrame(int FrameNumber);
        // TODO: make access of opencv clip info centric to here...
        std::vector<std::string> GetAllClips() const;
        std::vector<std::string> GetAllFolders() const;

        FClipInfo SelectedClipInfo;
        int SelectedFrame = 0;
        std::string FrameTitle;
        unsigned int LoadedFramePtr;
        bool AnyFrameLoaded = false;


    protected:
        void RenderContent() override;

    private:
        void RecursiveClipTreeNodeGenerator(const std::filesystem::path& Path, unsigned int Depth);
        void CreateSeletable_TrainingSets();
        void CreateSeletable_Models();
        void CreateSeletable_Predictions();
        void UpdateData();
        void MakeFrameTitle();
        ImVec2 GetBtnSize();
        float TimePassed;
        bool NeedReviewedOnly;
        bool IsViewingSomeDetail = false;
        std::map<int, size_t> FramesData;
        const std::array<std::string, 6> AcceptedClipsFormat = {".mp4", ".mov", ".wmv", ".avi", ".flv", ".mkv"};
    };
}
