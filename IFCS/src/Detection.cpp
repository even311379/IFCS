#include "Detection.h"

#include <charconv>

#include "DataBrowser.h"
#include "Setting.h"
#include "Style.h"

#include "ImguiNotify/font_awesome_5.h"
#include "Implot/implot.h"
#include "imgui_stdlib.h"

#include <fstream>
#include "yaml-cpp/yaml.h"
#include "opencv2/opencv.hpp"
#include "imgui_impl_glfw.h"
#include <GLFW/glfw3.h>

namespace IFCS
{
    void Detection::RenderContent()
    {
        if (ImGui::TreeNodeEx("Make Detection", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const float AvailWidth = ImGui::GetContentRegionAvail().x;
            // ImGui::SetNextItemWidth(240.f);
            if (ImGui::BeginCombo("Choose Model", SelectedModel))
            {
                YAML::Node ModelData = YAML::LoadFile(Setting::Get().ProjectPath + "/Models/Models.yaml");
                for (size_t i = 0; i < ModelData.size(); i++)
                {
                    std::string Name = ModelData[i]["Name"].as<std::string>();
                    // ImGui::Selectable(Name.c_str(), false);
                    if (ImGui::Selectable(Name.c_str(), Name == SelectedModel))
                    {
                        strcpy(SelectedModel, Name.c_str());
                        UpdateDetectionScript();
                    }
                }
                ImGui::EndCombo();
            }
            // ImGui::SetNextItemWidth(240.f);ImGui::SameLine();
            if (ImGui::BeginCombo("Choose Clip", SelectedClip))
            {
                for (const std::string& Clip : DataBrowser::Get().GetAllClips())
                {
                    std::string ClipNoPath = Clip.substr(Setting::Get().ProjectPath.size() + 7);
                    if (ImGui::Selectable(ClipNoPath.c_str(), ClipNoPath == SelectedClip))
                    {
                        strcpy(SelectedClip, ClipNoPath.c_str());
                        UpdateDetectionScript();
                    }
                }
                ImGui::EndCombo();
            }
            // ImGui::SetNextItemWidth(240.f);ImGui::SameLine();

            if (ImGui::InputInt("ImageSize", &ImageSize, 32, 128))
            {
                if (ImageSize < 64) ImageSize = 64;
                if (ImageSize > 1280) ImageSize = 1280;
                UpdateDetectionScript();
            }
            if (ImGui::InputFloat("Confidence", &Confidence, 0.01f, 0.1f, "%.2f"))
            {
                if (Confidence < 0.01f) Confidence = 0.01f;
                if (Confidence > 0.95f) Confidence = 0.95f;
                UpdateDetectionScript();
            }
            if (ImGui::InputFloat("IOU", &IOU, 0.01f, 0.1f, "%.2f"))
            {
                if (IOU < 0.01f) IOU = 0.01f;
                if (IOU > 0.95f) IOU = 0.95f;
                UpdateDetectionScript();
            }
            if (ImGui::InputText("Prediction name", &DetectionName))
            {
                UpdateDetectionScript();
                YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Detections/Detections.yaml");
                IsDetectionNameUsed = false;
                for (size_t i = 0; i < Data.size(); i++)
                {
                    if (Data[i]["Name"].as<std::string>() == DetectionName)
                    {
                        IsDetectionNameUsed = true;
                        break;
                    }
                }
            }
            if (IsDetectionNameUsed)
                ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "s% has already is use... Try another name!",
                                   DetectionName.c_str());
            if (ReadyToDetect())
            {
                ImGui::Text("About to run:");
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30);
                if (ImGui::Button(ICON_FA_COPY))
                {
                    ImGui::SetClipboardText((SetPathScript + "\n" + DetectScript).c_str());
                }
                ImGui::BeginChildFrame(ImGui::GetID("DetectScript"), ImVec2(0, ImGui::GetTextLineHeight() * 4));
                ImGui::TextWrapped((SetPathScript + "\n" + DetectScript).c_str());
                ImGui::EndChildFrame();

                if (ImGui::Button("Detect", ImVec2(0, 0)))
                {
                    MakeDetection();
                }
            }
            ImGui::Text("Detection Log");
            ImGui::BeginChildFrame(ImGui::GetID("DetectLog"), ImVec2(AvailWidth, ImGui::GetTextLineHeight() * 6));
            ImGui::TextWrapped("%s", DetectionLog.c_str());
            ImGui::EndChildFrame();
            ImGui::TreePop();
        }

