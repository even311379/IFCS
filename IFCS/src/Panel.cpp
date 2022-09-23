#include "Panel.h"
#include "Log.h"
#include "Utils.h"
#include "ImguiNotify/font_awesome_5.h"

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
        // render icon!!
        char buff[300];
        snprintf(buff, sizeof(buff), "this is icon1: %s and icon2: %s", ICON_FA_HANDS, ICON_FA_BUG);
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), buff);

        
        ImGui::Text("Test");
        ImGui::Text("Hello");
        // ImGui::InputText("MM", MM, IM_ARRAYSIZE(MM));
        // if (ImGui::Button("Add Log"))
        //     MyLog->AddLog(ELogLevel::Info, MM);
        // if (ImGui::Button("Add Warning Log"))
        //     MyLog->AddLog(ELogLevel::Warning, MM);
        // if (ImGui::Button("Add Error Log"))
        //     MyLog->AddLog(ELogLevel::Error, MM);
        ImGui::Text("測試讀取多語言");
        ImGui::Text(GetLocText("PanelName.Name").c_str());

        if (ImGui::Button("Load init run time??"))
        {
            ImGui::LoadIniSettingsFromDisk("BBB.ini");
        }
        if (ImGui::Button("Set language Chinese"))
        {
            Setting::Get().CurrentLanguage = ESupportedLanguage::TraditionalChinese;
        }
        if (ImGui::Button("Set language Eng"))
        {
            Setting::Get().CurrentLanguage = ESupportedLanguage::English;
        }
    }
}
