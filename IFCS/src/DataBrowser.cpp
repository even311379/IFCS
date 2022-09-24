#include "DataBrowser.h"

void IFCS::DataBrowser::RenderContent()
{
    // ImGui::PushFont()
    if (ImGui::TreeNode("Data Browser"))
    {
        if (ImGui::TreeNode("VideoClips"))
        {
            if (ImGui::TreeNode("Video1"))
            {
                for (int i=0; i < 10; i++)
                {
                    ImGui::Selectable("img");
                }
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Video1"))
            {
                for (int i=0; i < 10; i++)
                {
                    ImGui::Selectable("img");
                }
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("RawImages"))
        {
            for (int i=0; i < 10; i++)
            {
                ImGui::Selectable("img");
            }
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }
}