        if (IsDetecting)
        {
            if (DetectFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
            {
                DetectionLog += Utils::GetCurrentTimeString(true) + " Done!\n";
                DetectionName = ""; // force prevent same name
                IsDetecting = false;
            }
        }

        if (ImGui::TreeNodeEx("Analysis", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SetNextItemWidth(240.f);
            if (ImGui::BeginCombo("Choose Detection", SelectedDetection))
            {
                YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Detections/Detections.yaml");
                for (size_t i = 0; i < Data.size(); i++)
                {
                    std::string Name = Data[i]["Name"].as<std::string>();
                    if (ImGui::Selectable(Name.c_str(), Name == SelectedDetection))
                    {
                        strcpy(SelectedDetection, Name.c_str());
                        DetectionClip = Setting::Get().ProjectPath + "/Clips/" + Data[i]["TargetClip"].as<
                            std::string>();
                        UpdateFrame(1, true);
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            static const char* AnalysisType[] = {"Pass", "In Screen"};
            ImGui::SetNextItemWidth(240.f);
            ImGui::Combo("Choose Display Type", &SelectedAnalysisType, AnalysisType, IM_ARRAYSIZE(AnalysisType));
            // if not select... no need to render other stuff
            if (strlen(SelectedDetection) == 0)
            {
                ImGui::TreePop();
                return;
            }
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 1280) * 0.5f);
            ImGui::Image((void*)(intptr_t)LoadedFramePtr, ImVec2(1280, 720));
            static char* PlayIcon;
            if (CurrentFrame == EndFrame)
                PlayIcon = ICON_FA_SYNC;
            else if (IsPlaying)
                PlayIcon = ICON_FA_PAUSE;
            else
                PlayIcon = ICON_FA_PLAY;
            if (ImGui::Button(PlayIcon, {96, 32}))
            {
                if (CurrentFrame == EndFrame)
                    CurrentFrame = 0;
                if (!IsPlaying)
                {
                    IsPlaying = true;
                    JustPlayed = true;
                }
                else
                {
                    JustPaused = true;
                }
            }
            ImGui::SameLine();
            static ImVec2 NewFramePad(4, (32 - ImGui::GetFontSize()) / 2);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, NewFramePad);
            ImGui::SetNextItemWidth(120.f);
            if (ImGui::DragInt("##PlayStart", &StartFrame, 1, 1, EndFrame - 1))
            {
                if (CurrentFrame < StartFrame) CurrentFrame = StartFrame;
            }
            ImGui::PopStyleVar();
            ImGui::SameLine();
            DrawPlayRange();
            ImGui::SameLine();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, NewFramePad);
            ImGui::SetNextItemWidth(120.f);
            if (ImGui::DragInt("##PlayEnd", &EndFrame, 1, StartFrame + 1, TotalClipFrameSize))
            {
                if (CurrentFrame > EndFrame) CurrentFrame = EndFrame;
            }
            ImGui::PopStyleVar();
            // TODO: add export video?
            if (SelectedAnalysisType == 0)
            {
                RenderAnaylysisWidgets_Pass();
            }
            else
            {
                RenderAnaylysisWidgets_InScreen();
            }

            ImGui::TreePop();
        }


        ProcessingVideoPlay();
        // if (IsPlaying)
        // {
        //     CurrentFrame++;
        //     if (CurrentFrame >= EndFrame)
        //     {
        //         CurrentFrame = EndFrame;
        //         IsPlaying = false;
        //     }
        // }
    }

