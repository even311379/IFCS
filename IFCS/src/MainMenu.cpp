#include "MainMenu.h"

#include "Application.h"
#include "Annotation.h"
#include "CategoryManagement.h"
#include "DataBrowser.h"
#include "Deploy.h"
#include "imgui.h"
#include "Log.h"
#include "Train.h"
#include "Detection.h"
#include "Modals.h"
#include "Setting.h"
#include "Utils.h"
#include "Style.h"
#include "IconFontCppHeaders/IconsFontAwesome5.h"
#include "ImFileDialog/ImFileDialog.h"

IFCS::MainMenu::~MainMenu()
{
}

void IFCS::MainMenu::Render()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::MenuItem("Save", nullptr, false, Annotation::Get().NeedSaveFile))
        {
            Annotation::Get().SaveDataFile();
        }
        if (ImGui::BeginMenu(LOCTEXT("ToolbarMenu.Project")))
        {
            if (ImGui::BeginMenu("Open"))
            {
                if (ImGui::MenuItem(LOCTEXT("ToolbarItem.LoadProject")))
                {
                    Modals::Get().IsModalOpen_LoadProject = true;
                }
                if (Setting::Get().RecentProjects.size() > 1)
                {
                    ImGui::Separator();
                    for (const std::string& P : Setting::Get().RecentProjects)
                    {
                        if (P == Setting::Get().ProjectPath) continue;
                        if (ImGui::MenuItem(P.c_str()))
                        {
                            Setting::Get().ProjectPath = P;
                            Setting::Get().StartFromPreviousProject();
                        }
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.NewProject")))
            {
                Modals::Get().IsModalOpen_NewProject = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.ImportData")))
            {
                Modals::Get().IsModalOpen_ImportData = true;
            }
            if (ImGui::MenuItem(LOCTEXT("ToolbarItem.ExportData")))
            {
                Modals::Get().IsChoosingFolder = true;
                ifd::FileDialog::Instance().Open("ChooseExportAnnotationDialog", "Export annotation data", "");
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
                if (ImGui::MenuItem("CategoryManagement", "", CategoryManagement::Get().GetVisibility()))
                {
                    CategoryManagement::Get().ToggleVisibility();
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
            if (ImGui::BeginMenu("Deploy"))
            {
                if (ImGui::MenuItem("Deploy", "", Deploy::Get().GetVisibility()))
                {
                    Deploy::Get().ToggleVisibility();
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
        ImGui::Text(ICON_FA_ANGLE_RIGHT);
        if (CurrentWks == EWorkspace::Deploy)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, Style::RED(400, CurrentTheme));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Style::RED(400, CurrentTheme));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Style::RED(400, CurrentTheme));
        }
        if (ImGui::Button("Deploy", WksBtnSize))
        {
            if (CurrentWks != EWorkspace::Deploy)
                Setting::Get().SetWorkspace(EWorkspace::Deploy);
        }
        if (CurrentWks == EWorkspace::Deploy)
            ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::EndMainMenuBar();
    }
}

void IFCS::MainMenu::SetApp(Application* InApp)
{
    App = InApp;
}
