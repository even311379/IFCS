#include "MainMenu.h"

#include "Annotation.h"
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
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.Setting"),"", false, true))
            {
                OpenSetting();
            }
            ImGui::Separator();
            ImGui::MenuItem(LOCTEXT("ToolbarItem.Quit"));
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(LOCTEXT("ToolbarMenu.Panels")))
        {
            // TODO: more panels to add later...
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
            Annotation::Get().SetVisibility(true);
            FrameExtractor::Get().SetVisibility(true);
            CategoryManagement::Get().SetVisibility(true);
            TrainingSetGenerator::Get().SetVisibility(false);
            TrainingSetViewer::Get().SetVisibility(false);
            ModelGenerator::Get().SetVisibility(false);
            ModelViewer::Get().SetVisibility(false);
            Prediction::Get().SetVisibility(false);
            DataBrowser::Get().SetVisibility(true);
            Setting::Get().SetWorkspace(EWorkspace::Data);

        }
        ImGui::Text("|");
        if (ImGui::MenuItem(LOCTEXT("ToolbarMenu.WksTrain")))
        {
            Annotation::Get().SetVisibility(false);
            FrameExtractor::Get().SetVisibility(false);
            CategoryManagement::Get().SetVisibility(false);
            TrainingSetGenerator::Get().SetVisibility(true);
            TrainingSetViewer::Get().SetVisibility(true);
            ModelGenerator::Get().SetVisibility(true);
            ModelViewer::Get().SetVisibility(true);
            Prediction::Get().SetVisibility(false);
            DataBrowser::Get().SetVisibility(true);
            Setting::Get().SetWorkspace(EWorkspace::Train);
        }
        ImGui::Text("|");
        if (ImGui::MenuItem(LOCTEXT("ToolbarMenu.WksPredict")))
        {
            Annotation::Get().SetVisibility(false);
            FrameExtractor::Get().SetVisibility(false);
            CategoryManagement::Get().SetVisibility(false);
            TrainingSetGenerator::Get().SetVisibility(false);
            TrainingSetViewer::Get().SetVisibility(false);
            ModelGenerator::Get().SetVisibility(false);
            ModelViewer::Get().SetVisibility(false);
            Prediction::Get().SetVisibility(true);
            DataBrowser::Get().SetVisibility(true);
            Setting::Get().SetWorkspace(EWorkspace::Predict);
        }
        // very unlikely to add custom workspace... like blender...
        // ImGui::Text("|");
        // ImGui::MenuItem(ICON_FA_PLUS);
        // Utils::AddSimpleTooltip(LOCTEXT("ToolbarMenu.AddWks"));

        ImGui::EndMainMenuBar();
    }
}

void IFCS::MainMenu::OpenSetting()
{
}
