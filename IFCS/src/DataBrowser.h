#pragma once
#include <array>
#include <filesystem>
#include <yaml-cpp/node/node.h>

#include "Panel.h"

namespace IFCS
{
    class DataBrowser : public Panel
    {
    public:
        MAKE_SINGLETON(DataBrowser)
        // void SynProjectData();
        std::string SelectedVideo;
        std::string GetClipName();
        int SelectedFrame = -1;
    protected:
        void RenderContent() override;
    private:
        bool Test;
        char FilterText[128];
        ImVec2 GetBtnSize();
        void RecursiveClipTreeNodeGenerator(const std::filesystem::path& Path, unsigned int Depth);
        YAML::Node FramesNode;
        const std::array<std::string , 6> AcceptedClipsFormat = {".mp4", ".mov", ".wmv", ".avi", ".flv", ".mkv"};
        void LoadAnnotationImage();
    };
    
}
