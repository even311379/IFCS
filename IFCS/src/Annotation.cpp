#include "Annotation.h"

#include "imgui_internal.h"
#include "Setting.h"

namespace IFCS
{
    void Annotation::RenderContent()
    {
        ImGui::PushFont(Setting::Get().TitleFont);
        ImGui::Text("Clip name + frame ??");
        ImGui::PopFont();
        /*
        ImGuiWindow* Win = ImGui::GetCurrentWindow();
        Win->DrawList->AddRect(); // draw bg
        Win->DrawList->AddImage(); // image
        for (XXXX)
        {
            Win->DrawList->AddRect(); // annotation box
            Win->DrawList->AddText(); // display name
            if (CurrentMode == EAnnotationEditMode::Edit )
            {
                Win->DrawList->AddCircle(); // pan controller
                Win->DrawList->AddCircle(); // resize top left
                Win->DrawList->AddCircle(); // resize bottom left
                Win->DrawList->AddCircle(); // resize bottom right
                Win->DrawList->AddCircle(); // resize top right
            } else if (CurrentMode == EAnnotationEditMode::Reassign)
            {
                Win->DrawList->AddText(); // Assign icon!!
            }
        }
        */
        

        
    }
}
