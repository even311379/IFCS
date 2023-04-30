#include "Deploy.h"

#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include "misc/cpp/imgui_stdlib.h"

#include "Modals.h"
#include "Setting.h"
#include "Style.h"
#include "addons/imguidatechooser.h"
#include "ImFileDialog/ImFileDialog.h"


namespace IFCS
{
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
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Feasibility Test"))
                {
                    RenderFeasibilityTest();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Data pipeline"))
                {
                    RenderDataPipeline();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Feasibility Test"))
                {
                    RenderFeasibilityTest();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("SMS Notification"))
                {
                    RenderSMSNotification();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::EndChild();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 300);
        if (ImGui::Button("generate configuration", ImVec2(270, 0)))
        {
            GenerateConfiguration();
        }
    }
    
    static char OutputPath[256];
    static char InputPath[256];
    static char Camera1[64], Camera2[64], Camera3[64], Camera4[64], Camera5[64], Camera6[64];
    static uint8_t NumOfCameras = 2;
    static int TaskModeOption = 0;
    static int TaskRunTime[2];
    static int DatesOption = 0;
    static tm PickedDate1, PickedDate2, PickedDate3, PickedDate4, PickedDate5, PickedDate6, PickedDate7,
        PickedDate8, PickedDate9, PickedDate10, PickedDate11, PickedDate12, PickedDate13, PickedDate14;
    static uint8_t NumDates = 5;
    static tm StartDate;
    static tm EndDate;
    static bool bBackupImportantRegion = true;
    static bool bBackupCombinedClips = true;
    static bool bDeleteRawClips = false;
    
#define POPULATE_CAMERA_INPUT(Which) \
    if (NumOfCameras > Which - 1)\
    {\
        ImGui::SetNextItemWidth(240.f);\
        ImGui::InputText("Camera "#Which, Camera##Which, IM_ARRAYSIZE(Camera##Which));\
        \
    }
#define POPULATE_PICKED_DATE_INPUT(Which) \
    if (NumDates > Which - 1)\
    {\
        ImGui::SetNextItemWidth(240.f);\
        ImGui::DateChooser("", PickedDate##Which);\
        \
    }
    
    void Deploy::RenderAutomation()
    {
        // TODO: maybe replace file dialog by native file dialoge to prevent trailing slash issues?
        ImGui::BulletText("Choose input and output paths for the automation task");
        ImGui::Indent();
        // choose store repo
        ImGui::SetNextItemWidth(480.f);
        ImGui::InputText("", OutputPath, IM_ARRAYSIZE(OutputPath), ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("..."))
        {
            Modals::Get().IsChoosingFolder = true;
            ifd::FileDialog::Instance().Open("ChooseExternalModelPath", "Choose external model path",
                "Model {.pt}", false, Setting::Get().ProjectPath);
        }
        ImGui::SameLine();
        ImGui::Text("Output Path");

        // choose target clips repo
        ImGui::SetNextItemWidth(480.f);
        ImGui::InputText("", InputPath, IM_ARRAYSIZE(InputPath), ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("..."))
        {
            Modals::Get().IsChoosingFolder = true;
            ifd::FileDialog::Instance().Open("ChooseExternalModelPath", "Choose external model path",
                "Model {.pt}", false, Setting::Get().ProjectPath);
        }
        ImGui::SameLine();
        ImGui::Text("Output Text");
        ImGui::Unindent();
        // register cameras
        ImGui::BulletText("Register cameras");
        ImGui::Indent();
        
        // ImGui::SetNextItemWidth(240.f);
        // ImGui::InputText("Camera 1", Camera1, sizeof(char)*64);
        // POPULATE_CAMERA_INPUT(2)
        // POPULATE_CAMERA_INPUT(3)
        // POPULATE_CAMERA_INPUT(4)
        // POPULATE_CAMERA_INPUT(5)
        // POPULATE_CAMERA_INPUT(6)
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
            // if (NumOfCameras >= 6)
            // {
            //     return;
            // }
            // NumOfCameras++;
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
            // NumOfCameras=2;
            // Camera1[0] = '\0';
            // Camera2[0] = '\0';
            // Camera3[0] = '\0';
            // Camera4[0] = '\0';
            // Camera5[0] = '\0';
            // Camera6[0] = '\0';
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
                // TODO: force start date to be earlier than end date??
                ImGui::SetNextItemWidth(360.f);
                ImGui::DateChooser("Start Date", StartDate, "%Y/%m/%d");
                ImGui::SetNextItemWidth(360.f);
                ImGui::DateChooser("End Date", EndDate, "%Y/%m/%d");
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

    void Deploy::RenderFeasibilityTest()
    {
        // choose image from some where

        // image which and draw boxes

        // for every draw box, set its color feasible range
        
    }

    void Deploy::RenderDataPipeline()
    {
    }

    void Deploy::RenderSMSNotification()
    {
    }

    void Deploy::GenerateConfiguration()
    {
    }
}