    bool Detection::ReadyToDetect()
    {
        return (strlen(SelectedModel) != 0 && strlen(SelectedClip) != 0 && !DetectionName.empty() && !
            IsDetectionNameUsed);
    }

    void Detection::UpdateDetectionScript()
    {
        std::string ProjectPath = Setting::Get().ProjectPath;
        std::string SelectedModelName = SelectedModel;
        SetPathScript = "cd " + Setting::Get().YoloV7Path;
        DetectScript = "";
        DetectScript += Setting::Get().PythonPath + "/python detect.py";
        DetectScript += " --weights " + ProjectPath + "/Models/" + SelectedModelName + "/weights/best.pt";
        DetectScript += " --conf-thres " + std::to_string(Confidence).substr(0, 4);
        DetectScript += " --iou-thres " + std::to_string(IOU).substr(0, 4);
        DetectScript += " --img-size " + std::to_string(ImageSize);
        DetectScript += " --project " + ProjectPath + "/Detections";
        DetectScript += " --name " + DetectionName;
        DetectScript += " --source " + ProjectPath + "/Clips/" + SelectedClip;
        DetectScript += " --device 0 --save-txt --nosave";
    }

    void Detection::MakeDetection()
    {
        if (IsDetecting) return;
        DetectionLog = Utils::GetCurrentTimeString(true) + "Start detection ... Check progress in console\n";

        auto func = [=]()
        {
            system(SetPathScript.c_str());
            std::ofstream ofs;
            ofs.open("Detect.bat");
            ofs << SetPathScript << " &^\n" << DetectScript;
            ofs.close();
            system("Detect.bat");
        };

        IsDetecting = true;
        DetectFuture = std::async(std::launch::async, func);
    }


    void Detection::UpdateFrame(int FrameNumber, bool UpdateClipInfo)
    {
        cv::Mat frame;
        {
            cv::VideoCapture cap(DetectionClip);
            if (UpdateClipInfo)
            {
                TotalClipFrameSize = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
                EndFrame = TotalClipFrameSize;
                ClipFPS = (float)cap.get(cv::CAP_PROP_FPS);
            }
            cap.set(cv::CAP_PROP_POS_FRAMES, FrameNumber);
            cap >> frame;
            // cv::resize(frame, frame, cv::Size(1280, 720)); // 16 : 9
            cap.release();
        }
        FrameToGL(frame);
    }

    // TODO: performance so bad!!, cpu usage goes to 5X%, because of its a 4k video?
    // Create temp video with reduced resolution to save it?
    void Detection::ProcessingVideoPlay()
    {
        if (!IsPlaying) return;
        // check if receive stop play

        // open cap
        static cv::VideoCapture Cap;
        if (JustPlayed)
        {
            Cap.open(DetectionClip);

            Cap.set(cv::CAP_PROP_POS_FRAMES, CurrentFrame);
            JustPlayed = false;
        }
        if (JustPaused)
        {
            Cap.release();
            JustPaused = false;
            IsPlaying = false;
            return;
        }

        // check should wait for update frame
        static float TimePassed = 0.f;
        TimePassed += (1.0f / ImGui::GetIO().Framerate);
        if (TimePassed < (1 / ClipFPS)) return;
        TimePassed = 0.f;
        // update frame
        static cv::Mat frame;
        Cap.set(cv::CAP_PROP_POS_FRAMES, CurrentFrame);
        
        Cap >> frame;
        FrameToGL(frame);
        CurrentFrame += 1;
        //check should release cap and stop play
        if (CurrentFrame == EndFrame)
        {
            IsPlaying = false;
            Cap.release();
        }
    }

