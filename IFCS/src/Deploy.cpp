#include "Deploy.h"

#include <fstream>

#include "opencv2/opencv.hpp"
#include "backends/imgui_impl_glfw.h"
#include <GLFW/glfw3.h>

#include "misc/cpp/imgui_stdlib.h"

#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include "addons/imguidatechooser.h"
#include "ImFileDialog/ImFileDialog.h"

#include "DataBrowser.h"
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
    
    enum class EColorEditSpace : uint8_t
    {
        R,
        G,
        B,
        H,
        S,
        V
    };

    static EActiveDeployTab ActiveDeployTab = EActiveDeployTab::Automation;

    void Deploy::SetInputPath(const char* NewPath)
    {
        strcpy(InputPath, NewPath);
    }

    void Deploy::SetOutputPath(const char* NewPath)
    {
        strcpy(OutputPath, NewPath);
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
                    // TODO: check utf8 issue...
                    ReferenceImages.emplace_back(entry.path().string());
                }
            }
        }
        if (!ReferenceImages.empty())
        {
            UpdateReferenceImage();
        }
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
            // open dialog and serialize to file
        }
        Utils::AddSimpleTooltip("Save current configuration to file");
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_DOWNLOAD))
        {
            // open dialog and deserialize from file
        }
        Utils::AddSimpleTooltip("Load configuration from file");
        ImGui::SameLine();
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 300);
        if (ImGui::Button("generate script", ImVec2(270, 0)))
        {
            GenerateScript();
        }
    }
    

    static int TaskModeOption = 0;
    static int TaskRunTime[2];
    static int DatesOption = 0;
    static tm StartDate;
    static tm EndDate;
    static bool bBackupImportantRegion = true;
    static bool bBackupCombinedClips = true;
    static bool bDeleteRawClips = false;
    
    void Deploy::RenderAutomation()
    {
        // TODO: maybe replace file dialog by native file dialoge to prevent trailing slash issues?
        ImGui::BulletText("Choose input and output paths for the automation task");
        ImGui::Indent();

        // choose target clips repo (input)
        ImGui::SetNextItemWidth(480.f);
        ImGui::InputText("", InputPath, IM_ARRAYSIZE(InputPath), ImGuiInputTextFlags_ReadOnly);
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
        ImGui::InputText("", OutputPath, IM_ARRAYSIZE(OutputPath), ImGuiInputTextFlags_ReadOnly);
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

        /*
         * TODO: maybe I need two different camera names, the name in DVR and the name in a more human readable format
         */
        // register cameras
        ImGui::BulletText("Register cameras");
        ImGui::Indent();
        
        ImGui::PushID("CameraName");
        for (size_t i = 0; i < SelectedCameras.size(); i++)
        {
            ImGui::PushID(i);
            ImGui::SetNextItemWidth(240.f);
            ImGui::InputText("", &SelectedCameras[i]);
            ImGui::PopID();
        }
        ImGui::PopID();
        
        if (ImGui::Button(ICON_FA_PLUS))
        {
            if (SelectedCameras.size() < 6)
            {
                SelectedCameras.emplace_back("");
            }
            
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_MINUS))
        {
            if (SelectedCameras.size() > 1)
            {
                SelectedCameras.pop_back();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_SYNC))
        {
            SelectedCameras = {"", ""};
        }
        ImGui::Unindent();

        // choose task mode
        /*
         * 1) run it for particular dates right now
         * 2) run it for yesterday everyday
         */
        ImGui::BulletText("Task mode");
        ImGui::Indent();
        ImGui::RadioButton("Schedule", &TaskModeOption, 0); ImGui::SameLine();
        ImGui::RadioButton("Now", &TaskModeOption, 1);
        if (TaskModeOption == 0)
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Run at given time everyday for yesterday's clips");
        }
        else
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme),"Run right now for given dates");
        }
        
        if (TaskModeOption==0)
        {
            // mode 1
            // choose start time

            ImGui::SetNextItemWidth(240.f);
            if (ImGui::DragInt2("Scheduled Runtime (hour : minute)", TaskRunTime, 1, 0, 59))
            {
                if (TaskRunTime[0] > 23)
                {
                    TaskRunTime[0] = 23;
                }
                if (TaskRunTime[1] > 59)
                {
                    TaskRunTime[1] = 59;
                }
            }
        }
        else
        {
            // mode 2
            // choose date range or mutiple dates
            ImGui::RadioButton("Choose interval", &DatesOption, 0); ImGui::SameLine();
            ImGui::RadioButton("Pick dates", &DatesOption, 1);
            if (DatesOption == 0)
            {
                ImGui::SetNextItemWidth(360.f);
                if (ImGui::DateChooser("Start Date", StartDate, "%Y/%m/%d"))
                {
                    if (std::difftime(std::mktime(&StartDate), std::mktime(&EndDate)) > 0)
                    {
                        EndDate = StartDate;
                    }
                }
                ImGui::SetNextItemWidth(360.f);
                if (ImGui::DateChooser("End Date", EndDate, "%Y/%m/%d"))
                {
                    if (std::difftime(std::mktime(&StartDate), std::mktime(&EndDate)) > 0)
                    {
                        EndDate = StartDate;
                    }
                }
            }
            else
            {
                ImGui::PushID("DatesChooser");
                for (size_t i = 0; i < SelectedDates.size(); i++)
                {
                    ImGui::PushID(i);
                    ImGui::SetNextItemWidth(240.f);
                    ImGui::DateChooser("", SelectedDates[i], "%Y/%m/%d");
                    ImGui::PopID();
                }
                ImGui::PopID();

                ImGui::PushID("DatesControl");
                if (ImGui::Button(ICON_FA_PLUS))
                {
                    SelectedDates.emplace_back(tm());
                }
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_MINUS))
                {
                    if (SelectedDates.size() == 1) return;
                    SelectedDates.pop_back();
                }
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_SYNC))
                {
                    SelectedDates.clear();
                    SelectedDates.emplace_back(tm());
                }
                ImGui::PopID();
            }
        }
        ImGui::Unindent();
        
        // how to handle these clips when done?
        ImGui::BulletText("Post processing");
        ImGui::Indent();
        // enable important region backup
        ImGui::Checkbox("Backup clips with important regions", &bBackupImportantRegion);
        // enable combined clips backup
        ImGui::Checkbox("Backup combinded clip", &bBackupCombinedClips);
        // delete raw clips after backup?
        ImGui::Checkbox("Delete raw clips after backup?", &bDeleteRawClips);
        ImGui::Unindent();
    }

    static int SelectedCameraIndex = 0;
    static ImVec2 ReferenceImagePos;
    static bool bAdding;
    static ImVec2 AddPointStart;
    static ImVec2 AddPointEnd;

    static EColorEditSpace ColorEditSpace;

    void Deploy::RenderFeasibilityTest()
    {
        // setup for which camera?
        if (ActiveDeployTab != EActiveDeployTab::FeasibilityTest)
        {
            std::sort( SelectedCameras.begin(), SelectedCameras.end() );
            SelectedCameras.erase( unique( SelectedCameras.begin(), SelectedCameras.end() ), SelectedCameras.end() );
            // sync selected cameras to feasible zones
            for (auto& camera : SelectedCameras)
            {
                if (camera.empty()) continue;
                if (!FeasibleZones.count(camera))
                {
                    FeasibleZones[camera] = {};
                }
            }
            for (auto& [k, v] : FeasibleZones)
            {
                if (!Utils::Contains(SelectedCameras, k))
                {
                    FeasibleZones.erase(k);
                }
            }
        }

        if (FeasibleZones.size() == 0)
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "No camera is registered! Checkout autotation tab");
            return;
        }

        // Choose camera combo
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x * 0.5 - 180.f);
        ImGui::SetNextItemWidth(360.f);
        if (ImGui::BeginCombo("Choose Camera", SelectedCameras[SelectedCameraIndex].c_str()))
        {
            for (size_t i = 0; i < SelectedCameras.size(); i++)
            {
                bool bSelected = (SelectedCameraIndex == i);
                if (ImGui::Selectable(SelectedCameras[i].c_str(), bSelected))
                {
                    SelectedCameraIndex = i;
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
            spdlog::info("is adding new zone with xywh: {}, {}, {}, {}  ... correct?", NewZone.XYWH[0], NewZone.XYWH[1], NewZone.XYWH[2], NewZone.XYWH[3]);
            NewZone.ColorLower = NewZone.ColorUpper = GetAverageColor(NewZone.XYWH);
            FeasibleZones[SelectedCameras[SelectedCameraIndex]].emplace_back(NewZone);
        }

        // choose image from some where
        ImGui::Text("Reference Images Path");
        ImGui::SetNextItemWidth(325.f);
        ImGui::InputText("", ReferenceImagePath, IM_ARRAYSIZE(ReferenceImagePath), ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("...##Btn_FeasibilityRefImgDir"))
        {
            Modals::Get().IsChoosingFolder = true;
            ifd::FileDialog::Instance().Open("ChooseFeasibilityReferenceImagePath", "Choose path to reference images",
                "", false, Setting::Get().ProjectPath);
        }
        ImGui::SameLine();
        // draw control buttons
        if (!ReferenceImages.empty())
        {
            ImGui::SameLine();
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
        }

        if (FeasibleZones[SelectedCameras[SelectedCameraIndex]].empty())
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "No feasibility test zone is registered!");
            return;
        }
        ImGui::Separator();
        ImGui::PushFont(Setting::Get().TitleFont);
        if (ImGui::Button(ICON_FA_TIMES, ImVec2(64, 0)))
        {
            FeasibleZones[SelectedCameras[SelectedCameraIndex]].clear();
        }
        ImGui::PopFont();
        Utils::AddSimpleTooltip("Clear all feasibility test zones");
        ImGui::SameLine(0, 360.f);
        ImGui::Text("Color Edit Mode:");ImGui::SameLine();
        
        // for every draw box, set its color feasible range
        if (ImGui::Button("R")) ColorEditSpace = EColorEditSpace::R;
        ImGui::SameLine();
        if (ImGui::Button("G")) ColorEditSpace = EColorEditSpace::G;
        ImGui::SameLine();
        if (ImGui::Button("B")) ColorEditSpace = EColorEditSpace::B;
        ImGui::SameLine(0, 30);
        if (ImGui::Button("H")) ColorEditSpace = EColorEditSpace::H;
        ImGui::SameLine();
        if (ImGui::Button("S")) ColorEditSpace = EColorEditSpace::S;
        ImGui::SameLine();
        if (ImGui::Button("V")) ColorEditSpace = EColorEditSpace::V;
        ImGui::SameLine(0, 200);

        ImGui::Text("PASS");
        // static ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
        //     ImGuiTableFlags_BordersV | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        // if (ImGui::BeginTable("##ColorEditTable", 4, flags))
        // {
        //     ImGui::TableSetupColumn("Zone", ImGuiTableColumnFlags_WidthFixed, 100.f);
        //     ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 100.f);
        //     ImGui::TableSetupColumn("Lower", ImGuiTableColumnFlags_WidthFixed, 100.f);
        //     ImGui::TableSetupColumn("Upper", ImGuiTableColumnFlags_WidthFixed, 100.f);
        //     ImGui::TableHeadersRow();
        //
        //     for (int i = 0; i < FeasibleZones[SelectedCameras[SelectedCameraIndex]].size(); i++)
        //     {
        //         ImGui::TableNextRow();
        //         ImGui::TableSetColumnIndex(0);
        //         ImGui::Text("%d", i);
        //         ImGui::TableSetColumnIndex(1);
        //         ImGui::ColorButton("##Color", ImVec4(FeasibleZones[SelectedCameras[SelectedCameraIndex]][i].ColorLower[0] / 255.f,
        //             FeasibleZones[SelectedCameras[SelectedCameraIndex]][i].ColorLower[1] / 255.f,
        //             FeasibleZones[SelectedCameras[SelectedCameraIndex]][i].ColorLower[2] / 255.f, 1.f));
        //         ImGui::TableSetColumnIndex(2);
        //         ImGui::PushItemWidth(80.f);
        //         ImGui::DragScalarN("##Lower", ImGuiDataType_U8, FeasibleZones[SelectedCameras[SelectedCameraIndex]][i].ColorLower.data(), 3, 1.f, 0, 255);
        //         ImGui::PopItemWidth();
        //         ImGui::TableSetColumnIndex(3);
        //         ImGui::PushItemWidth(80.f);
        //         ImGui::DragScalarN("##Upper", ImGuiDataType_U8, FeasibleZones[SelectedCameras[SelectedCameraIndex]][i].ColorUpper.data(), 3, 1.f, 0, 255);
        //         ImGui::PopItemWidth();
        //     }
        //     ImGui::EndTable();
        // }
 

        //color picker?


    }

    void Deploy::UpdateReferenceImage()
    {
        // imread and update info
        cv::Mat Img = cv::imread(ReferenceImages[CurrentRefImgIndex]);

        if (Img.empty())
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
            Img = cv::imdecode(buffer, cv::IMREAD_COLOR);
        }
        cv::cvtColor(Img, Img, cv::COLOR_BGR2RGB);
        cv::resize(Img, Img, cv::Size((int)WorkArea.x, (int)WorkArea.y));
        glGenTextures(1, &ReferenceImagePtr);
        glBindTexture(GL_TEXTURE_2D, ReferenceImagePtr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); // Same
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Img.cols, Img.rows, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, Img.data);
    }

    ImVec4 Deploy::GetAverageColor(const std::array<float, 4>& InXYWH)
    {
        return {1.f ,1.f, 1.f, 1.f};
    }

    void Deploy::RenderTestZones(ImVec2 StartPos)
    {
        for (const FFeasibleZone& Zone : FeasibleZones[SelectedCameras[SelectedCameraIndex]])
        {
            float X = Zone.XYWH[0];
            float Y = Zone.XYWH[1];
            float W = Zone.XYWH[2];
            float H = Zone.XYWH[3];
            ImVec2 ZoneStartPos = StartPos + (ImVec2(X, Y) - ImVec2(W * 0.5, H * 0.5)) * WorkArea;
            ImVec2 ZoneEndPos = ZoneStartPos + ImVec2(W, H) * WorkArea;
            ImGui::GetWindowDrawList()->AddRectFilled(ZoneStartPos, ZoneEndPos, ImColor(1.f, 0.f, 0.f, 1.f));
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

    void Deploy::RenderDataPipeline()
    {
        // detection start time

        // detection end time

        // count duration

        // count feasibility? how to deside?

        // generic model parameters for detection

        // count parameters? how to access?

        // server settings
        // account

        // password
    }

    void Deploy::RenderSMSNotification()
    {
        // twilio account, api key, phone number
        
        // register receivers

        // condition and message and frequency
    }

    void Deploy::GenerateScript()
    {
    }
}
