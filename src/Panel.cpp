#include "Panel.h"

namespace IFCS
{
    Panel::Panel()
        : Name(nullptr), ShouldOpen(false), Flags(0), IsSetup(false)
    {
    }

    Panel::~Panel()
    {
    }

    void Panel::Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose)
    {
        Name = InName;
        ShouldOpen = InShouldOpen;
        Flags = InFlags;
        IsSetup = true;
        CanClose = InCanClose;
    }

    void Panel::Render()
    {
        if (IsSetup)
        {
            if (ShouldOpen)
            {
                PreRender();
                ImGui::Begin(Name, CanClose ? &ShouldOpen : nullptr, Flags);
                {
                    RenderContent();
                }
                ImGui::End();
                PostRender();
            }
        }
    }

    void Panel::PreRender()
    {
    }

    void Panel::RenderContent()
    {
    }

    void Panel::PostRender()
    {
    }

    void Panel::SetVisibility(bool NewVisibility)
    {
        ShouldOpen = NewVisibility;
    }

}
