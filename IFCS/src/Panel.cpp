#include "Panel.h"
#include "Log.h"
#include "Utils.h"
#include "ImguiNotify/font_awesome_5.h"

#define LOCTEXT(key) Utils::GetLocText(key).c_str() 

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

    void BGPanel::Setup()
    {
        ImGuiWindowFlags BGFlags = ImGuiWindowFlags_NoDocking;
        BGFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        Panel::Setup("BG", true, BGFlags);
    }

    void BGPanel::PreRender()
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    }

    void BGPanel::RenderContent()
    {
        ImGui::PopStyleVar(3);
        ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    void TestPanel::RenderContent()
    {
        // render icon!!
        char buff[300];
        snprintf(buff, sizeof(buff), "this is icon1: %s and icon2: %s", ICON_FA_HANDS, ICON_FA_BUG);
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), buff);


        ImGui::Text("Test");
        ImGui::Text("Hello");
        ImGui::Text(s.c_str());
        ImGui::Text(cs);
        // ImGui::InputText("MM", MM, IM_ARRAYSIZE(MM));
        // if (ImGui::Button("Add Log"))
        //     MyLog->AddLog(ELogLevel::Info, MM);
        // if (ImGui::Button("Add Warning Log"))
        //     MyLog->AddLog(ELogLevel::Warning, MM);
        // if (ImGui::Button("Add Error Log"))
        //     MyLog->AddLog(ELogLevel::Error, MM);
        ImGui::Text("測試讀取多語言");
        // ImGui::Text(Utils::GetLocText("PanelName.Name"));

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
