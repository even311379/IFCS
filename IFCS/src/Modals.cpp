#include "Modals.h"
#include "ImFileDialog/ImFileDialog.h"
#include "Setting.h"
#include "Style.h"

namespace IFCS
{
    static ImVec2 Center;

    void Modals::Render()
    {
        if (!IsChoosingFolder)
        {
            if (IsModalOpen_Welcome)
                ImGui::OpenPopup("Welcome");
            if (IsModalOpen_NewProject)
                ImGui::OpenPopup("New Project");
            if (IsModalOpen_LoadProject)
                ImGui::OpenPopup("Load Project");
            if (IsModalOpen_ImportData)
                ImGui::OpenPopup("Import Data");
            if (IsModalOpen_ExportData)
                ImGui::OpenPopup("Export Data");
            if (IsModalOpen_Setting)
                ImGui::OpenPopup("Setting");
            if (IsModalOpen_About)
                ImGui::OpenPopup("About");
            if (IsModalOpen_Tutorial)
                ImGui::OpenPopup("Tutorial");
            if (IsModalOpen_Contact)
                ImGui::OpenPopup("Contact");
            if (IsModalOpen_License)
                ImGui::OpenPopup("License");
        }
        Center = ImGui::GetMainViewport()->GetCenter();
        RenderWelcome();
        RenderNewProject();
        RenderLoadProject();
        RenderSetting();
        RenderAbout();
        RenderTutorial();
        RenderContact();
        RenderLicense();
        HandleFileDialogClose();
    }

    void Modals::Sync()
    {
        ThemeToUse = static_cast<int>(Setting::Get().Theme);
        strcpy(TempPythonPath, Setting::Get().PythonPath.c_str());
        strcpy(TempYoloV7Path, Setting::Get().YoloV7Path.c_str());
    }

