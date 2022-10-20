#include "Setting.h"

#include <filesystem>
#include <fstream>
#include <stdbool.h>
#include <spdlog/spdlog.h>

#include "Annotation.h"
#include "CategoryManagement.h"
#include "DataBrowser.h"
#include "FrameExtractor.h"
#include "ModelGenerator.h"
#include "ModelViewer.h"
#include "Panel.h"
#include "Prediction.h"
#include "TrainingSetGenerator.h"
#include "TrainingSetViewer.h"
#include "ImFileDialog/ImFileDialog.h"
#include "Spectrum/imgui_spectrum.h"
#include "yaml-cpp/yaml.h"

namespace IFCS
{
    Setting::Setting()
    {
    }


    // TODO: the way modal works is very different from example code, what could it get wrong?
    void Setting::RenderModal()
    {
        const ImVec2 Center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Setting", &IsModalOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::BulletText("Choose Theme:");
            ImGui::SameLine();
            if (ImGui::RadioButton("Light", &ThemeToUse, 0))
            {
                Theme = ETheme::Light;
                Spectrum::StyleColorsSpectrum();
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Dark", &ThemeToUse, 1))
            {
                Theme = ETheme::Dark;
                Spectrum::StyleColorsSpectrum(false);
            }
            ImGui::BulletText("Prefered Language:");
            ImGui::SameLine();
            // TODO: this is a static way... not considering future expansion yet...
            const static char* LanguageOptions[] = {"English", "Traditional Chinese"};
            ImGui::SetNextItemWidth(240.f);
            ImGui::Combo("##Language_options", &LanguageToUse, LanguageOptions, IM_ARRAYSIZE(LanguageOptions));
            const static char* AppSizeOptions[] = {
                "FHD (1920 x 1080)", "2K (2560 x 1440)", "4k (3840 x 2160)", "Custom"
            };
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
            // TODO: python only!!! no need for conda and env!!...
            ImGui::Text("Yolo v7 Environment");
            ImGui::InputText("##ToConda", TempCondaPath, IM_ARRAYSIZE(TempCondaPath), ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            if (ImGui::Button("Choose Conda Folder"))
            {
                ifd::FileDialog::Instance().Open("ChooseCondaFolder", "Choose conda folder", "");
            }
            if (ImGui::InputText("Env Name", TempPythonEnv, IM_ARRAYSIZE(TempPythonEnv)))
            {
                PythonEnv = TempPythonEnv;
            }
            if (ImGui::Button("Create Env and Set?"))
            {
                CreateEnv();
            }
            ImGui::InputText("##ToYolov7", TempYoloV7Path, IM_ARRAYSIZE(TempYoloV7Path), ImGuiInputTextFlags_ReadOnly);
            if (ImGui::Button("Download and Set?"))
            {
                DownloadYoloV7();
            }
            ImGui::SameLine();
            if (ImGui::Button("Choose Yolo V7 Folder"))
            {
                ifd::FileDialog::Instance().Open("ChooseYoloV7Folder", "Choose yoloV7 folder", "");
            }
            if (ImGui::Button("OK", ImVec2(ImGui::GetWindowWidth() * 0.2f, ImGui::GetFontSize() * 1.5f)))
            {
                ImGui::CloseCurrentPopup();
                IsModalOpen = false;
            }
            ImGui::EndPopup();
        }
    }

    void Setting::LoadEditorIni()
    {
        YAML::Node EditorIni = YAML::LoadFile("Config/Editor.ini");
        if (!EditorIni["LastOpenProject"]) return;
        if (!EditorIni["RecentProjects"]) return;
        ProjectPath = EditorIni["LastOpenProject"].as<std::string>();
        if (EditorIni["RecentProjects"].IsSequence())
        {
            for (std::size_t i = 0; i < EditorIni["RecentProjects"].size(); i++)
            {
                RecentProjects.insert(EditorIni["RecentProjects"][i].as<std::string>());
            }
        }
        LoadUserIni();
    }

    void Setting::LoadUserIni()
    {
        YAML::Node UserIni = YAML::LoadFile(ProjectPath + std::string("/IFCSUser.ini"));
        Project = UserIni["Project"].as<std::string>();
        PreferredLanguage = static_cast<ESupportedLanguage>(UserIni["PreferredLanguage"].as<int>());
        ActiveWorkspace = static_cast<EWorkspace>(UserIni["ActiveWorkspace"].as<int>());
        Theme = static_cast<ETheme>(UserIni["Theme"].as<int>());
        CondaPath = UserIni["CondaPath"].as<std::string>();
        PythonEnv = UserIni["PythonEnv"].as<std::string>();
        YoloV7Path = UserIni["YoloV7Path"].as<std::string>();
        strcpy(TempCondaPath, CondaPath.c_str());
        strcpy(TempPythonEnv, PythonEnv.c_str());
        strcpy(TempYoloV7Path, YoloV7Path.c_str());
        ProjectIsLoaded = true;
    }

    void Setting::Save()
    {
        // save editor.ini
        YAML::Emitter editor_out;
        editor_out << YAML::BeginMap;
        editor_out << YAML::Key << "LastOpenProject";
        editor_out << YAML::Value << ProjectPath;
        editor_out << YAML::Key << "RecentProjects";
        editor_out << YAML::BeginSeq;
        for (const auto& p : RecentProjects)
        {
            editor_out << p;
        }
        editor_out << YAML::EndSeq;

        std::ofstream fout("Config/Editor.ini");
        fout << editor_out.c_str();

        // TODO: maybe use enum string?   or not?
        // save user.init
        YAML::Emitter user_out;
        user_out << YAML::BeginMap;
        user_out << YAML::Key << "Project";
        user_out << YAML::Value << Project;
        user_out << YAML::Key << "ActiveWorkspace";
        user_out << YAML::Value << static_cast<int>(ActiveWorkspace);
        user_out << YAML::Key << "PreferredLanguage";
        user_out << YAML::Value << static_cast<int>(PreferredLanguage);
        user_out << YAML::Key << "Theme";
        user_out << YAML::Value << static_cast<int>(Theme);

        // save as local file
        std::ofstream fout_2(ProjectPath + std::string("/IFCSUser.ini"));
        fout_2 << user_out.c_str();
        ProjectIsLoaded = true;
    }

    void Setting::Tick1()
    {
        SetWorkspace(ActiveWorkspace);
    }

    void Setting::CreateStartup()
    {
        RecentProjects.insert(ProjectPath);
        std::filesystem::create_directories(ProjectPath); // TODO: can not use chinese folder name
        std::filesystem::create_directories(ProjectPath + std::string("/TrainingClips"));
        // lack of ..."i"...tra_ning ... waste me 2 hours...
        std::filesystem::create_directories(ProjectPath + std::string("/TrainingImages"));
        std::filesystem::create_directories(ProjectPath + std::string("/Models"));
        // TODO: check what to create in later dev cycle
        std::filesystem::create_directories(ProjectPath + std::string("/Prediction"));
        std::filesystem::create_directories(ProjectPath + std::string("/Data"));
        // should create empty files for future use...
        std::ofstream output1(ProjectPath + std::string("/Data/Annotation.yaml"));
        std::ofstream output2(ProjectPath + std::string("/Data/Categories.yaml"));
        std::ofstream output3(ProjectPath + std::string("/Data/ExtractionRegions.yaml"));
        Save();
    }

    void Setting::SetWorkspace(EWorkspace NewWorkspace)
    {
        Annotation::Get().SetVisibility(false);
        FrameExtractor::Get().SetVisibility(false);
        CategoryManagement::Get().SetVisibility(false);
        TrainingSetGenerator::Get().SetVisibility(false);
        TrainingSetViewer::Get().SetVisibility(false);
        ModelGenerator::Get().SetVisibility(false);
        ModelViewer::Get().SetVisibility(false);
        Prediction::Get().SetVisibility(false);
        DataBrowser::Get().SetVisibility(true);
        ActiveWorkspace = NewWorkspace;
        switch (ActiveWorkspace)
        {
        case EWorkspace::Data:
            Annotation::Get().SetVisibility(true);
            FrameExtractor::Get().SetVisibility(true);
            CategoryManagement::Get().SetVisibility(true);
            BGPanel::Get().SetDataWksNow = true;
            break;
        case EWorkspace::Train:
            TrainingSetGenerator::Get().SetVisibility(true);
            TrainingSetViewer::Get().SetVisibility(true);
            ModelGenerator::Get().SetVisibility(true);
            ModelViewer::Get().SetVisibility(true);
            BGPanel::Get().SetTrainWksNow = true;
            break;
        case EWorkspace::Predict:
            Prediction::Get().SetVisibility(true);
            BGPanel::Get().SetPredictWksNow = true;
            break;
        }
    }

    bool Setting::IsYoloEnvSet() const
    {
        return !CondaPath.empty() && !YoloV7Path.empty() && !PythonEnv.empty();
    }

    // TODO: you know what to do, right?
    void Setting::DownloadYoloV7()
    {
    }

    void Setting::CreateEnv()
    {
    }
}
