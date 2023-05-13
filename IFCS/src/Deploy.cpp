#include "Deploy.h"

#include <fstream>

#include "opencv2/opencv.hpp"
#include "backends/imgui_impl_glfw.h"
#include <GLFW/glfw3.h>

#include "misc/cpp/imgui_stdlib.h"

#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include "yaml-cpp/yaml.h"

#include "addons/imguidatechooser.h"
#include "ImFileDialog/ImFileDialog.h"

#include "DataBrowser.h"
#include "imgui_internal.h"
#include "Modals.h"
#include "Setting.h"
#include "Style.h"


namespace IFCS
{
    enum class EActiveDeployTab : uint8_t
    {
        Automation,
        FeasibilityTest,
        DataPipeline,
        SMSNotification,
    };
    
    static EActiveDeployTab ActiveDeployTab = EActiveDeployTab::Automation;

    void Deploy::SetInputPath(const char* NewPath)
    {
        Data.TaskInputDir = NewPath;
    }

    void Deploy::SetOutputPath(const char* NewPath)
    {
        Data.TaskOutputDir = NewPath;
    }

    void Deploy::SetReferenceImagePath(const char* NewPath)
    {
        ReferenceImages.clear();
        CurrentRefImgIndex = 0;
        strcpy(ReferenceImagePath, NewPath);
        for (auto& entry : std::filesystem::directory_iterator(ReferenceImagePath))
        {
            if (entry.is_directory()) continue;
            for (const std::string& Format : DataBrowser::Get().AcceptedImageFormat )
            {
                if (Utils::InsensitiveStringCompare(entry.path().extension().string(), Format))
                {
                    ReferenceImages.emplace_back(entry.path().string());
                }
            }
        }
        if (!ReferenceImages.empty())
        {
            UpdateReferenceImage();
        }
    }

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

    void Deploy::SetExternalModelFile(const std::string& NewModelFile)
    {
        Data.Cameras[CameraIndex_DP].ModelFilePath = NewModelFile;
    }

    void Deploy::GenerateRunScript(const std::string& ScriptsLocation)
    {
        // write config file
        YAML::Emitter Out;
        YAML::Node OutNode = Data.Serialize();
        OutNode["YoloV7Path"] = Setting::Get().YoloV7Path;
        Out << OutNode;
        std::ofstream(ScriptsLocation + "/IFCS_DeployConfig.yaml") << Out.c_str();
        // copy python file
        std::filesystem::copy_file("Scripts/IFCS_Deploy.py", ScriptsLocation + "/IFCS_Deploy.py", std::filesystem::copy_options::overwrite_existing);
        // write run script
        std::ofstream ofs;
        ofs.open(ScriptsLocation + "/RUN.bat");
        ofs << Setting::Get().PythonPath << "/python.exe IFCS_Deploy.py";
        ofs.close();
    }

