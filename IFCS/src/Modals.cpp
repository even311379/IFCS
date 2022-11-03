#include "Modals.h"

void Modals::Render()
{
    if (!Setting::Get().ProjectIsLoaded && !WelcomeModal::Get().IsChoosingFolder)
    {
        ImGui::OpenPopup("Welcome");
    }
    if (Setting::Get().IsModalOpen && !Setting::Get().IsChoosingFolder)
    {
        ImGui::OpenPopup("Setting");
    }
}
