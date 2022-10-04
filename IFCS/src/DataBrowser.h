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
        std::string selected_video;
        int selected_frame = -1;
    protected:
        void RenderContent() override;
    private:
        bool Test;
        char FilterText[128];
        ImVec2 GetBtnSize();
        void RecursiveClipTreeNodeGenerator(const std::filesystem::path& path, unsigned int depth);
        YAML::Node FramesNode;
        // int clip_selection_mask = (1 << 2);
        // unsigned int clip_node_idx = 0u;
        // int clicked_clip_node = -1;
        const std::array<std::string , 6> AcceptedClipsFormat = {".mp4", ".mov", ".wmv", ".avi", ".flv", ".mkv"};
    };
    
}