    void Detection::FrameToGL(cv::Mat Data)
    {
        // Resize will can save gpu utilization
        cv::resize(Data, Data, cv::Size(1280, 720));
        cv::cvtColor(Data, Data, cv::COLOR_BGR2RGB); // test just rgb?

        // TODO: draw detection box!

        // TODO: draw n_pass or n_on_screen
        
        glGenTextures(1, &LoadedFramePtr);
        glBindTexture(GL_TEXTURE_2D, LoadedFramePtr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); // Same
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Data.cols, Data.rows, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, Data.data);
    }

    void Detection::DrawPlayRange()
    {
        ImVec2 CurrentPos = ImGui::GetCursorScreenPos();
        ImU32 Color = ImGui::ColorConvertFloat4ToU32(Style::BLUE(600, Setting::Get().Theme));
        ImU32 PlayColor = ImGui::ColorConvertFloat4ToU32(Style::YELLOW(400, Setting::Get().Theme));
        const float Width = ImGui::GetContentRegionAvail().x - 130;
        ImVec2 RectStart = CurrentPos + ImVec2(5, 22);
        ImVec2 RectEnd = RectStart + ImVec2(Width, 5);
        ImGui::GetWindowDrawList()->AddRectFilled(RectStart, RectEnd, Color, 20.f);
        ImVec2 RectPlayStart = RectStart + ImVec2(0, -10);
        ImVec2 RectPlayEnd = RectStart + ImVec2(float(CurrentFrame - StartFrame) / float(EndFrame - StartFrame) * Width,
                                                10);
        ImGui::GetWindowDrawList()->AddRectFilled(RectPlayStart, RectPlayEnd, PlayColor);
        // draw current frame text?
        if (IsPlaying && RectPlayEnd.x > RectPlayStart.x + 30)
        {
            char FrameTxt[8];
            sprintf(FrameTxt, "%d", CurrentFrame);
            ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 16, RectPlayEnd + ImVec2(strlen(FrameTxt)*-16.f, -18.f), Color, FrameTxt );
        }
        ImGui::SetCursorScreenPos(RectPlayStart);
        ImGui::InvisibleButton("PlayControl", ImVec2(Width, 15));
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (ImGui::IsItemClicked(0))
        {
            CurrentFrame = int((ImGui::GetMousePos().x - RectStart.x) / Width * (float)(EndFrame - StartFrame)) +
                StartFrame;
            if (CurrentFrame < StartFrame) CurrentFrame = StartFrame;
            if (IsPlaying)
                JustPaused = true;
            else
                UpdateFrame(CurrentFrame);
        }
    }

    void Detection::RenderAnaylysisWidgets_Pass()
    {
        // specify line
        ImGui::Text("Set Fish way start and end:");
        ImGui::SetNextItemWidth(240.f);
        ImGui::DragFloat2("Start P1", FishWayStartP1, 0.01f, 0, 1);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(240.f);
        ImGui::DragFloat2("Start P2", FishWayStartP2, 0.01f, 0, 1);
        ImGui::SetNextItemWidth(240.f);
        ImGui::DragFloat2("End P1", FishWayEndP1, 0.01f, 0, 1);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(240.f);
        ImGui::DragFloat2("End P2", FishWayEndP2, 0.01f, 0, 1);

        static ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
        if (ImGui::BeginTable("##PassSummation", 3, flags, ImVec2(-1, 0)))
        {
            ImGui::TableSetupColumn("Group", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("Current", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("##Graphs");
            ImGui::TableHeadersRow();
            ImPlot::PushColormap(ImPlotColormap_Plasma);
            if (IsPlaying)
                PlayOffset = (PlayOffset + 1) % TotalClipFrameSize; // speed control? ... need to slow down... 
            for (size_t row = 0; row < Results.size(); row++)
            {
                const char* id = Results[row].CategoryDisplayName.c_str();
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text(id);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f", Results[row].MeanNums[CurrnetPlayPosition]);
                ImGui::TableSetColumnIndex(2);
                ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
                if (ImPlot::BeginPlot(id, ImVec2(-1, 35), ImPlotFlags_CanvasOnly | ImPlotFlags_NoChild))
                {
                    ImPlot::SetupAxes(0, 0, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                    ImPlot::SetupAxesLimits(0, MaxDisplayNums, 0, 6, ImGuiCond_Always);
                    ImPlot::SetNextLineStyle(Results[row].Color);
                    ImPlot::SetNextFillStyle(Results[row].Color, 0.25);
                    ImPlot::PlotLine(id, Results[row].MeanNums.data(), 100, 1, 0, ImPlotLineFlags_Shaded,
                                     PlayOffset);
                    ImPlot::EndPlot();
                }
                ImPlot::PopStyleVar();
            }
            ImGui::EndTable();
        }

        if (ImGui::BeginTable("##IndividualData", 3, flags, ImVec2(-1, 0)))
        {
            ImGui::TableSetupColumn("Group", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("Total Sum", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("Accumulated sum");
            ImGui::TableHeadersRow();
            // if (IsPlaying)
            //     PlayOffset = (PlayOffset + 1) % TotalClipFrameSize; // speed control? ... need to slow down... 
            // for (size_t row = 0; row < Results.size(); row++)
            // {
            //     const char* id = Results[row].CategoryDisplayName.c_str();
            //     ImGui::TableNextRow();
            //     ImGui::TableSetColumnIndex(0);
            //     ImGui::Text(id);
            //     ImGui::TableSetColumnIndex(1);
            //     ImGui::Text("%d", FakeSum[row]);
            //     ImGui::TableSetColumnIndex(2);
            //     char buf[32];
            //     sprintf(buf, "%d", FakeSum[row]);
            //     ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Results[row].Color);
            //     ImGui::ProgressBar(FakeProportion[row], ImVec2(-1, 0), buf);
            //     ImGui::PopStyleColor();
            // }
            ImGui::EndTable();
        }
    }

    void Detection::RenderAnaylysisWidgets_InScreen()
    {
        ImGui::SetNextItemWidth(240.f);
        // ImGui::DragInt("Per Unit Frame Size", &PUFS, 1, 1, TotalClipFrameSize);
        static ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;

        if (ImGui::BeginTable("##InScreenSize", 3, flags, ImVec2(-1, 0)))
        {
            ImGui::TableSetupColumn("Group", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("Current", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("##Graphs");
            ImGui::TableHeadersRow();
            ImPlot::PushColormap(ImPlotColormap_Plasma);
            if (IsPlaying)
                PlayOffset = (PlayOffset + 1) % TotalClipFrameSize; // speed control? ... need to slow down... 
            for (size_t row = 0; row < Results.size(); row++)
            {
                const char* id = Results[row].CategoryDisplayName.c_str();
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text(id);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f", Results[row].MeanNums[CurrnetPlayPosition]);
                ImGui::TableSetColumnIndex(2);
                ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
                if (ImPlot::BeginPlot(id, ImVec2(-1, 35), ImPlotFlags_CanvasOnly | ImPlotFlags_NoChild))
                {
                    ImPlot::SetupAxes(0, 0, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                    ImPlot::SetupAxesLimits(0, MaxDisplayNums, 0, 6, ImGuiCond_Always);
                    ImPlot::SetNextLineStyle(Results[row].Color);
                    ImPlot::SetNextFillStyle(Results[row].Color, 0.25);
                    ImPlot::PlotLine(id, Results[row].MeanNums.data(), 100, 1, 0, ImPlotLineFlags_Shaded,
                                     PlayOffset);
                    ImPlot::EndPlot();
                }
                ImPlot::PopStyleVar();
            }
            ImGui::EndTable();
        }
    }

    // implement analysis of predictions
    void Detection::Analysis()
    {
        // create fake data for now...
        for (int i = 0; i < 5; i++)
        {
            FAnalysisResult Fake;
            char buff[32];
            snprintf(buff, sizeof(buff), "Fake_%d", i);
            Fake.CategoryDisplayName = buff;
            Fake.Color = Utils::RandomPickColor();
            for (int j = 0; j < TotalClipFrameSize; j++)
            {
                Fake.MeanNums.push_back((float)Utils::RandomIntInRange(0, 5));
                Fake.PassNums.push_back(Utils::RandomIntInRange(0, 10));
            }
            Results.push_back(Fake);
        }
    }

    // create fake data....
    void Detection::Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose)
    {
        Panel::Setup(InName, InShouldOpen, InFlags, InCanClose);
        Analysis();
    }
}
