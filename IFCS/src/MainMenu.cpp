#include "MainMenu.h"
#include "imgui.h"
#include "Log.h"
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
                ImGui::MenuItem(LOCTEXT("ToolbarItem.DataBrowser"));
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(LOCTEXT("ToolbarMenu.Train")))
            {
                ImGui::MenuItem(LOCTEXT("ToolbarItem.DataBrowser"));
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(LOCTEXT("ToolbarMenu.Predict")))
            {
                ImGui::MenuItem(LOCTEXT("ToolbarItem.DataBrowser"));
                ImGui::EndMenu();
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
        ImGui::MenuItem(LOCTEXT("ToolbarMenu.WksData"));
        ImGui::Text("|");
        ImGui::MenuItem(LOCTEXT("ToolbarMenu.WksTrain"));
        ImGui::Text("|");
        ImGui::MenuItem(LOCTEXT("ToolbarMenu.WksPredict"));
        ImGui::Text("|");
        ImGui::MenuItem(ICON_FA_PLUS);
        Utils::AddSimpleTooltip(LOCTEXT("ToolbarMenu.AddWks"));

        ImGui::EndMainMenuBar();
    }
}

void IFCS::MainMenu::OpenSetting()
{
}
