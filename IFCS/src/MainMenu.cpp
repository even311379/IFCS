#include "MainMenu.h"

#include "Annotation.h"
#include "Application.h"
#include "CategoryManagement.h"
#include "DataBrowser.h"
#include "FrameExtractor.h"
#include "imgui.h"
#include "Log.h"
#include "Train.h"
#include "Prediction.h"
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
            ImGui::MenuItem(LOCTEXT("ToolbarItem.NewProject"));
            ImGui::MenuItem(LOCTEXT("ToolbarItem.LoadProject"));
            ImGui::MenuItem(LOCTEXT("ToolbarItem.ImportData"));
            ImGui::MenuItem(LOCTEXT("ToolbarItem.ExportData"));
            ImGui::Separator();
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.Setting")))
            {
                // ImGui::OpenPopup("Setting");
                Setting::Get().IsModalOpen = true;
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
        if (CurrentWks == EWorkspace::Predict)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, Style::RED(400, CurrentTheme));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Style::RED(400, CurrentTheme));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Style::RED(400, CurrentTheme));
        }
        if (ImGui::Button(LOCTEXT("ToolbarMenu.WksPredict"), WksBtnSize))
        {
            if (CurrentWks != EWorkspace::Predict)
                Setting::Get().SetWorkspace(EWorkspace::Predict);
        }
        if (CurrentWks == EWorkspace::Predict)
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
