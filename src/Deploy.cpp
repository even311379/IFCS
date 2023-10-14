﻿#include "Deploy.h"
#include <fstream>
#if defined _WINDOWS
#include <shellapi.h>
#endif

#include "misc/cpp/imgui_stdlib.h"
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include "yaml-cpp/yaml.h"
#include "addons/imguidatechooser.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"
#include "DataBrowser.h"
#include "Modals.h"
#include "Setting.h"
#include "Style.h"

namespace IFCS
{
    void Deploy::LoadConfigFile(const std::string& ConfigFilePath)
    {
        const YAML::Node ConfigFile = YAML::LoadFile(ConfigFilePath);
        Data = FDeploymentData(ConfigFile);
    }

    void Deploy::SaveConfigFile(const std::string& ConfigFilePath)
    {
        if (!std::filesystem::is_directory(ConfigFilePath)) return;
        YAML::Emitter Out;
        Out << Data.Serialize();
        std::ofstream(ConfigFilePath + "/DeployConfig.yaml") << Out.c_str();
    }

    void Deploy::RenderContent()
    {
        ImVec2 DeployWindowSize = ImGui::GetContentRegionAvail();
        DeployWindowSize.y -= 36;
        ImGui::BeginChild("DeployWindow", DeployWindowSize, false);
        {
            if (ImGui::TreeNodeEx("Input & Output", ImGuiTreeNodeFlags_DefaultOpen))
            {
                RenderInputOutput();
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx("Configure Task", ImGuiTreeNodeFlags_DefaultOpen))
            {
                RenderConfigureTask();
                ImGui::TreePop();
            }
            // TODO: temporarily remove all sms stuff...
            // if (ImGui::TreeNode("Notification"))
            // {
            //     RenderNotification();
            //     ImGui::TreePop();
            // }
        }
        ImGui::EndChild();
        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::Button(ICON_FA_UPLOAD))
        {
            ImGuiFileDialog::Instance()->OpenDialog("SaveDeployConfigFile", "Choose path to save configuration file", nullptr, Setting::Get().ProjectPath, 1, nullptr, ImGuiFileDialogFlags_Modal);
        }
        Utils::AddSimpleTooltip("Save current configuration to file");
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_DOWNLOAD))
        {
            ImGuiFileDialog::Instance()->OpenDialog("LoadDeployConfigFile", "Choose configuration file to load", ".yaml", Setting::Get().ProjectPath, 1, nullptr, ImGuiFileDialogFlags_Modal);
        }
        Utils::AddSimpleTooltip("Load configuration from file");
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 280);
        if (ImGui::Button("Generate Script", ImVec2(270, 0)))
        {
            if (Data.TaskOutputDir.empty()) return;
            // write config file
            YAML::Emitter Out;
            YAML::Node OutNode = Data.Serialize();
            OutNode["YoloV7Path"] = Setting::Get().YoloV7Path;
            Out << OutNode;
            std::ofstream fout(Data.TaskOutputDir + "/IFCS_DeployConfig.yaml");
            fout << Out.c_str();
            // copy python file
            std::string AppPath = Utils::ChangePathSlash(std::filesystem::current_path().u8string());
            std::filesystem::copy_file(AppPath + "/Scripts/deploy/Deploy.py", Data.TaskOutputDir + "/Deploy.py", std::filesystem::copy_options::overwrite_existing);
            std::filesystem::copy_file(AppPath + "/Scripts/deploy/individual_tracker.py", Data.TaskOutputDir + "/individual_tracker.py", std::filesystem::copy_options::overwrite_existing);
            // write run script
            std::ofstream ofs;
#if defined _WINDOWS            
            ofs.open(Data.TaskOutputDir + "/Run.bat");
            ofs << Setting::Get().PythonPath << "/python.exe Deploy.py";
            ofs.close();
            ofs.open(Data.TaskOutputDir + "/SendData.bat");
            ofs << Setting::Get().PythonPath << "/python.exe Deploy.py --send-data-only";
            ofs.close();            
            ShellExecuteA(NULL, "open", Data.TaskOutputDir.c_str(), NULL, NULL, SW_SHOWDEFAULT);
#else
            ofs.open(Data.TaskOutputDir + "/Run.sh");
            ofs << "#!/usr/bin/bash\n";
            ofs << Setting::Get().PythonPath << "/python Deploy.py";
            ofs.close();
            ofs.open(Data.TaskOutputDir + "/SendData.sh");
            ofs << "#!/usr/bin/bash\n";
            ofs << Setting::Get().PythonPath << "/python Deploy.py --send-data-only";
            
            std::string command = "chmod +x " + Data.TaskOutputDir + "/Run.sh";
            system(command.c_str());
            command = "chmod +x " + Data.TaskOutputDir + "/SendData.sh";
            system(command.c_str());
            command = "xdg-open " + Data.TaskOutputDir;
            system(command.c_str());
#endif            
        }
    }
    
    void Deploy::RenderInputOutput()
    {
        ImGui::SetNextItemWidth(480.f);
        ImGui::InputText("", &Data.TaskInputDir, ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("...##Btn_TaskInputDir"))
        {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseTaskInputDir", "Choose path which contains folders for each camera",
                                                    nullptr, Setting::Get().ProjectPath, 1, nullptr, ImGuiFileDialogFlags_Modal);
        }
        ImGui::SameLine();
        ImGui::Text("Input Path");
        if (!Data.Cameras.empty())
        {
            ImGui::BulletText("Detected Cameras:");
            ImGui::Indent();
            for (const auto& C : Data.Cameras)
            {
                 ImGui::Text(C.CameraName.c_str());
            }
            ImGui::Unindent();
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme),"The names MUST be the same with the data base!");
        }
                
        // choose store repo
        ImGui::SetNextItemWidth(480.f);
        ImGui::InputText("", &Data.TaskOutputDir, ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("...##Btn_TaskOutputDir"))
        {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseTaskOutputDir", "Choose automation task output directory", nullptr, Setting::Get().ProjectPath, 1, nullptr, ImGuiFileDialogFlags_Modal);
        }
        ImGui::SameLine();
        ImGui::Text("Output Path");

        // server data
        ImGui::Text("Data Visualization in web application");
        // server
        ImGui::SetNextItemWidth(256.f);
        ImGui::InputTextWithHint("Server address", "https://samplesite.com or http://143.198.216.92", &Data.ServerAddress);
        Utils::AddSimpleTooltip("The server address of your web application");
        // account
        ImGui::SetNextItemWidth(256.f);
        ImGui::InputText("User account", &Data.ServerAccount);
        Utils::AddSimpleTooltip("The user account in your web application");
        // password
        ImGui::SetNextItemWidth(256.f);
        ImGui::InputText("User password", &Data.ServerPassword, ImGuiInputTextFlags_Password);
        // auto send data?
        ImGui::Checkbox("Should send data automatically?", &Data.ShouldAutoSendServer);
        Utils::AddSimpleTooltip("If true, results will send to server automatically, otherwise, it will only store at your output path.");

        // dialogs in this section
        if (ImGuiFileDialog::Instance()->Display("ChooseTaskInputDir", ImGuiWindowFlags_NoCollapse, ImVec2(800, 600))) 
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                Data.TaskInputDir = Utils::ChangePathSlash(ImGuiFileDialog::Instance()->GetCurrentPath());
                Data.Cameras.clear();
                for (const auto& entry : std::filesystem::directory_iterator(Data.TaskInputDir))
                {
                    if (!is_directory(entry.path()))
                        continue;
                    FCameraSetup NewCamera;
                    NewCamera.CameraName = entry.path().filename().string();
                    Data.Cameras.emplace_back(NewCamera);
                }
            }
            // close
            ImGuiFileDialog::Instance()->Close();
        }
        if (ImGuiFileDialog::Instance()->Display("ChooseTaskOutputDir", ImGuiWindowFlags_NoCollapse, ImVec2(800, 600))) 
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                Data.TaskOutputDir = Utils::ChangePathSlash(ImGuiFileDialog::Instance()->GetCurrentPath());
            }
            ImGuiFileDialog::Instance()->Close();
        }
    }
    
    void Deploy::RenderConfigureTask()
    {
        if (ImGui::RadioButton("Now", Data.IsTaskStartNow == true)) { Data.IsTaskStartNow = true; } ImGui::SameLine();
        if (ImGui::RadioButton("Schedule", Data.IsTaskStartNow == false)) { Data.IsTaskStartNow = false; }
        if (Data.IsTaskStartNow)
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme),"Target for given dates");
        }
        else
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Routinely run it everyday at %d:%d. Target on clips from previous day.", Data.ScheduledRuntime[0], Data.ScheduledRuntime[1]);
        }
        
        if (Data.IsTaskStartNow)
        {
            // choose date range or multiple dates
            if (ImGui::RadioButton("Choose interval", Data.IsRunSpecifiedDates == false))
            {
                Data.IsRunSpecifiedDates = false;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Pick dates", Data.IsRunSpecifiedDates == true))
            {
                Data.IsRunSpecifiedDates = true;
            }
            if (Data.IsRunSpecifiedDates)
            {
                ImGui::PushID("DatesChooser");
                for (size_t j = 0; j < Data.RunDates.size(); j++)
                {
                    ImGui::PushID(static_cast<int>(j));
                    ImGui::SetNextItemWidth(256.f);
                    ImGui::DateChooser("", Data.RunDates[j], "%Y/%m/%d");
                    ImGui::PopID();
                }
                ImGui::PopID();

                ImGui::PushID("DatesControl");
                if (ImGui::Button(ICON_FA_PLUS))
                {
                    Data.RunDates.emplace_back(tm());
                }
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_MINUS))
                {
                    if (Data.RunDates.size() > 1)
                        Data.RunDates.pop_back();
                }
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_SYNC))
                {
                    Data.RunDates.clear();
                    Data.RunDates.emplace_back(tm());
                }
                ImGui::PopID();
            }
            else
            {
                ImGui::SetNextItemWidth(256.f);
                if (ImGui::DateChooser("Start Date", Data.StartDate, "%Y/%m/%d"))
                {
                    if (std::difftime(std::mktime(&Data.StartDate), std::mktime(&Data.EndDate)) > 0)
                    {
                        Data.EndDate = Data.StartDate;
                    }
                }
                ImGui::SetNextItemWidth(256.f);
                if (ImGui::DateChooser("End Date", Data.EndDate, "%Y/%m/%d"))
                {
                    if (std::difftime(std::mktime(&Data.StartDate), std::mktime(&Data.EndDate)) > 0)
                    {
                        Data.EndDate = Data.StartDate;
                    }
                }
            }
        }
        else
        {
            // choose start time
            ImGui::SetNextItemWidth(256.f);
            if (ImGui::DragInt2("Scheduled Runtime (hour : minute)", Data.ScheduledRuntime, 1, 0, 59))
            {
                if (Data.ScheduledRuntime[0] > 23)
                {
                    Data.ScheduledRuntime[0] = 23;
                }
                if (Data.ScheduledRuntime[1] > 59)
                {
                    Data.ScheduledRuntime[1] = 59;
                }
            }
        }
        // the data for each camera?

        ImGui::Text("Detection frequency");
        ImGui::SetNextItemWidth(256.f);
        ImGui::DragInt("Detection Period (every X seconds)", &Data.DetectionPeriodInSecond, 1, 1, 120, "%d Seconds");
        Utils::AddSimpleTooltip("Detect targets in screen every X seconds");
        ImGui::SetNextItemWidth(256.f);
        ImGui::DragInt("Pass Count Duration (minutes per hour)", &Data.PassCountDurationInMinutes, 1, 1, 59, "%d Minute");
        Utils::AddSimpleTooltip("Perform per frame detection to estimate pass count for first X minutes per hour");
        ImGui::Text("Camera Setup:");
        if (Data.Cameras.empty())
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "No camera clips folder detected! Choose input folder first!");
            return;
        }
        static int SelectedCameraIdx = 0;
        
        ImGui::Checkbox("Apply same model / detection parameters / pass count parameters to all cameras?", &Data.UseSameCameraSetup);

        ImGui::SetNextItemWidth(256.f);
        if (ImGui::BeginCombo("Select Camera", Data.Cameras[SelectedCameraIdx].CameraName.c_str()))
        {
            for (int i = 0; i < Data.Cameras.size(); i++)
            {
                if (ImGui::Selectable(Data.Cameras[i].CameraName.c_str()))
                {
                    SelectedCameraIdx = i;   
                }
            }
            ImGui::EndCombo();
        }
        
        // model parameters
        static std::vector<std::string> InternalModelOptions;
        if (InternalModelOptions.empty())
        {
            YAML::Node ModelData = YAML::LoadFile(Setting::Get().ProjectPath + "/Models/Models.yaml");
            for (YAML::const_iterator it = ModelData.begin(); it != ModelData.end(); ++it)
            {
                InternalModelOptions.emplace_back(it->first.as<std::string>());
            }
        }
        if (ImGui::TreeNode("Choose model:"))
        {
            if (ImGui::Checkbox("Use external model?", &Data.Cameras[SelectedCameraIdx].UseExternalModel))
            {
                if (Data.UseSameCameraSetup)
                {
                    for (auto& C : Data.Cameras)
                    {
                        C.ModelName.clear();
                        C.ModelFilePath.clear();
                    }
                }
                else
                {
                    Data.Cameras[SelectedCameraIdx].ModelName.clear();
                    Data.Cameras[SelectedCameraIdx].ModelFilePath.clear();
                }
            }
            if (Data.Cameras[SelectedCameraIdx].UseExternalModel)
            {
                ImGui::SetNextItemWidth(480.f);
                ImGui::InputText("", &Data.Cameras[SelectedCameraIdx].ModelFilePath, ImGuiInputTextFlags_ReadOnly);
                ImGui::SameLine();
                if (ImGui::Button("...##Btn_ExternModelFile"))
                {
                    ImGuiFileDialog::Instance()->OpenDialog("ChooseExternalModelPath_Deploy", "Choose external model file", ".pt", Setting::Get().ProjectPath, 1, nullptr, ImGuiFileDialogFlags_Modal);
                }
                ImGui::SameLine();
                ImGui::Text("Choose external model file");
                
                ImGui::SetNextItemWidth(256.f);
                if (ImGui::InputText("Model name", &Data.Cameras[SelectedCameraIdx].ModelName) && Data.UseSameCameraSetup)
                {
                    std::string NewName = Data.Cameras[SelectedCameraIdx].ModelName; 
                    for (auto& C : Data.Cameras)
                    {
                        C.ModelName = NewName;
                    }
                }

                // the impl of file dialog
                if (ImGuiFileDialog::Instance()->Display("ChooseExternalModelPath_Deploy", ImGuiWindowFlags_NoCollapse, ImVec2(800, 600))) 
                {
                    if (ImGuiFileDialog::Instance()->IsOk())
                    {
                        const std::string ModelFilePath = Utils::ChangePathSlash(ImGuiFileDialog::Instance()->GetCurrentFileName());
                        if (Data.UseSameCameraSetup)
                        {
                            for (auto& C : Data.Cameras)
                            {
                                C.ModelFilePath = ModelFilePath;
                            }
                        }
                        else
                        {
                            Data.Cameras[SelectedCameraIdx].ModelFilePath = ModelFilePath;
                        }
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
            }
            else
            {
                ImGui::SetNextItemWidth(256.f);
                if (ImGui::BeginCombo("Choose Model", Data.Cameras[SelectedCameraIdx].ModelName.c_str()))
                {
                    for (const std::string& Name : InternalModelOptions)
                    {
                        if (ImGui::Selectable(Name.c_str(), Name == Data.Cameras[SelectedCameraIdx].ModelName))
                        {
                            if (Data.UseSameCameraSetup)
                            {
                                for (auto& C : Data.Cameras)
                                {
                                    C.ModelName = Name;
                                    C.ModelFilePath = Setting::Get().ProjectPath + "/Models/" + Name + "/weights/best.pt";
                                }
                            }
                            else
                            {
                                Data.Cameras[SelectedCameraIdx].ModelName = Name;
                                Data.Cameras[SelectedCameraIdx].ModelFilePath = Setting::Get().ProjectPath + "/Models/" + Name + "/weights/best.pt";
                            }
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            ImGui::TreePop();
        }
    
        // detection parameters
        static std::string LoadedDetectionParamName = "";
        if (ImGui::TreeNode("Set detection parameter"))
        {
            ImGui::SetNextItemWidth(256.f);
            if (ImGui::BeginCombo("Load detection parameters from existing one?", LoadedDetectionParamName.c_str()))
            {
                YAML::Node DetectionData = YAML::LoadFile(Setting::Get().ProjectPath + "/Detections/Detections.yaml");
                for (YAML::const_iterator it = DetectionData.begin(); it != DetectionData.end(); ++it)
                {
                    std::string DetectionName = it->first.as<std::string>();
                    if (ImGui::Selectable(DetectionName.c_str(), DetectionName == LoadedDetectionParamName))
                    {
                        LoadedDetectionParamName = DetectionName;
                        if (Data.UseSameCameraSetup)
                        {
                            for (auto& C : Data.Cameras)
                            {
                                C.ImageSize = it->second["ImageSize"].as<int>();
                                C.Confidence = it->second["Confidence"].as<float>();
                                C.IOU = it->second["IOU"].as<float>();
                            }
                        }
                        else
                        {
                            Data.Cameras[SelectedCameraIdx].ImageSize = it->second["ImageSize"].as<int>();
                            Data.Cameras[SelectedCameraIdx].Confidence = it->second["Confidence"].as<float>();
                            Data.Cameras[SelectedCameraIdx].IOU = it->second["IOU"].as<float>();
                        }
                    }
                }
                ImGui::EndCombo();
            }
            if (Data.UseSameCameraSetup)
            {
                ImGui::SetNextItemWidth(256.f);
                if (ImGui::DragInt("Image Size", &Data.Cameras[SelectedCameraIdx].ImageSize, 32, 128, 1024))
                {
                    int NewValue = Data.Cameras[0].ImageSize;
                    for (auto& C : Data.Cameras)
                        C.ImageSize = NewValue;
                }
                ImGui::SetNextItemWidth(256.f);
                if (ImGui::DragFloat("Confidence", &Data.Cameras[SelectedCameraIdx].Confidence, 0.05f, 0.01f, 0.95f, "%.2f"))
                {
                    float NewValue = Data.Cameras[0].Confidence;
                    for (auto& C : Data.Cameras)
                        C.Confidence = NewValue;
                }
                ImGui::SetNextItemWidth(256.f);
                if (ImGui::DragFloat("IOU", &Data.Cameras[SelectedCameraIdx].IOU, 0.05f, 0.01f, 0.95f, "%.2f"))
                {
                    float NewValue = Data.Cameras[0].IOU;
                    for (auto& C : Data.Cameras)
                        C.IOU = NewValue;
                }
            } else
            {
                ImGui::SetNextItemWidth(256.f);
                ImGui::DragInt("Image Size", &Data.Cameras[SelectedCameraIdx].ImageSize, 32, 128, 1024);
                ImGui::SetNextItemWidth(256.f);
                ImGui::DragFloat("Confidence", &Data.Cameras[SelectedCameraIdx].Confidence, 0.05f, 0.01f, 0.95f, "%.2f");
                ImGui::SetNextItemWidth(256.f);
                ImGui::DragFloat("IOU", &Data.Cameras[SelectedCameraIdx].IOU, 0.05f, 0.01f, 0.95f, "%.2f");
            }
            if (ImGui::Checkbox("Should apply ROI on detection?", &Data.Cameras[SelectedCameraIdx].ShouldApplyDetectionROI))
            {
                if (Data.UseSameCameraSetup)
                {
                    bool NewValue = Data.Cameras[SelectedCameraIdx].ShouldApplyDetectionROI;
                    for (auto& C : Data.Cameras)
                    {
                        C.ShouldApplyDetectionROI = NewValue;
                        C.DetectionROI = {0.f, 0.f, 1.f, 1.f};
                    }
                }
                else
                {
                    Data.Cameras[SelectedCameraIdx].DetectionROI = {0.f, 0.f, 1.f, 1.f};
                }
            }
            if (Data.Cameras[SelectedCameraIdx].ShouldApplyDetectionROI)
            {
                ImGui::SetNextItemWidth(480.f);
                if (ImGui::SliderFloat4("ROI (x1, y1, x2, y2)", Data.Cameras[SelectedCameraIdx].DetectionROI.data(), 0.f, 1.0f) && Data.UseSameCameraSetup)
                {
                    std::array<float,4> NewValue = Data.Cameras[SelectedCameraIdx].DetectionROI;
                    for (auto& C : Data.Cameras)
                    {
                        C.DetectionROI = NewValue;
                    }
                }
            }
            ImGui::TreePop();
        }

        // pass count parameters
        if (ImGui::TreeNode("Set pass count parameters"))
        {
            if (ImGui::RadioButton("Vertical", Data.Cameras[SelectedCameraIdx].IsPassVertical))
            {
                if (Data.UseSameCameraSetup)
                {
                    for (auto& C : Data.Cameras)
                        C.IsPassVertical = true;
                }
                else
                {
                    Data.Cameras[SelectedCameraIdx].IsPassVertical = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Horizontal", !Data.Cameras[SelectedCameraIdx].IsPassVertical))
            {
                if (Data.UseSameCameraSetup)
                {
                    for (auto& C : Data.Cameras)
                        C.IsPassVertical = false;
                }
                else
                {
                    Data.Cameras[SelectedCameraIdx].IsPassVertical = false;
                }
            }
            ImGui::SetNextItemWidth(256.f);
            if (ImGui::SliderFloat2("Fish way start / end", Data.Cameras[SelectedCameraIdx].FishwayStartEnd.data() , 0.f, 1.f) && Data.UseSameCameraSetup)
            {
                std::array<float, 2> NewValue = Data.Cameras[SelectedCameraIdx].FishwayStartEnd;
                for (auto& C : Data.Cameras)
                    C.FishwayStartEnd = NewValue;
            }
            if (ImGui::Checkbox("Add fishway middle line?", &Data.Cameras[SelectedCameraIdx].UseFishwayMiddle))
            {
                Data.Cameras[SelectedCameraIdx].FishwayMiddle = Data.Cameras[SelectedCameraIdx].FishwayStartEnd[1];
                bool NewValue = Data.Cameras[SelectedCameraIdx].UseFishwayMiddle;
                if (Data.UseSameCameraSetup)
                {
                    for (auto& C : Data.Cameras)
                    {
                        C.UseFishwayMiddle = NewValue;
                        C.FishwayMiddle = C.FishwayStartEnd[1];
                    }
                }
            }
            ImGui::SetNextItemWidth(256.f);
            if (Data.Cameras[SelectedCameraIdx].UseFishwayMiddle)
            {
                if (ImGui::SliderFloat("Fishway middle", &Data.Cameras[SelectedCameraIdx].FishwayMiddle , 0.f, 1.f) && Data.UseSameCameraSetup)
                {
                    float NewValue = Data.Cameras[SelectedCameraIdx].FishwayMiddle;
                    for (auto& C : Data.Cameras)
                        C.FishwayMiddle = NewValue;
                }
            }
            
            if (ImGui::Checkbox("Enable max speed threshold?", &Data.Cameras[SelectedCameraIdx].ShouldEnableSpeedThreshold) && Data.UseSameCameraSetup)
            {
                bool NewValue = Data.Cameras[SelectedCameraIdx].ShouldEnableSpeedThreshold;
                for (auto& C : Data.Cameras)
                    C.ShouldEnableSpeedThreshold = NewValue;
            }
            
            if (ImGui::Checkbox("Enable similar body size threshold?", &Data.Cameras[SelectedCameraIdx].ShouldEnableBodySizeThreshold) && Data.UseSameCameraSetup)
            {
                bool NewValue = Data.Cameras[SelectedCameraIdx].ShouldEnableBodySizeThreshold;
                for (auto& C : Data.Cameras)
                    C.ShouldEnableBodySizeThreshold = NewValue;
            }
            
            if (Data.Cameras[SelectedCameraIdx].ShouldEnableSpeedThreshold)
            {
                ImGui::SetNextItemWidth(256.f);
                if (ImGui::DragFloat("Max speed threshold", &Data.Cameras[SelectedCameraIdx].SpeedThreshold, 0.01f, 0.01f, 0.5f, "%.2f") && Data.UseSameCameraSetup)
                {
                    float NewValue = Data.Cameras[SelectedCameraIdx].SpeedThreshold;
                    for (auto& C : Data.Cameras)
                    {
                        C.SpeedThreshold = NewValue;
                    }
                }
            }
            if (Data.Cameras[SelectedCameraIdx].ShouldEnableBodySizeThreshold)
            {
                ImGui::SetNextItemWidth(256.f);
                if (ImGui::DragFloat("Body size threshold", &Data.Cameras[SelectedCameraIdx].BodySizeThreshold, 0.01f, 0.01f, 0.5f, "%.2f") && Data.UseSameCameraSetup)
                {
                float NewValue = Data.Cameras[SelectedCameraIdx].BodySizeThreshold;
                for (auto& C : Data.Cameras)
                    C.BodySizeThreshold = NewValue;
                }
            }
            ImGui::SetNextItemWidth(256.f);
            if (ImGui::DragInt("Frames buffer threshold", &Data.Cameras[SelectedCameraIdx].FrameBufferSize, 1, 1, 60) && Data.UseSameCameraSetup)
            {
                int NewValue = Data.Cameras[SelectedCameraIdx].FrameBufferSize;
                for (auto& C : Data.Cameras)
                    C.FrameBufferSize = NewValue;
            }
            ImGui::SetNextItemWidth(256.f);
            if (ImGui::InputText("Other category name", &Data.Cameras[SelectedCameraIdx].OtherCategoryName))
            {
                std::string NewValue =  Data.Cameras[SelectedCameraIdx].OtherCategoryName;
                for (auto& C: Data.Cameras)
                    C.OtherCategoryName = NewValue;
            }
            ImGui::TreePop();
        }
    }

}