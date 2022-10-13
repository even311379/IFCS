#pragma once
#include <array>
#include <filesystem>
#include "Panel.h"


namespace IFCS
{

    class DataBrowser : public Panel
    {
    public:
        MAKE_SINGLETON(DataBrowser)
        // void SynProjectData();
        FClipInfo SelectedClipInfo;
        int SelectedFrame = -1;
        void LoadOtherFrame(bool IsNext); // ture for next, false for previous

        std::string FrameTitle;
        unsigned int LoadedFramePtr;
        void LoadFrame(int FrameNumber);
        bool AnyFrameLoaded = false;
        // TODO: make access of opencv clip info centric to here...
        std::vector<std::string> GetAllClips() const;
        std::vector<std::string> GetAllFolders() const;
    protected:
        void RenderContent() override;
    private:
        bool Test;
        char FilterText[128];
        ImVec2 GetBtnSize();
        void RecursiveClipTreeNodeGenerator(const std::filesystem::path& Path, unsigned int Depth);
        // TODO: remove this var.... 
        // YAML::Node FramesNode;
        std::unordered_map<int, size_t> FramesData;
        void UpdateFramesNode();
        const std::array<std::string, 6> AcceptedClipsFormat = {".mp4", ".mov", ".wmv", ".avi", ".flv", ".mkv"};
        void MakeFrameTitle();
    };
}
