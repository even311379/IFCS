#include "MainMenu.h"

#include "Annotation.h"
#include "Application.h"
#include "CategoryManagement.h"
#include "DataBrowser.h"
#include "FrameExtractor.h"
#include "imgui.h"
#include "Log.h"
#include "ModelGenerator.h"
#include "ModelViewer.h"
#include "Prediction.h"
#include "Setting.h"
#include "TrainingSetGenerator.h"
#include "TrainingSetViewer.h"
#include "Utils.h"
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
            ImGui::MenuItem(LOCTEXT("ToolbarItem.NewProject"));
            ImGui::MenuItem(LOCTEXT("ToolbarItem.LoadProject"));
            ImGui::MenuItem(LOCTEXT("ToolbarItem.ImportData"));
            ImGui::MenuItem(LOCTEXT("ToolbarItem.ExportData"));
            ImGui::Separator();
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.Setting")))
            {
                // ImGui::OpenPopup("Setting");
                Setting::Get().IsModalOpen = true;
                spdlog::info("Setting popup is clicked...");
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
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(LOCTEXT("ToolbarMenu.Train")))
            {
                if (ImGui::MenuItem("TrainingSetGenerator", "", TrainingSetGenerator::Get().GetVisibility()))
                {
                    TrainingSetGenerator::Get().ToggleVisibility();
                }
                if (ImGui::MenuItem("TrainingSetViewer", "", TrainingSetViewer::Get().GetVisibility()))
                {
                    TrainingSetViewer::Get().ToggleVisibility();
                }
                if (ImGui::MenuItem("ModelGenerator", "", ModelGenerator::Get().GetVisibility()))
                {
                    ModelGenerator::Get().ToggleVisibility();
                }
                if (ImGui::MenuItem("ModelViewer", "", ModelViewer::Get().GetVisibility()))
                {
                    ModelViewer::Get().ToggleVisibility();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(LOCTEXT("ToolbarMenu.Predict")))
            {
                if (ImGui::MenuItem("Prediction", "", Prediction::Get().GetVisibility()))
                {
                    Prediction::Get().ToggleVisibility();
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
            ImGui::MenuItem(LOCTEXT("ToolbarItem.About"));
            ImGui::MenuItem(LOCTEXT("ToolbarItem.Tutorial"));
            ImGui::MenuItem(LOCTEXT("ToolbarItem.Contact"));
            ImGui::MenuItem(LOCTEXT("ToolbarItem.Licence"));
            ImGui::EndMenu();
        }
        ImGui::Dummy({200, 1});
        ImGui::Text(LOCTEXT("ToolbarTxt.Wks"));
        Utils::AddSimpleTooltip(LOCTEXT("ToolbarMenu.ChooseWks"));
        ImGui::Text("|");
        if (ImGui::MenuItem(LOCTEXT("ToolbarMenu.WksData")))
        {
            Setting::Get().SetWorkspace(EWorkspace::Data);
        }
        ImGui::Text("|");
        if (ImGui::MenuItem(LOCTEXT("ToolbarMenu.WksTrain")))
        {
            Setting::Get().SetWorkspace(EWorkspace::Train);
        }
        ImGui::Text("|");
        if (ImGui::MenuItem(LOCTEXT("ToolbarMenu.WksPredict")))
        {
            Setting::Get().SetWorkspace(EWorkspace::Predict);
        }
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

