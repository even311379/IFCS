#include "MainMenu.h"

#include "Annotation.h"
#include "Application.h"
#include "CategoryManagement.h"
#include "DataBrowser.h"
#include "FrameExtractor.h"
#include "imgui.h"
#include "Log.h"
#include "Train.h"
#include "Detection.h"
#include "Modals.h"
#include "Setting.h"
#include "TrainingSetGenerator.h"
#include "Utils.h"
#include "Style.h"
#include "ImguiNotify/font_awesome_5.h"


IFCS::MainMenu::~MainMenu()
{
}

void IFCS::MainMenu::Render()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu(LOCTEXT("ToolbarMenu.Project")))
        {
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.NewProject")))
            {
                Modals::Get().IsModalOpen_NewProject = true;
            }
            // TODO: recent projects...
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.LoadProject")))
            {
                Modals::Get().IsModalOpen_LoadProject = true;
            }
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.ImportData")))
            {
                // TODO: weird problem that makes opening this widget so much buggy...
                // Modals::Get().IsModalOpen_ImportData = true;
            }
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.ExportData")))
            {
                // Modals::Get().IsModalOpen_ExportData = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.Setting")))
            {
                Modals::Get().IsModalOpen_Setting = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.Quit")))
            {
                App->RequestToQuit = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(LOCTEXT("ToolbarMenu.Panels")))
        {
            if (ImGui::BeginMenu(LOCTEXT("ToolbarMenu.Data")))
            {
                if (ImGui::MenuItem("Annotation", "", Annotation::Get().GetVisibility()))
                {
                    Annotation::Get().ToggleVisibility();
                }
                if (ImGui::MenuItem("FrameExtractor", "", FrameExtractor::Get().GetVisibility()))
                {
                    FrameExtractor::Get().ToggleVisibility();
                }
                if (ImGui::MenuItem("CategoryManagement", "", CategoryManagement::Get().GetVisibility()))
                {
                    CategoryManagement::Get().ToggleVisibility();
                }
                if (ImGui::MenuItem("TrainingSetGenerator", "", TrainingSetGenerator::Get().GetVisibility()))
                {
                    TrainingSetGenerator::Get().ToggleVisibility();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(LOCTEXT("ToolbarMenu.Train")))
            {
                if (ImGui::MenuItem("ModelGenerator", "", Train::Get().GetVisibility()))
                {
                    Train::Get().ToggleVisibility();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(LOCTEXT("ToolbarMenu.Detect")))
            {
                if (ImGui::MenuItem("Detection", "", Detection::Get().GetVisibility()))
                {
                    Detection::Get().ToggleVisibility();
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.DataBrowser"), "", DataBrowser::Get().GetVisibility()))
            {
                DataBrowser::Get().ToggleVisibility();
            }
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.Log"), "", LogPanel::Get().GetVisibility(), true))
            {
                LogPanel::Get().ToggleVisibility();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(LOCTEXT("ToolbarMenu.Help")))
        {
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.About")))
            {
                Modals::Get().IsModalOpen_About = true;
            }
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.Tutorial")))
            {
                Modals::Get().IsModalOpen_Tutorial = true;
            }
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.Contact")))
            {
                Modals::Get().IsModalOpen_Contact = true;
            }
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.Licence")))
            {
                Modals::Get().IsModalOpen_License = true;
            }
            ImGui::EndMenu();
        }
        ImGui::Dummy({200, 1});
        ImGui::Text(LOCTEXT("ToolbarTxt.Wks"));
        Utils::AddSimpleTooltip(LOCTEXT("ToolbarMenu.ChooseWks"));
        static const ImVec2 WksBtnSize = {240, 0};
        static EWorkspace CurrentWks;
        static ETheme CurrentTheme;
        CurrentWks = Setting::Get().ActiveWorkspace;
        CurrentTheme = Setting::Get().Theme;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.f);
        if (CurrentWks == EWorkspace::Data)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, Style::RED(400, CurrentTheme));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Style::RED(400, CurrentTheme));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Style::RED(400, CurrentTheme));
        }
        if (ImGui::Button(LOCTEXT("ToolbarMenu.WksData"), WksBtnSize))
        {
            if (CurrentWks != EWorkspace::Data)
                Setting::Get().SetWorkspace(EWorkspace::Data);
        }
        if (CurrentWks == EWorkspace::Data)
            ImGui::PopStyleColor(3);
        ImGui::Text(ICON_FA_ANGLE_RIGHT);
        if (CurrentWks == EWorkspace::Train)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, Style::RED(400, CurrentTheme));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Style::RED(400, CurrentTheme));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Style::RED(400, CurrentTheme));
        }
        if (ImGui::Button(LOCTEXT("ToolbarMenu.WksTrain"), WksBtnSize))
        {
            if (CurrentWks != EWorkspace::Train)
                Setting::Get().SetWorkspace(EWorkspace::Train);
        }
        if (CurrentWks == EWorkspace::Train)
            ImGui::PopStyleColor(3);
        ImGui::Text(ICON_FA_ANGLE_RIGHT);
        if (CurrentWks == EWorkspace::Detect)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, Style::RED(400, CurrentTheme));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Style::RED(400, CurrentTheme));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Style::RED(400, CurrentTheme));
        }
        if (ImGui::Button(LOCTEXT("ToolbarMenu.WksDetect"), WksBtnSize))
        {
            if (CurrentWks != EWorkspace::Detect)
                Setting::Get().SetWorkspace(EWorkspace::Detect);
        }
        if (CurrentWks == EWorkspace::Detect)
            ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        // very unlikely to add custom workspace... like blender...
        // ImGui::Text("|");
        // ImGui::MenuItem(ICON_FA_PLUS);
        // Utils::AddSimpleTooltip(LOCTEXT("ToolbarMenu.AddWks"));

        ImGui::EndMainMenuBar();
    }
}

void IFCS::MainMenu::SetApp(Application* InApp)
{
    App = InApp;
}