    void Modals::RenderWelcome()
    {
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Welcome", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text(
                "Welcome to IFCS!\nIntegrated Fish Counting System\nThe first step is to setup PROJECT!\nAll your hardwork will be stored there");
            ImGui::Dummy(ImVec2(0, 30));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 30));
            static bool bFromExistingProject = false;
            ImGui::Checkbox("From existing project?", &bFromExistingProject);
            if (bFromExistingProject)
            {
                ImGui::BulletText("Project Location:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(400);
                ImGui::InputText("##hidden123", TempExistingProjectLocation, IM_ARRAYSIZE(TempExistingProjectLocation),
                                 ImGuiInputTextFlags_ReadOnly);
                ImGui::SameLine();
                if (ImGui::Button("Choose Existing Project"))
                {
                    IsChoosingFolder = true;
                    ifd::FileDialog::Instance().Open("ChooseExistingProjectLocationDialog",
                                                     "Choose existing project location", "");
                }
                ImGui::BeginDisabled(!CheckValidExistingProject());
                if (ImGui::Button("OK", ImVec2(ImGui::GetWindowWidth() * 0.2f, ImGui::GetFontSize() * 1.5f)))
                {
                    Setting::Get().ProjectPath = TempExistingProjectLocation;
                    Setting::Get().StartFromPreviousProject();
                    ImGui::CloseCurrentPopup();
                    IsModalOpen_Welcome = false;
                }
                ImGui::EndDisabled();
            }
            else
            {
                ImGui::BulletText("New Project Name:");
                ImGui::SameLine(300);
                ImGui::SetNextItemWidth(400);
                ImGui::InputText("##Project Name", TempProjectName, IM_ARRAYSIZE(TempProjectName));
                ImGui::BulletText("New Project Location:");
                ImGui::SameLine(300);
                ImGui::SetNextItemWidth(400);
                ImGui::InputText("##hidden", TempProjectLocation, IM_ARRAYSIZE(TempProjectLocation),
                                 ImGuiInputTextFlags_ReadOnly);
                ImGui::SameLine();
                if (ImGui::Button("Choose"))
                {
                    IsChoosingFolder = true;
                    ifd::FileDialog::Instance().Open("ChooseNewProjectLocationDialog", "Choose new project location",
                                                     "");
                }
                ImGui::BeginDisabled(!CheckValidProjectName());
                if (ImGui::Button("OK", ImVec2(240, 0)))
                {
                    Setting::Get().Project = TempProjectName;
                    Setting::Get().ProjectPath = std::string(TempProjectLocation) + "/" + Setting::Get().Project;
                    Setting::Get().CreateStartup();
                    ImGui::CloseCurrentPopup();
                    IsModalOpen_Welcome = false;
                    IsModalOpen_Setting = true;
                }
                ImGui::EndDisabled();
                if (!CheckValidProjectName())
                {
                    ImGui::SameLine();
                    ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Fill in valid content");
                }
            }
            ImGui::EndPopup();
        }
    }

    bool Modals::CheckValidProjectName()
    {
        if (strlen(TempProjectName) == 0) return false;
        if (strlen(TempProjectLocation) == 0) return false;
        // check folder name is used
        for (const auto& entry : std::filesystem::directory_iterator(TempProjectLocation))
        {
            if (entry.path().filename().u8string() == std::string(TempProjectName))
                return false;
        }
        const char InvalidChars[] = "\\/:*?\"<>|";
        for (char C : InvalidChars)
        {
            if (std::string(TempProjectName).find(C) != std::string::npos)
                return false;
        }
        return true;
    }

    bool Modals::CheckValidExistingProject()
    {
        return std::filesystem::exists(std::string(TempExistingProjectLocation) + "/IFCSUser.ini");
    }

    void Modals::RenderNewProject()
    {
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::BulletText("New Project Name:");
            ImGui::SameLine(300);
            ImGui::SetNextItemWidth(400);
            ImGui::InputText("##Project Name", TempProjectName, IM_ARRAYSIZE(TempProjectName));
            ImGui::BulletText("New Project Location:");
            ImGui::SameLine(300);
            ImGui::SetNextItemWidth(400);
            ImGui::InputText("##hidden", TempProjectLocation, IM_ARRAYSIZE(TempProjectLocation),
                             ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            if (ImGui::Button("Choose"))
            {
                IsChoosingFolder = true;
                ifd::FileDialog::Instance().Open("ChooseNewProjectLocationDialog", "Choose new project location", "");
            }
            ImGui::BeginDisabled(!CheckValidProjectName());
            if (ImGui::Button("OK", ImVec2(240, 0)))
            {
                Setting::Get().Project = TempProjectName;
                Setting::Get().ProjectPath = std::string(TempProjectLocation) + "/" + Setting::Get().Project;
                Setting::Get().CreateStartup();
                ImGui::CloseCurrentPopup();
                IsModalOpen_NewProject = false;
                IsModalOpen_Setting = true;
            }
            ImGui::EndDisabled();
            if (!CheckValidProjectName())
            {
                ImGui::SameLine();
                ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Fill in valid content");
            }
            ImGui::EndPopup();
        }
    }

    void Modals::RenderLoadProject()
    {
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Load Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::BulletText("Project Location:");
            ImGui::SetNextItemWidth(400);
            ImGui::InputText("##hidden123", TempExistingProjectLocation, IM_ARRAYSIZE(TempExistingProjectLocation),
                             ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            if (ImGui::Button("Choose Existing Project"))
            {
                IsChoosingFolder = true;
                ImGui::SetNextWindowSize(ImVec2(1280, 720));
                ifd::FileDialog::Instance().Open("ChooseExistingProjectLocationDialog",
                                                 "Choose existing project location", "");
            }
            ImGui::BeginDisabled(!CheckValidExistingProject());
            if (ImGui::Button("OK", ImVec2(240, 0)))
            {
                Setting::Get().ProjectPath = TempExistingProjectLocation;
                Setting::Get().StartFromPreviousProject();
                ImGui::CloseCurrentPopup();
                IsModalOpen_LoadProject = false;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(240, 0)))
            {
                ImGui::CloseCurrentPopup();
                IsModalOpen_LoadProject = false;
            }
            ImGui::EndPopup();
        }
    }


    void Modals::RenderImportData()
    {
        // ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Import Data"))
        {
            ImGui::Text("123");
            if (ImGui::Button("OK", ImVec2(240, 0)))
            {
                ImGui::CloseCurrentPopup();
                IsModalOpen_ImportData = false;
            }
            ImGui::EndPopup();
        }
    }

    void Modals::RenderExportData()
    {
        // ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Export Data"))
        {
            ImGui::Text("123");
            if (ImGui::Button("OK", ImVec2(240, 0)))
            {
                ImGui::CloseCurrentPopup();
                IsModalOpen_ExportData = false;
            }
            ImGui::EndPopup();
        }
    }

    void Modals::RenderSetting()
    {
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Setting", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::BulletText("Choose Theme:");
            ImGui::SameLine();
            if (ImGui::RadioButton("Light", &ThemeToUse, 0))
            {
                Setting::Get().Theme = ETheme::Light;
                Style::ApplyTheme(ETheme::Light);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Dark", &ThemeToUse, 1))
            {
                Setting::Get().Theme = ETheme::Dark;
                Style::ApplyTheme(ETheme::Dark);
            }
            // TODO: finish localization thing?
            ImGui::TextDisabled("Localization not implement yet!");
            ImGui::BulletText("Prefered Language:");
            ImGui::SameLine();
            // TODO: this is a static way... not considering future expansion yet...
            const static char* LanguageOptions[] = {"English", "Traditional Chinese"};
            ImGui::SetNextItemWidth(240.f);
            ImGui::Combo("##Language_options", &LanguageToUse, LanguageOptions, IM_ARRAYSIZE(LanguageOptions));
            const static char* AppSizeOptions[] = {
                "FHD (1920 x 1080)", "2K (2560 x 1440)", "4k (3840 x 2160)", "Custom"
            };
            // TODO: finish app sizing?
            ImGui::TextDisabled("app sizing... not impl yet...");
            ImGui::BulletText("App size: ");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(240.f);
            ImGui::Combo("##AppSize", &AppSizeToUse, AppSizeOptions, IM_ARRAYSIZE(AppSizeOptions));
            if (AppSizeToUse == 3)
            {
                ImGui::Separator();
                ImGui::Indent();
                ImGui::BeginGroup();
                {
                    ImGui::BulletText("Custom App size: ");
                    ImGui::SetNextItemWidth(120.f);
                    ImGui::SliderInt("Width", &CustomWidth, 1280, 3840);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(120.f);
                    ImGui::SliderInt("Height", &CustomHeight, 720, 2160);
                    ImGui::BulletText("Size Adjustment: ");
                    ImGui::SetNextItemWidth(120.f);
                    ImGui::DragFloat("Widgets", &WidgetResizeScale, 0.2f, 0.1f, 10.f);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(120.f);
                    ImGui::DragFloat("Font", &GlobalFontScaling, 0.2f, 0.1f, 5.0f);
                }
                ImGui::EndGroup();
                ImGui::Unindent();
            }
            ImGui::Separator();
            ImGui::BulletText("Yolo v7 Environment");
            ImGui::InputText("Python path", TempPythonPath, IM_ARRAYSIZE(TempPythonPath), ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            if (ImGui::Button("Choose"))
            {
                IsChoosingFolder = true;
                ifd::FileDialog::Instance().Open("ChoosePythonPath", "Choose python path", "");
            }
            ImGui::InputText("Yolo v7 path", TempYoloV7Path, IM_ARRAYSIZE(TempYoloV7Path),
                             ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            if (ImGui::Button("Choose Yolo V7 Folder"))
            {
                IsChoosingFolder = true;
                ifd::FileDialog::Instance().Open("ChooseYoloV7Path", "Choose yolo V7 path", "");
            }
            if (ImGui::Button("OK", ImVec2(ImGui::GetWindowWidth() * 0.2f, ImGui::GetFontSize() * 1.5f)))
            {
                ImGui::CloseCurrentPopup();
                IsModalOpen_Setting = false;
            }
            ImGui::EndPopup();
        }
    }

    void Modals::RenderAbout()
    {
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("About"))
        {
            ImGui::Text("About");
            if (ImGui::Button("OK", ImVec2(ImGui::GetWindowWidth() * 0.2f, ImGui::GetFontSize() * 1.5f)))
            {
                ImGui::CloseCurrentPopup();
                IsModalOpen_About = false;
            }
            ImGui::EndPopup();
        }
    }

    void Modals::RenderTutorial()
    {
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Tutorial"))
        {
            ImGui::Text("Tutorial");
            if (ImGui::Button("OK", ImVec2(ImGui::GetWindowWidth() * 0.2f, ImGui::GetFontSize() * 1.5f)))
            {
                ImGui::CloseCurrentPopup();
                IsModalOpen_Tutorial = false;
            }
            ImGui::EndPopup();
        }
    }

    void Modals::RenderContact()
    {
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Contact"))
        {
            ImGui::Text("Contact!!??");
            if (ImGui::Button("OK", ImVec2(ImGui::GetWindowWidth() * 0.2f, ImGui::GetFontSize() * 1.5f)))
            {
                ImGui::CloseCurrentPopup();
                IsModalOpen_Contact = false;
            }
            ImGui::EndPopup();
        }
    }

    void Modals::RenderLicense()
    {
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("License"))
        {
            ImGui::Text("License!!??");
            if (ImGui::Button("OK", ImVec2(ImGui::GetWindowWidth() * 0.2f, ImGui::GetFontSize() * 1.5f)))
            {
                ImGui::CloseCurrentPopup();
                IsModalOpen_License = false;
            }
            ImGui::EndPopup();
        }
    }

    void Modals::HandleFileDialogClose()
    {
        if (ifd::FileDialog::Instance().IsDone("ChooseExistingProjectLocationDialog"))
        {
            if (ifd::FileDialog::Instance().HasResult())
            {
                std::string RawString = ifd::FileDialog::Instance().GetResult().string();
                std::replace(RawString.begin(), RawString.end(), '\\', '/');
                strcpy(TempExistingProjectLocation, RawString.c_str());
            }
            ifd::FileDialog::Instance().Close();
            IsChoosingFolder = false;
        }
        if (ifd::FileDialog::Instance().IsDone("ChooseNewProjectLocationDialog"))
        {
            if (ifd::FileDialog::Instance().HasResult())
            {
                std::string RawString = ifd::FileDialog::Instance().GetResult().string();
                std::replace(RawString.begin(), RawString.end(), '\\', '/');
                strcpy(TempProjectLocation, RawString.c_str());
            }
            ifd::FileDialog::Instance().Close();
            IsChoosingFolder = false;
        }
        if (ifd::FileDialog::Instance().IsDone("ChoosePythonPath"))
        {
            if (ifd::FileDialog::Instance().HasResult())
            {
                std::string RawString = ifd::FileDialog::Instance().GetResult().string();
                std::replace(RawString.begin(), RawString.end(), '\\', '/');
                strcpy(TempPythonPath, RawString.c_str());
                Setting::Get().PythonPath = RawString;
            }
            ifd::FileDialog::Instance().Close();
            IsChoosingFolder = false;
        }
        if (ifd::FileDialog::Instance().IsDone("ChooseYoloV7Path"))
        {
            if (ifd::FileDialog::Instance().HasResult())
            {
                std::string RawString = ifd::FileDialog::Instance().GetResult().string();
                std::replace(RawString.begin(), RawString.end(), '\\', '/');
                strcpy(TempYoloV7Path, RawString.c_str());
                Setting::Get().YoloV7Path = RawString;
            }
            ifd::FileDialog::Instance().Close();
            IsChoosingFolder = false;
        }
    }
}
