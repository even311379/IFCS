#include "Panel.h"

#include "imgui_internal.h"
#include "Log.h"
#include "Setting.h"
#include "Style.h"
#include "Utils.h"
#include "ImFileDialog/ImFileDialog.h"

#include "IconFontCppHeaders/IconsFontAwesome5.h"
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

    void BGPanel::Setup()
    {
        bAlwaysRender = true;
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
        ImGuiDockNodeFlags Flags = ImGuiDockNodeFlags_PassthruCentralNode;
        ImGuiID DockspaceID = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(DockspaceID, ImVec2(0.0f, 0.0f), Flags);
        if (SetDataWksNow)
        {
            ImGui::DockBuilderRemoveNode(DockspaceID);
            ImGui::DockBuilderAddNode(DockspaceID, Flags | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(DockspaceID, ImGui::GetMainViewport()->Size);
            
            ImGuiID Left;
            ImGuiID Center;
            ImGuiID Right;
            ImGui::DockBuilderSplitNode(DockspaceID, ImGuiDir_Left, 0.15f, &Left, &Center);
            ImGui::DockBuilderSplitNode(Center, ImGuiDir_Right, 0.175f, &Right, nullptr);
            ImGui::DockBuilderDockWindow("Data Browser", Left);
            ImGui::DockBuilderDockWindow("Annotation", Center);
            ImGui::DockBuilderDockWindow("Frame Extractor", Center);
            ImGui::DockBuilderDockWindow("Training Set Generator", Center);
            ImGui::DockBuilderDockWindow("Category Management", Right);
            ImGui::DockBuilderFinish(DockspaceID);
            SetDataWksNow = false;
        }
        if (SetTrainWksNow)
        {
            ImGui::DockBuilderRemoveNode(DockspaceID);
            ImGui::DockBuilderAddNode(DockspaceID, Flags | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(DockspaceID, ImGui::GetMainViewport()->Size);
            
            ImGuiID Left;
            ImGuiID Right;
            ImGui::DockBuilderSplitNode(DockspaceID, ImGuiDir_Left, 0.15f, &Left, &Right);
            ImGui::DockBuilderDockWindow("Data Browser", Left);
            ImGui::DockBuilderDockWindow("Model Generator", Right);
            ImGui::DockBuilderFinish(DockspaceID);
            SetTrainWksNow = false;
        }
        if (SetPredictWksNow)
        {
            ImGui::DockBuilderRemoveNode(DockspaceID);
            ImGui::DockBuilderAddNode(DockspaceID, Flags | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(DockspaceID, ImGui::GetMainViewport()->Size);
            
            ImGuiID Left;
            ImGuiID Right;
            ImGui::DockBuilderSplitNode(DockspaceID, ImGuiDir_Left, 0.15f, &Left, &Right);
            ImGui::DockBuilderDockWindow("Data Browser", Left);
            ImGui::DockBuilderDockWindow("Prediction", Right);
            ImGui::DockBuilderFinish(DockspaceID);
            SetPredictWksNow = false;
        }
    }

    void BGPanel::PostRender()
    {
        ImGui::PopStyleVar(3);
    }

    void UtilPanel::RenderContent()
    {
        const char* Options[] = {"Data", "Train","Predict"};
        ImGui::Combo("Save to Wks", &SelectedWksToSave, Options, IM_ARRAYSIZE(Options));
        if (ImGui::Button("Save Wks"))
        {
            if (SelectedWksToSave == 0)
            {
                ImGui::SaveIniSettingsToDisk("Config/DataWks.ini");
            }
            else if (SelectedWksToSave == 1)
            {
                ImGui::SaveIniSettingsToDisk("Config/TrainWks.ini");
            }
            else if (SelectedWksToSave == 2)
            {
                ImGui::SaveIniSettingsToDisk("Config/PredictWks.ini");
            }
        }
        ImGui::Separator();
    }

    void TestPanel::RenderContent()
    {
        // render icon!!
        char buff[300];
        snprintf(buff, sizeof(buff), "this is icon1: %s and icon2: %s", ICON_FA_HANDS, ICON_FA_BUG);
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), buff);


        ImGui::Text("Test");
        ImGui::Text("Hello");
        ImGui::InputText("MM", MM, IM_ARRAYSIZE(MM));
        if (ImGui::Button("Add Log"))
            LogPanel::Get().AddLog(ELogLevel::Info, MM);
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
            Setting::Get().PreferredLanguage = ESupportedLanguage::TraditionalChinese;
        }
        if (ImGui::Button("Set language Eng"))
        {
            Setting::Get().PreferredLanguage = ESupportedLanguage::English;
        }
    }


}
