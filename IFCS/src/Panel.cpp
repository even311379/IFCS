#include "Panel.h"

#include "imgui_internal.h"
#include "Log.h"
#include "Setting.h"
#include "Utils.h"
#include "ImFileDialog/ImFileDialog.h"
#include "ImguiNotify/font_awesome_5.h"
#include "Spectrum/imgui_spectrum.h"

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
                    // if (ImGui::IsWindowAppearing() || bAlwaysRender) // TODO: just checked once ? not working...
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
            ImGui::DockBuilderSplitNode(Center, ImGuiDir_Right, 0.2f, &Right, nullptr);
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
            ImGui::DockBuilderSplitNode(DockspaceID, ImGuiDir_Left, 0.2f, &Left, &Right);
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
            ImGui::DockBuilderSplitNode(DockspaceID, ImGuiDir_Left, 0.2f, &Left, &Right);
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


    void WelcomeModal::Render()
    {
        // TODO: better looking
        // TODO: start from exising project?
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Welcome", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Welcome to IFCS!");
            ImGui::Text("The first step is to setup PROJECT!");
            ImGui::Text("All your hardwork will be stored at there");
            ImGui::Dummy(ImVec2(10, 30));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(10, 30));
            ImGui::BulletText("Project Location:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(400);
            ImGui::InputText("##hidden", TempProjectLocation, IM_ARRAYSIZE(TempProjectLocation), ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            if (ImGui::Button("Choose Project"))
            {
                IsChoosingFolder = true;
                ifd::FileDialog::Instance().Open("ChooseProjectLocationDialog", "Choose project location", "");
            }
            ImGui::BulletText("Project Name:   ");
            // directly insert blank lines are the best approach to make them align~
            ImGui::SameLine();
            ImGui::SetNextItemWidth(400);
            if (ImGui::InputText("##Project Name", TempProjectName, IM_ARRAYSIZE(TempProjectName)))
            {
                Setting::Get().Project = TempProjectName;
            }
            ImGui::SetCursorPosX(ImGui::GetWindowWidth()* 0.4f);
            ImGui::BeginDisabled(!CheckValidInputs());
            if (ImGui::Button("OK", ImVec2(ImGui::GetWindowWidth() * 0.2f, ImGui::GetFontSize() * 1.5f)))
            {
                Setting::Get().ProjectPath = std::string(TempProjectLocation) + Setting::Get().Project;
                Setting::Get().CreateStartup();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled();
            if (!CheckValidInputs())
            {
                ImGui::SameLine();
                ImGui::TextColored(Spectrum::RED(400, Setting::Get().Theme == ETheme::Light), "Fill in valid content");
            }
            ImGui::EndPopup();
        }
    }

    bool WelcomeModal::CheckValidInputs()
    {
        return std::filesystem::exists(TempProjectLocation) && !Setting::Get().Project.empty();
    }
}
