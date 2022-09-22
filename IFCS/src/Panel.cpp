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

    void Panel::Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags)
    {
        Name = InName;
        ShouldOpen = InShouldOpen;
        Flags = InFlags;
        IsSetup = true;
    }

    void Panel::Render()
    {
        if (IsSetup)
        {
            PreRender();
            ImGui::Begin(Name, &ShouldOpen, Flags);
            {
                RenderContent();
            }
            ImGui::End();
        }
        else
        {
            // TODO: add info to log...
        }
        
    }

    void Panel::PreRender()
    {
    }

    void Panel::RenderContent()
    {
    }

    void Panel::SetVisibility(bool NewVisibility)
    {
        ShouldOpen = NewVisibility;
    }

    void BG::Setup()
    {
        ImGuiWindowFlags BGFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        BGFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        Panel::Setup("BG", true, BGFlags);
    }

    void BG::PreRender()
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    }

    void BG::RenderContent()
    {
        ImGui::PopStyleVar(3);
        ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Project"))
            {
                ImGui::MenuItem("New Project");
                ImGui::MenuItem("New Project");
                ImGui::MenuItem("New Project");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Setting"))
            {
                ImGui::MenuItem("New Project");
                ImGui::MenuItem("New Project");
                ImGui::MenuItem("New Project");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Panels"))
            {
                ImGui::MenuItem("New Project");
                ImGui::MenuItem("New Project");
                ImGui::MenuItem("New Project");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("About"))
            {
                ImGui::MenuItem("New Project");
                ImGui::MenuItem("New Project");
                ImGui::MenuItem("New Project");
                ImGui::EndMenu();
            }
            ImGui::Dummy({200, 1});
            ImGui::Text("Workspaces: ");
            ImGui::MenuItem("Prepare Data");
            ImGui::MenuItem("Annotation");
            ImGui::MenuItem("Train");
            ImGui::MenuItem("Predict");

            ImGui::EndMenuBar();
            
        }
    }

    void TestPanel::RenderContent()
    {
        ImGui::Text("Test");
    }
}