    void Deploy::RenderContent()
    {
        ImVec2 DeployWindowSize = ImGui::GetContentRegionAvail();
        DeployWindowSize.y -= 36;
        ImGui::BeginChild("DeployWindow", DeployWindowSize, false);
        {
            if (ImGui::BeginTabBar("DeployTabs"))
            {
                if (ImGui::BeginTabItem("Automation"))
                {
                    RenderAutomation();
                    ActiveDeployTab = EActiveDeployTab::Automation;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Feasibility Test"))
                {
                    RenderFeasibilityTest();
                    ActiveDeployTab = EActiveDeployTab::FeasibilityTest;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Data pipeline"))
                {
                    RenderDataPipeline();
                    ActiveDeployTab = EActiveDeployTab::DataPipeline;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("SMS Notification"))
                {
                    RenderSMSNotification();
                    ActiveDeployTab = EActiveDeployTab::SMSNotification;
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::EndChild();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (ImGui::Button(ICON_FA_UPLOAD))
        {
            Modals::Get().IsChoosingFolder = true;
            ifd::FileDialog::Instance().Open("SaveDeployConfigFile", "Choose path to save configuration file",
                "", false, Setting::Get().ProjectPath);
        }
        Utils::AddSimpleTooltip("Save current configuration to file");
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_DOWNLOAD))
        {
            Modals::Get().IsChoosingFolder = true;
            ifd::FileDialog::Instance().Open("LoadDeployConfigFile", "Choose configuration file to load",
                "config file {.yaml}", false, Setting::Get().ProjectPath);
        }
        Utils::AddSimpleTooltip("Load configuration from file");
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 280);
        if (ImGui::Button("generate script", ImVec2(270, 0)))
        {
            Modals::Get().IsChoosingFolder = true;
            ifd::FileDialog::Instance().Open("GenerateDeployScriptsLocation", "Choose location to generate run script",
                "", false, Setting::Get().ProjectPath);
        }
    }


    
    void Deploy::RenderAutomation()
    {
        // setup for which camera?
        if (ActiveDeployTab != EActiveDeployTab::Automation)
        {
            if (Data.Cameras.empty())
            {
                Data.Cameras.emplace_back();
            }
        }

        // TODO: maybe replace file dialog by native file dialoge to prevent trailing slash issues?
        ImGui::BulletText("Choose input and output paths for the automation task");
        ImGui::Indent();

        // choose target clips repo (input)
        ImGui::SetNextItemWidth(480.f);
        ImGui::InputText("", &Data.TaskInputDir, ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("...##Btn_TaskInputDir"))
        {
            Modals::Get().IsChoosingFolder = true;
            ifd::FileDialog::Instance().Open("ChooseTaskInputDir", "Choose automation task input directory (DVR clips repo)",
                "", false, Setting::Get().ProjectPath);
        }
        ImGui::SameLine();
        ImGui::Text("Input Path");
        
        // choose store repo
        ImGui::SetNextItemWidth(480.f);
        ImGui::InputText("", &Data.TaskOutputDir, ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("...##Btn_TaskOutputDir"))
        {
            Modals::Get().IsChoosingFolder = true;
            ifd::FileDialog::Instance().Open("ChooseTaskOutputDir", "Choose automation task output directory",
                "", false, Setting::Get().ProjectPath);
        }
        ImGui::SameLine();
        ImGui::Text("Output Path");
        ImGui::Unindent();

        static bool IsDVRNameDifferent = true;
        // register cameras
        ImGui::BulletText("Register cameras");
        ImGui::Indent();
        ImGui::Checkbox("Camera name in DVR and in DataBase is different?", &IsDVRNameDifferent);
        Utils::AddSimpleTooltip("In case your DVR system uses a different name for the camera than the name in the database, "
                                "check this option and fill the DVR name in the right column");
        ImGui::PushID("CameraName");
        int i = 0;
        for (auto& Camera : Data.Cameras)
        {
            ImGui::PushID(i);
            ImGui::SetNextItemWidth(240.f);
            ImGui::InputText("", &Camera.CameraName);
            if (IsDVRNameDifferent)
            {
                ImGui::SameLine();
                ImGui::Text(ICON_FA_ARROW_LEFT);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(240.f);
                ImGui::InputText("(DVR)", &Camera.DVRName);
            }
            ImGui::PopID();
            i++;
        }
        ImGui::PopID();
        
        if (ImGui::Button(ICON_FA_PLUS))
        {
            if (Data.Cameras.size() < 6)
            {
                Data.Cameras.emplace_back();
            }
            
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_MINUS))
        {
            if (Data.Cameras.size() > 1)
            {
                Data.Cameras.pop_back();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_SYNC))
        {
            Data.Cameras.clear();
            Data.Cameras.emplace_back();
        }
        ImGui::Unindent();

        // choose task mode
        /*
         * 1) run it for particular dates right now
         * 2) run it for yesterday everyday
         */
        ImGui::BulletText("Task mode");
        ImGui::Indent();
        if (ImGui::RadioButton("Schedule", Data.IsTaskStartNow == false)) { Data.IsTaskStartNow = false; } ImGui::SameLine();
        if (ImGui::RadioButton("Now", Data.IsTaskStartNow == true)) { Data.IsTaskStartNow = true; }
        if (Data.IsTaskStartNow)
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme),"Run right now for given dates");
        }
        else
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Run at given time everyday for yesterday's clips");
        }
        
        if (!Data.IsTaskStartNow)
        {
            // mode 1
            // choose start time

            ImGui::SetNextItemWidth(240.f);
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
        else
        {
            // mode 2
            // choose date range or mutiple dates
            if (ImGui::RadioButton("Choose interval", Data.IsRunSpecifiedDates == false))
                { Data.IsRunSpecifiedDates = false; }
            ImGui::SameLine();
            if (ImGui::RadioButton("Pick dates", Data.IsRunSpecifiedDates == true))
            {
                Data.IsRunSpecifiedDates = true;
            }
            if (!Data.IsRunSpecifiedDates)
            {
                ImGui::SetNextItemWidth(360.f);
                if (ImGui::DateChooser("Start Date", Data.StartDate, "%Y/%m/%d"))
                {
                    if (std::difftime(std::mktime(&Data.StartDate), std::mktime(&Data.EndDate)) > 0)
                    {
                        Data.EndDate = Data.StartDate;
                    }
                }
                ImGui::SetNextItemWidth(360.f);
                if (ImGui::DateChooser("End Date", Data.EndDate, "%Y/%m/%d"))
                {
                    if (std::difftime(std::mktime(&Data.StartDate), std::mktime(&Data.EndDate)) > 0)
                    {
                        Data.EndDate = Data.StartDate;
                    }
                }
            }
            else
            {
                ImGui::PushID("DatesChooser");
                for (size_t j = 0; j < Data.RunDates.size(); j++)
                {
                    ImGui::PushID(j);
                    ImGui::SetNextItemWidth(240.f);
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
                    if (Data.RunDates.size() == 1) return;
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
        }
        ImGui::Unindent();
        
        // how to handle these clips when done?
        ImGui::BulletText("Post processing");
        ImGui::Indent();
        // enable important region backup
        ImGui::Checkbox("Backup clips with important regions", &Data.ShouldBackupImportantRegions);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(32.f);
        ImGui::DragInt("Buffer time (minutes)", &Data.BackupBufferTime, 1, 1, 20, "%d");
        Utils::AddSimpleTooltip("extend the duration before and after the detected frame by buffer time as important region");
        // enable combined clips backup
        ImGui::Checkbox("Backup combinded clip", &Data.ShouldBackupCombinedClips);
        // delete raw clips after backup?
        ImGui::Checkbox("Delete raw clips after backup?", &Data.ShouldDeleteRawClips);
        ImGui::Unindent();
    }

    static ImVec2 ReferenceImagePos;
    static bool bAdding;
    static ImVec2 AddPointStart;
    static ImVec2 AddPointEnd;
    static ImVec4 HintColor = Style::RED(400, Setting::Get().Theme);

    void Deploy::CleanRegisteredCameras()
    {
        Data.Cameras.erase(
            std::remove_if( Data.Cameras.begin(), Data.Cameras.end(), [](const FCameraSetup& camera) { return camera.CameraName.empty(); } ),
            Data.Cameras.end()
            );
    }


    void Deploy::RenderFeasibilityTest()
    {
        // setup for which camera?
        if (ActiveDeployTab != EActiveDeployTab::FeasibilityTest)
        {
            CleanRegisteredCameras();
        }

        if (Data.Cameras.size() == 0)
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "No camera is registered! Checkout autotation tab first!");
            return;
        }

        // Choose camera combo
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x * 0.5f - 180.f);
        ImGui::SetNextItemWidth(360.f);
        if (ImGui::BeginCombo("Choose Camera", Data.Cameras[CameraIndex_FT].CameraName.c_str()))
        {
            for (size_t i = 0; i < Data.Cameras.size(); i++)
            {
                if (ImGui::Selectable(Data.Cameras[i].CameraName.c_str(), CameraIndex_FT == i))
                {
                    CameraIndex_FT = i;
                }
            }
            ImGui::EndCombo();
        }
        

        // image widget and draw boxes
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - WorkArea[0]) * 0.5f);
        ReferenceImagePos = ImGui::GetCursorScreenPos();
        ImGui::BeginChild("ReferenceImage", WorkArea);
        {
            ImGui::Image((void*)(intptr_t)ReferenceImagePtr, WorkArea);
            if (bAdding)
                RenderAddingProgress(AddPointStart);
            RenderTestZones(ReferenceImagePos);
        }
        ImGui::EndChild();
        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseClicked(0))
            {
                AddPointStart = ImGui::GetMousePos();
                bAdding = true;
            }
        }
        
        // handle final input
        if (bAdding && ImGui::IsMouseReleased(0))
        {
            bAdding = false;
            
            // add to feasible zones
            FFeasibleZone NewZone;
            NewZone.XYWH[0] = (abs(AddPointStart.x + AddPointEnd.x) / 2.f - ReferenceImagePos.x) / WorkArea.x; 
            NewZone.XYWH[1] = (abs(AddPointStart.y + AddPointEnd.y) / 2.f - ReferenceImagePos.y) / WorkArea.y; 
            NewZone.XYWH[2] = abs(AddPointStart.x - AddPointEnd.x) / WorkArea.x;
            NewZone.XYWH[3] = abs(AddPointStart.y - AddPointEnd.y) / WorkArea.y;
            // spdlog::info("is adding new zone with xywh: {}, {}, {}, {}  ... correct?", NewZone.XYWH[0], NewZone.XYWH[1], NewZone.XYWH[2], NewZone.XYWH[3]);
            NewZone.ColorLower = NewZone.ColorUpper = GetAverageColor(NewZone.XYWH);
            Data.Cameras[CameraIndex_FT].FeasibleZones.emplace_back(NewZone);
        }

        // choose image from some where
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x * 0.5f - 325.f);
        ImGui::Text("Reference Images Path");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(325.f);
        ImGui::InputText("", ReferenceImagePath, IM_ARRAYSIZE(ReferenceImagePath), ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("...##Btn_FeasibilityRefImgDir"))
        {
            Modals::Get().IsChoosingFolder = true;
            ifd::FileDialog::Instance().Open("ChooseFeasibilityReferenceImagePath", "Choose path to reference images",
                "", false, Setting::Get().ProjectPath);
        }
        // draw control buttons
        if (!ReferenceImages.empty())
        {
            std::string ImgName = ReferenceImages[CurrentRefImgIndex].substr(std::strlen(ReferenceImagePath) + 1);
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 360 - 64) * 0.5f);
            ReferenceImagePos = ImGui::GetCursorScreenPos();
            if (ImGui::Button(ICON_FA_ARROW_LEFT, ImVec2(32, 0)))
            {
                CurrentRefImgIndex++;
                if (CurrentRefImgIndex >= ReferenceImages.size())
                    CurrentRefImgIndex = 0;
                UpdateReferenceImage();
            }
            Utils::AddSimpleTooltip("Previous");
            ImGui::SameLine();
            ImGui::BeginChildFrame(ImGui::GetID("ReferenceImageFileName"), ImVec2(360, ImGui::GetTextLineHeightWithSpacing() + 2));
            float TextWidth = ImGui::CalcTextSize(ImgName.c_str()).x;
            ImGui::SetCursorPosX(180 - TextWidth * 0.5f);
            ImGui::Text("%s", ImgName.c_str());
            ImGui::EndChildFrame();
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_ARROW_RIGHT, ImVec2(32, 0)))
            {
                CurrentRefImgIndex--;
                if (CurrentRefImgIndex < 0)
                    CurrentRefImgIndex = ReferenceImages.size() - 1;
                UpdateReferenceImage();
            }
            Utils::AddSimpleTooltip("Next");
            ImGui::SameLine();
            ImGui::ColorEdit3("##hint", (float*)&HintColor, ImGuiColorEditFlags_NoInputs);
        }

        if (Data.Cameras[CameraIndex_FT].FeasibleZones.empty())
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "No feasibility test zone is registered!");
            return;
        }
        ImGui::Separator();
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x * 0.5f - ImGui::CalcTextSize("PASS").x * 0.5f );
        ImGui::PushFont(Setting::Get().TitleFont);
        if (IsTestZonesPassRefImg())
            ImGui::TextColored(Style::GREEN(400, Setting::Get().Theme) ,"PASS");
        else
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme) ,"FAIL");
        ImGui::PopFont();
        
        static const ImVec2 SmallBtnSize = {32.f, 32.f};
        static const ImVec2 ColorBtnSize = {64.f, 32.f};
        static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable| ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit;
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x * 0.5 - 480);
        if (ImGui::BeginTable("##ColorEditTable", 6, flags))
        {
            ImGui::TableSetupColumn("Zone ID", ImGuiTableColumnFlags_WidthFixed, 128.f);
            ImGui::TableSetupColumn("Color in Ref", ImGuiTableColumnFlags_WidthFixed, 128.f);
            ImGui::TableSetupColumn("Lower Color Limit", ImGuiTableColumnFlags_WidthFixed, 256.f);
            ImGui::TableSetupColumn("Upper Color Limit", ImGuiTableColumnFlags_WidthFixed, 256.f);
            ImGui::TableSetupColumn("Is Pass?", ImGuiTableColumnFlags_WidthFixed, 128.f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 64.f);
            ImGui::TableHeadersRow();
            static ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha |
                ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoDragDrop;
            static ImVec4 BackupColor;
            ImGui::PushID("ColorEdit");
            for (int i = 0; i < Data.Cameras[CameraIndex_FT].FeasibleZones.size(); i++)
            {
                ImGui::PushID(i);
                FFeasibleZone& Zone = Data.Cameras[CameraIndex_FT].FeasibleZones[i]; 
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", i);
                ImGui::TableSetColumnIndex(1);
                ImGui::ColorButton("##Color", GetAverageColor(Zone.XYWH), ImGuiColorEditFlags_None, ColorBtnSize);
                ImGui::TableSetColumnIndex(2);
                if (ImGui::ColorButton("##LowerColor", Zone.ColorLower, ImGuiColorEditFlags_None, ColorBtnSize))
                {
                    ImGui::OpenPopup("LowerColorPicker");
                    BackupColor = Zone.ColorLower;
                }
                if (ImGui::BeginPopup("LowerColorPicker"))
                {
                    ImGui::Text("Set lower color limit");
                    ImGui::Separator();
                    MakeColorPicker(&Zone.ColorLower, BackupColor);
                    ImGui::EndPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_SYNC"##Lower", SmallBtnSize))
                {
                    Zone.ColorLower = GetAverageColor(Zone.XYWH); 
                }
                Utils::AddSimpleTooltip("Sync lower color limit to ref color");
                ImGui::TableSetColumnIndex(3);
                if (ImGui::ColorButton("##UpperColor", Zone.ColorUpper, ImGuiColorEditFlags_None, ColorBtnSize))
                {
                    ImGui::OpenPopup("UpperColorPicker");
                    BackupColor = Zone.ColorUpper;
                }
                if (ImGui::BeginPopup("UpperColorPicker"))
                {
                    ImGui::Text("Set upper color limit");
                    ImGui::Separator();
                    MakeColorPicker(&Zone.ColorUpper, BackupColor);
                    ImGui::EndPopup();
                }
                // ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 8));
                // ImGui::ColorEdit3("##UpperColor", (float*)&Zone.ColorUpper, flags);
                // ImGui::PopStyleVar();
                ImGui::SameLine();
                if(ImGui::Button(ICON_FA_SYNC"##Upper", SmallBtnSize))
                {
                    Zone.ColorUpper = GetAverageColor(Zone.XYWH);
                }
                Utils::AddSimpleTooltip("Sync upper color limit to ref color");
                
                ImGui::TableSetColumnIndex(4);
                if (IsColorInRange(GetAverageColor(Zone.XYWH), Zone.ColorLower, Zone.ColorUpper))
                    ImGui::TextColored(Style::GREEN(400, Setting::Get().Theme), "PASS");
                else
                    ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "FAIL");
                ImGui::TableSetColumnIndex(5);
                if (ImGui::Button(ICON_FA_TIMES, SmallBtnSize))
                {
                    Data.Cameras[CameraIndex_FT].FeasibleZones.erase(Data.Cameras[CameraIndex_FT].FeasibleZones.begin() + i);
                }
                Utils::AddSimpleTooltip("Remove this test zone?");
                ImGui::PopID();
            }
            ImGui::PopID();
            ImGui::EndTable();
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, Style::RED(400, Setting::Get().Theme));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Style::RED(600, Setting::Get().Theme));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Style::RED(700, Setting::Get().Theme));
        if (ImGui::Button(ICON_FA_TRASH, SmallBtnSize))
        {
            Data.Cameras[CameraIndex_FT].FeasibleZones.clear();
        }
        ImGui::PopStyleColor(3);
        Utils::AddSimpleTooltip("Clear all feasibility test zones");
 

    }

    void Deploy::UpdateReferenceImage()
    {
        // imread and update info
        ImgData = cv::imread(ReferenceImages[CurrentRefImgIndex]);

        if (ImgData.empty())
        {
            // Solution to unicode path in opencv: https://stackoverflow.com/a/43369056
            std::wstring wpath = Utils::ConvertUtf8ToWide(ReferenceImages[CurrentRefImgIndex]);
            
            std::ifstream f(wpath, std::iostream::binary);
            // Get its size
            std::filebuf* pbuf = f.rdbuf();
            size_t size = pbuf->pubseekoff(0, f.end, f.in);
            pbuf->pubseekpos(0, f.in);

            // Put it in a vector
            std::vector<uchar> buffer(size);
            pbuf->sgetn((char*)buffer.data(), size);

            // Decode the vector
            ImgData = cv::imdecode(buffer, cv::IMREAD_COLOR);
        }
        cv::cvtColor(ImgData, ImgData, cv::COLOR_BGR2RGB);
        cv::resize(ImgData, ImgData, cv::Size((int)WorkArea.x, (int)WorkArea.y));
        glGenTextures(1, &ReferenceImagePtr);
        glBindTexture(GL_TEXTURE_2D, ReferenceImagePtr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); // Same
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ImgData.cols, ImgData.rows, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, ImgData.data);
    }

    ImColor Deploy::GetAverageColor(const std::array<float, 4>& InXYWH)
    {
        if (ImgData.empty())
            return {0.f ,0.f, 0.f, 1.f};
        const int Width = ImgData.cols;
        const int Height = ImgData.rows;
        const cv::Rect ROI = cv::Rect(
            static_cast<int>((InXYWH[0] - InXYWH[2] * 0.5f) * static_cast<float>(Width)),
            static_cast<int>((InXYWH[1] - InXYWH[3] * 0.5f) * static_cast<float>(Height)),
            static_cast<int>(InXYWH[2] * static_cast<float>(Width)),
            static_cast<int>(InXYWH[3] * static_cast<float>(Height)));
        const cv::Mat ImgRoi = ImgData(ROI);
        cv::Scalar Avg = cv::mean(ImgRoi);
        return ImColor(static_cast<int>(Avg[0]), static_cast<int>(Avg[1]), static_cast<int>(Avg[2]), 255);
    }

    void Deploy::RenderTestZones(ImVec2 StartPos)
    {
        int i = 0;
        for (const FFeasibleZone& Zone : Data.Cameras[CameraIndex_FT].FeasibleZones)
        {
            float X = Zone.XYWH[0];
            float Y = Zone.XYWH[1];
            float W = Zone.XYWH[2];
            float H = Zone.XYWH[3];
            ImVec2 ZoneStartPos = StartPos + (ImVec2(X, Y) - ImVec2(W * 0.5, H * 0.5)) * WorkArea;
            ImVec2 ZoneEndPos = ZoneStartPos + ImVec2(W, H) * WorkArea;
            ImGui::GetWindowDrawList()->AddRectFilled(ZoneStartPos, ZoneEndPos, ImGui::ColorConvertFloat4ToU32(HintColor));
            // render ID
            ImVec2 LabelSize = ImGui::CalcTextSize(std::to_string(i).c_str());
            ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 18.f, (ZoneStartPos + ZoneEndPos - LabelSize)/2,
                ImColor(1.f, 1.f, 1.f, 1.f), std::to_string(i).c_str());
            i++;
        }
    }

    void Deploy::RenderAddingProgress(ImVec2 StartPos)
    {
        AddPointEnd = ImGui::GetMousePos();
        if (AddPointEnd.x > WorkArea.x + ReferenceImagePos.x)
        {
            AddPointEnd.x = WorkArea.x + ReferenceImagePos.x;
        }
        if (AddPointEnd.x < ReferenceImagePos.x)
        {
            AddPointEnd.x = ReferenceImagePos.x;
        }
        if (AddPointEnd.y > WorkArea.y + ReferenceImagePos.y)
        {
            AddPointEnd.y = WorkArea.y + ReferenceImagePos.y;
        }
        if (AddPointEnd.y < ReferenceImagePos.y)
        {
            AddPointEnd.y = ReferenceImagePos.y;
        }
        
        ImGui::GetWindowDrawList()->AddRect(AddPointStart, AddPointEnd, ImColor(1.f, 0.f, 0.f, 1.f), 0, 0, 3);
    }

    void Deploy::MakeColorPicker(ImVec4* TargetColor, ImVec4 BackupColor)
    {
        // ImGui::Text("");
        // ImGui::Separator();
        ImGui::ColorPicker3("##picker", (float*)TargetColor, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
        ImGui::SameLine();

        ImGui::BeginGroup(); // Lock X position
        ImGui::Text("Current");
        ImGui::ColorButton("##current", *TargetColor, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
        ImGui::Text("Previous");
        if (ImGui::ColorButton("##previous", BackupColor, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40)))
            *TargetColor = BackupColor;
        ImGui::EndGroup();
    }

    bool Deploy::IsTestZonesPassRefImg()
    {
        for (const FFeasibleZone& Zone : Data.Cameras[CameraIndex_FT].FeasibleZones)
        {
            if (IsColorInRange(GetAverageColor(Zone.XYWH), Zone.ColorLower, Zone.ColorUpper))
            {
                return true;
            } 
        }
        return false;
    }

    bool Deploy::IsColorInRange(ImVec4 TestColor, ImVec4 LowerColor, ImVec4 UpperColor)
    {
        return TestColor.x >= LowerColor.x && TestColor.x <= UpperColor.x &&
               TestColor.y >= LowerColor.y && TestColor.y <= UpperColor.y &&
               TestColor.z >= LowerColor.z && TestColor.z <= UpperColor.z;
    }

    static float TempFPS = 30.f;
    
    void Deploy::RenderDataPipeline()
    {
        if (ActiveDeployTab != EActiveDeployTab::DataPipeline)
        {
            CleanRegisteredCameras();
        }
        ImGui::BulletText("Time");
        ImGui::Indent();
        ImGui::DragInt("Detection Frequency", &Data.DetectionFrequency, 1, 1, 600, "%d Frames");
        ImGui::Indent();
        ImGui::Text("If your frame rate is ");ImGui::SameLine();
        ImGui::SetNextItemWidth(64.f);
        ImGui::DragFloat("##FPS_Count", &TempFPS, 0.1f, 0.f, 360.f, "%.1f"); ImGui::SameLine();
        ImGui::Text(" , the detection will be performed every %.1f seconds.", static_cast<float>(Data.DetectionFrequency) / TempFPS);
        ImGui::Unindent();
        
        ImGui::Checkbox("Day time only?", &Data.IsDaytimeOnly);
        Utils::AddSimpleTooltip("The dectection will only be performed during day time only? (X AM to Y PM)");
        if (Data.IsDaytimeOnly)
        {
            ImGui::DragInt("Detection Start Time", &Data.DetectionStartTime, 1, 0, 12, "%d AM");
            ImGui::DragInt("Detection End Time", &Data.DetectionEndTime, 1, 0, 12, "%d PM");
        }
        
        // count duration
        ImGui::DragInt("Pass Count Duration", &Data.PassCountDuration, 1, 0, 60, "%d Minutes");
        Utils::AddSimpleTooltip("Will performe per frame detection for X minutes at the start of every hour to estimate pass count.");

        // count feasibility
        ImGui::BulletText("Pass Count Feasibility");
        ImGui::DragFloat("Feasibility Failure Threshold", &Data.PassCountFeasiblityThreshold, 0.01f, 0.f, 1.f, "%.2f");
        Utils::AddSimpleTooltip("If the X/% of detections during the pass count duration failed the feasibility test, the pass count for this hour will be a infeasible.");
        ImGui::Indent();
        const float TotalDetectionsInPassCountDuration = static_cast<float>(Data.PassCountDuration) * 60 * TempFPS / static_cast<float>(Data.DetectionFrequency);
        ImGui::Text("Over %.0f of %.0f infeasible detections will be considered as infeasible pass count.", Data.PassCountFeasiblityThreshold * TotalDetectionsInPassCountDuration, TotalDetectionsInPassCountDuration);
        ImGui::Unindent();
        ImGui::Unindent();

        ImGui::Separator();
        // model parameters
        ImGui::BulletText("Model Parameters");
        if (Data.Cameras.size() == 0)
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "No camera is registered! Checkout autotation tab first!");
            return;
        }

        // for each camera...
        // Choose camera combo
        ImGui::SameLine();
        ImGui::SetNextItemWidth(360.f);
        if (ImGui::BeginCombo("Choose Camera", Data.Cameras[CameraIndex_DP].CameraName.c_str()))
        {
            for (size_t i = 0; i < Data.Cameras.size(); i++)
            {
                if (ImGui::Selectable(Data.Cameras[i].CameraName.c_str(), CameraIndex_DP == i))
                {
                    CameraIndex_DP = i;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Indent();

        // choose model
        static bool bUseExternalModelFile = false;
        static bool bOverrideModelName = false;
        static char SelectedModel[256] = "";

        FCameraSetup* SelectedCamera = &Data.Cameras[CameraIndex_DP];
        ImGui::BulletText("Common Parameters");
        ImGui::Indent();

        // TODO: make the model path correctly set...
        // internal model name
        if (bUseExternalModelFile)
        {
            ImGui::SetNextItemWidth(480.f);
            ImGui::InputText("", &SelectedCamera->ModelFilePath, ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            if (ImGui::Button("...##Btn_ExternModelFile"))
            {
                Modals::Get().IsChoosingFolder = true;
                ifd::FileDialog::Instance().Open("ChooseExternalModelPath_Deploy", "Choose external model file",
                    "Model {.pt}", false, Setting::Get().ProjectPath);
            }
            ImGui::SameLine();
            ImGui::Text("Choose external model file");
        }
        else
        {
            if (SelectedModel[0] == '\0')
            {
                YAML::Node ModelData = YAML::LoadFile(Setting::Get().ProjectPath + "/Models/Models.yaml");
                for (YAML::const_iterator it = ModelData.begin(); it != ModelData.end(); ++it)
                {
                    strcpy(SelectedModel, it->first.as<std::string>().c_str());
                    break;
                }
            }
            if (ImGui::BeginCombo("Choose Model", SelectedModel))
            {
                YAML::Node ModelData = YAML::LoadFile(Setting::Get().ProjectPath + "/Models/Models.yaml");
                for (YAML::const_iterator it = ModelData.begin(); it != ModelData.end(); ++it)
                {
                    auto Name = it->first.as<std::string>();
                    if (ImGui::Selectable(Name.c_str(), Name == SelectedModel))
                    {
                        strcpy(SelectedModel, Name.c_str());
                        SelectedCamera->ModelName = Name;
                        SelectedCamera->ModelFilePath = Setting::Get().ProjectPath + "/Models/" + Name + "/weights/best.pt";
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::Checkbox("Override Model Name?", &bOverrideModelName))
            {
                SelectedCamera->ModelName = SelectedModel;
            }
        }
        if (bUseExternalModelFile || bOverrideModelName)
        {
            ImGui::InputText("Model name", &SelectedCamera->ModelName);
        }
        if (ImGui::Checkbox("Use external model?", &bUseExternalModelFile))
        {
            if (bUseExternalModelFile)
            {
                SelectedCamera->ModelName = "";
                SelectedCamera->ModelFilePath = "";
            }
            else
            {
                SelectedCamera->ModelName = SelectedModel;
                SelectedCamera->ModelFilePath = Setting::Get().ProjectPath + "/Models/" + SelectedModel + "/weights/best.pt";
            }
        }
        
        // image size
        ImGui::DragInt("Image Size (Test)", &SelectedCamera->ImageSize, 32, 128, 1024);
        // confidence threshold
        ImGui::DragFloat("Confidence", &SelectedCamera->Confidence, 0.05f, 0.01f,0.95f, "%.2f");
        // IOU
        ImGui::DragFloat("IOU", &SelectedCamera->IOU, 0.05f, 0.01f,0.95f, "%.2f");
        ImGui::Unindent();

        
        // detection ROI
        ImGui::BulletText("Detection parameters");
        ImGui::Indent();
        ImGui::Checkbox("Set Detection ROI?", &SelectedCamera->ShouldApplyDetectionROI);
        Utils::AddSimpleTooltip("Detection only applied to the region of interest (ROI)?");
        if (SelectedCamera->ShouldApplyDetectionROI)
        {
            ImGui::SliderFloat4("ROI (x1, y1, x2, y2)", SelectedCamera->DetectionROI, 0.f, 1.0f);
        }
        ImGui::Unindent();
        // count direction vertical or horizontal
        ImGui::BulletText("Pass count parameters");
        ImGui::Indent();
        ImGui::Text("Set Fish way start and end:");
        if (ImGui::RadioButton("Vertical", SelectedCamera->IsPassVertical))
        {
            SelectedCamera->IsPassVertical = true;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Horizontal", !SelectedCamera->IsPassVertical))
        {
            SelectedCamera->IsPassVertical = false;
        }
        ImGui::SameLine();
        ImGui::SliderFloat2("Fish way start / end", SelectedCamera->FishwayStartEnd , 0.f, 1.f);
        ImGui::Checkbox("Enable max speed threshould?", &SelectedCamera->ShouldEnableSpeedThreshold);
        ImGui::Checkbox("Enable similar body size threshould?", &SelectedCamera->ShouldEnableBodySizeThreshold);
        if (SelectedCamera->ShouldEnableSpeedThreshold)
        {
            ImGui::DragFloat("Max speed threshould", &SelectedCamera->SpeedThreshold, 0.01f, 0.01f, 0.5f, "%.2f");
        }
        if (SelectedCamera->ShouldEnableBodySizeThreshold)
        {
            ImGui::DragFloat("Body size threshould", &SelectedCamera->BodySizeThreshold, 0.01f, 0.01f, 0.5f, "%.2f");
        }
        ImGui::DragFloat("Confidence threshold", &SelectedCamera->SpeciesDetermineThreshold, 0.01f, 0.01f, 0.5f, "%.2f");
        ImGui::DragInt("Frames buffer threshold", &SelectedCamera->FrameBufferSize, 1, 1, 60);
        ImGui::Unindent();
        ImGui::Unindent();
        // count parameters? how to access?

        ImGui::Separator();
        // server settings
        ImGui::BulletText("Server");
        ImGui::Indent();
        // account
        ImGui::InputText("User account", &Data.ServerAccount);
        Utils::AddSimpleTooltip("The user account of your web application");
        // passwordd
        ImGui::InputText("Password", &Data.ServerPassword, ImGuiInputTextFlags_Password);
        ImGui::Unindent();
        
    }

    void Deploy::RenderSMSNotification()
    {
        // twilio account, api key, phone number
        ImGui::BulletText("Twilio settup");
        ImGui::Indent();
        ImGui::InputText("Twilio account SID", &Data.TwilioSID);
        ImGui::InputText("Twilio Auth Token", &Data.TwilioAuthToken, ImGuiInputTextFlags_Password);
        ImGui::InputText("Twilio phone number", &Data.TwilioPhoneNumber, ImGuiInputTextFlags_CharsDecimal);
        ImGui::Unindent();
        
        // register receivers
        ImGui::BulletText("Register receivers");
        ImGui::Indent();

        ImGui::Text("Name");
        ImGui::SameLine(256, 32);
        ImGui::Text("Phone Numer");
        ImGui::SameLine(512, 32);
        ImGui::Text("Group");
        if (ImGui::BeginTable("##ReceiverRegisterTable", 4, ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit))
        {
            for (int i  = 0; i < Data.SMSReceivers.size(); i++)
            {
                ImGui::PushID(i);
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(256);
                ImGui::InputText("##Receiver name", &Data.SMSReceivers[i].Name);
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(256);
                ImGui::InputText("##Receiver phone number", &Data.SMSReceivers[i].Phone, ImGuiInputTextFlags_CharsDecimal);
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(128);
                if (ImGui::BeginCombo("##group", Data.SMSReceivers[i].Group.c_str()))
                {
                    if (ImGui::Selectable("Manager", Data.SMSReceivers[i].Group == "Manager"))
                        Data.SMSReceivers[i].Group = "Manager";
                    if (ImGui::Selectable("Client", Data.SMSReceivers[i].Group == "Clinet"))
                        Data.SMSReceivers[i].Group = "Client";
                    if (ImGui::Selectable("Helper", Data.SMSReceivers[i].Group == "Helper"))
                        Data.SMSReceivers[i].Group = "Helper";
                    ImGui::EndCombo();
                }
                ImGui::TableNextColumn();
                if (ImGui::Button(ICON_FA_TIMES))
                {
                    Data.SMSReceivers.erase(Data.SMSReceivers.begin() + i);
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        
        if (ImGui::Button(ICON_FA_PLUS"##Btn_AddReceiver"))
        {
            Data.SMSReceivers.emplace_back();
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_TRASH"##Btn_Reset"))
        {
            Data.SMSReceivers.clear();
            Data.SMSReceivers.emplace_back();
        }
        ImGui::Unindent();

        static bool TempBool;
        ImGui::BulletText("Content");
        ImGui::Indent();
        static bool ShouldReportDailySummary = false;
        static bool ShouldReportWeeklySummary = false;
        static bool ShouldReportMonthlySummary = false;
        static bool ShouldReportFirstDayInfeasible = false;
        static bool ShouldReportXDaysInfeasible = false;
        static bool ShouldReportLoseConnetion = false;
        static bool ShouldNotifyScriptStart = false;
        // condition and message and frequency
        if (ImGui::BeginTable("##SMSRegisterTable", 3, ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders))
        {
            ImGui::TableSetupColumn("Message Type", ImGuiTableColumnFlags_WidthFixed, 128.f);
            ImGui::TableSetupColumn("Option", ImGuiTableColumnFlags_WidthFixed, 320.f);
            ImGui::TableSetupColumn("Targeted Group", ImGuiTableColumnFlags_WidthFixed, 320.f);
            ImGui::TableHeadersRow();
            ImGui::TableNextColumn();
            ImGui::Text("Report");
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("Daily Summary", &ShouldReportDailySummary))
            {
                Data.DailyReportSendGroup.GroupManager = false;
                Data.DailyReportSendGroup.GroupClient = false;
                Data.DailyReportSendGroup.GroupHelper = false;
            }
            ImGui::TableNextColumn();
            if (ShouldReportDailySummary)
            {
                ImGui::PushID("DailyGroup");
                ImGui::Checkbox("Manager", &Data.DailyReportSendGroup.GroupManager);ImGui::SameLine();
                ImGui::Checkbox("Client", &Data.DailyReportSendGroup.GroupClient);ImGui::SameLine();
                ImGui::Checkbox("Helper", &Data.DailyReportSendGroup.GroupHelper);
                ImGui::PopID();
            }
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("Weekly Summary", &ShouldReportWeeklySummary))
            {
                Data.WeeklyReportSendGroup.GroupManager = false;
                Data.WeeklyReportSendGroup.GroupClient = false;
                Data.WeeklyReportSendGroup.GroupHelper = false;
            }
            ImGui::TableNextColumn();
            if (ShouldReportWeeklySummary)
            {
                ImGui::PushID("WeeklyGroup");
                ImGui::Checkbox("Manager", &Data.WeeklyReportSendGroup.GroupManager);ImGui::SameLine();
                ImGui::Checkbox("Client",  &Data.WeeklyReportSendGroup.GroupClient);ImGui::SameLine();
                ImGui::Checkbox("Helper",  &Data.WeeklyReportSendGroup.GroupHelper);
                ImGui::PopID();
            }
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("Monthly Summary", &ShouldReportMonthlySummary))
            {
                Data.MonthlyReportSendGroup.GroupManager = false;
                Data.MonthlyReportSendGroup.GroupClient = false;
                Data.MonthlyReportSendGroup.GroupHelper = false;
            }
            ImGui::TableNextColumn();
            if (ShouldReportMonthlySummary)
            {
                ImGui::PushID("MonthlyGroup");
                ImGui::Checkbox("Manager", &Data.MonthlyReportSendGroup.GroupManager);ImGui::SameLine();
                ImGui::Checkbox("Client",  &Data.MonthlyReportSendGroup.GroupClient);ImGui::SameLine();
                ImGui::Checkbox("Helper",  &Data.MonthlyReportSendGroup.GroupHelper);
                ImGui::PopID();
            }
            ImGui::TableNextColumn();
            ImGui::Text("Feasibility");
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("First day", &ShouldReportFirstDayInfeasible))
            {
                Data.InFeasibleFirstDaySendGroup.GroupManager = false;
                Data.InFeasibleFirstDaySendGroup.GroupClient = false;
                Data.InFeasibleFirstDaySendGroup.GroupHelper = false;
            }
            ImGui::TableNextColumn();
            if (ShouldReportFirstDayInfeasible)
            {
                ImGui::PushID("InfeasibleFirstDayGroup");
                ImGui::Checkbox("Manager", &Data.InFeasibleFirstDaySendGroup.GroupManager);ImGui::SameLine();
                ImGui::Checkbox("Client",  &Data.InFeasibleFirstDaySendGroup.GroupClient);ImGui::SameLine();
                ImGui::Checkbox("Helper",  &Data.InFeasibleFirstDaySendGroup.GroupHelper);
                ImGui::PopID();
            }
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("Continuing for", &ShouldReportXDaysInfeasible))
            {
                Data.InFeasibleXDaysSendGroup.GroupManager = false;
                Data.InFeasibleXDaysSendGroup.GroupClient = false;
                Data.InFeasibleXDaysSendGroup.GroupHelper = false;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(48);
            ImGui::DragInt("##ContinuingDays", &Data.InFeasibleTolerate, 1, 2, 14);
            ImGui::SameLine();
            ImGui::Text("days");
            ImGui::TableNextColumn();
            if (ShouldReportXDaysInfeasible)
            {
                ImGui::PushID("InfeasibleXDaysGroup");
                ImGui::Checkbox("Manager", &Data.InFeasibleXDaysSendGroup.GroupManager);ImGui::SameLine();
                ImGui::Checkbox("Client",  &Data.InFeasibleXDaysSendGroup.GroupClient);ImGui::SameLine();
                ImGui::Checkbox("Helper",  &Data.InFeasibleXDaysSendGroup.GroupHelper);
                ImGui::PopID();
            }
            ImGui::TableNextColumn();
            ImGui::Text("Server");
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("Lose connection", &ShouldReportLoseConnetion))
            {
                Data.LoseConnectionSendGroup.GroupManager = false;
                Data.LoseConnectionSendGroup.GroupClient = false;
                Data.LoseConnectionSendGroup.GroupHelper = false;
            }
            ImGui::TableNextColumn();
            if (ShouldReportLoseConnetion)
            {
                ImGui::PushID("LoseConnectionGroup");
                ImGui::Checkbox("Manager", &Data.LoseConnectionSendGroup.GroupManager);ImGui::SameLine();
                ImGui::Checkbox("Client",  &Data.LoseConnectionSendGroup.GroupClient);ImGui::SameLine();
                ImGui::Checkbox("Helper",  &Data.LoseConnectionSendGroup.GroupHelper);
                ImGui::PopID();
            }
            ImGui::TableNextColumn();
            ImGui::Text("Test");
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("Test SMS is working", &ShouldNotifyScriptStart))
            {
                Data.SMSTestSendGroup.GroupManager = false;
                Data.SMSTestSendGroup.GroupClient = false;
                Data.SMSTestSendGroup.GroupHelper = false;
            }
            ImGui::TableNextColumn();
            if (ShouldNotifyScriptStart)
            {
                ImGui::PushID("TestGroup");
                ImGui::Checkbox("Manager", &Data.SMSTestSendGroup.GroupManager);ImGui::SameLine();
                ImGui::Checkbox("Client",  &Data.SMSTestSendGroup.GroupClient);ImGui::SameLine();
                ImGui::Checkbox("Helper",  &Data.SMSTestSendGroup.GroupHelper);
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::Unindent();
    }


}
