#include "Detection.h"

#include <charconv>

#include "DataBrowser.h"
#include "Setting.h"
#include "Style.h"

#include "ImguiNotify/font_awesome_5.h"
#include "Implot/implot.h"
#include "imgui_stdlib.h"

#include <fstream>
#include <variant>

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
                YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Models/Models.yaml");
                for (YAML::const_iterator it = Data.begin(); it != Data.end(); ++it)
                {
                    auto Name = it->first.as<std::string>();
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
                for (YAML::const_iterator it = Data.begin(); it != Data.end(); ++it)
                {
                    if (it->first.as<std::string>() == DetectionName)
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
                // append to project data
                FDetectionDescription Desc;
                Desc.Name = DetectionName;
                Desc.Categories = YAML::LoadFile(Setting::Get().ProjectPath + "/Models/Models.yaml")[
                    std::string(SelectedModel)]["Categories"].as<std::vector<std::string>>();
                Desc.CreationTime = Utils::GetCurrentTimeString();
                Desc.SourceModel = SelectedModel;
                Desc.TargetClip = Setting::Get().ProjectPath + "/Clips/" + SelectedClip;
                Desc.Confidence = Confidence;
                Desc.IOU = IOU;
                Desc.ImageSize = ImageSize;
                YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Detections/Detections.yaml");
                Data[DetectionName] = Desc.Serialize();
                YAML::Emitter Out;
                Out << Data;
                std::ofstream ofs(Setting::Get().ProjectPath + "/Detections/Detections.yaml");
                ofs << Out.c_str();
                ofs.close();
                IsDetecting = false;
                DetectionName = ""; // force prevent same name
            }
        }

        if (ImGui::TreeNodeEx("Analysis", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SetNextItemWidth(240.f);
            if (ImGui::BeginCombo("Choose Detection", SelectedDetection))
            {
                YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Detections/Detections.yaml");
                for (YAML::const_iterator it = Data.begin(); it != Data.end(); ++it)
                {
                    std::string Name = it->first.as<std::string>();
                    if (ImGui::Selectable(Name.c_str(), Name == SelectedDetection))
                    {
                        strcpy(SelectedDetection, Name.c_str());
                        DetectionClip = it->second["TargetClip"].as<std::string>();
                        LoadedLabels.clear();
                        std::string Path = Setting::Get().ProjectPath + "/Detections/" + Name + "/labels";
                        for (const auto& Entry : std::filesystem::directory_iterator(Path))
                        {
                            std::string TxtName = Entry.path().filename().u8string();
                            size_t FrameDigits = TxtName.find('.') - TxtName.find('_') - 1;
                            std::string Temp = TxtName.substr(TxtName.find('_') + 1, FrameDigits);
                            int FrameCount = std::stoi(Temp);
                            std::ifstream TxtFile(Entry.path().u8string());
                            std::string Line;
                            std::vector<FLabelData> Labels;
                            while (std::getline(TxtFile, Line))
                            {
                                std::stringstream SS(Line);
                                std::string Item;
                                std::vector<std::string> D;
                                while (std::getline(SS, Item, ' '))
                                {
                                    D.push_back(Item);
                                }
                                Labels.push_back(FLabelData(
                                    std::stoi(D[0]),
                                    std::stof(D[1]),
                                    std::stof(D[2]),
                                    std::stof(D[3]),
                                    std::stof(D[4]),
                                    std::stof(D[5])
                                ));
                            }
                            spdlog::info("label with size {} add to loaded labels ", Labels.size());
                            LoadedLabels[FrameCount] = Labels;
                        }
                        Categories.clear();
                        for (const auto& C : it->second["Categories"].as<std::vector<std::string>>())
                        {
                            Categories.push_back({C, Utils::RandomPickColor(Setting::Get().Theme)});
                        }
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
            // category colors...
            // No need for this!!
            int N = 0;
            for (const auto& [k, v] : Categories)
            {
                ImGui::ColorEdit3(k.c_str(), (float*)&v, ImGuiColorEditFlags_NoInputs);
                N++;
                if (N < Categories.size() && N % 6 == 0)
                {
                    ImGui::SameLine(120.f * N);
                }
            }
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 1280) * 0.5f);
            ImVec2 StartPos = ImGui::GetCursorScreenPos();
            ImGui::Image((void*)(intptr_t)LoadedFramePtr, ImVec2(1280, 720));
            RenderDetectionBox(StartPos);
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
        DetectScript += " --device 0 --save-txt --save-conf";

        ////////////////////////////////////////////////
        std::string AppPath = std::filesystem::current_path().u8string();
        SaveToProjectScript = Setting::Get().ProjectPath + "/python " + AppPath +
            "/Scripts/SaveDetectionDataToProject.py";
        // TODO: add lots of arguments and write that python script...
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


    void Detection::RenderDetectionBox(ImVec2 StartPos)
    {
        // ImU32 Color = ImGui::ColorConvertFloat4ToU32(Style::RED());
        // ImGui::GetWindowDrawList()->AddRect(StartPos, StartPos + ImVec2{100, 100}, Color);
        if (LoadedLabels.count(CurrentFrame))
        {
            for (const FLabelData& L : LoadedLabels[CurrentFrame])
            {
                ImVec2 P1 = StartPos;
                ImVec2 P2 = StartPos;
                P1.x += (L.X - L.Width * 0.5f) * WorkArea.x;
                P1.y += (L.Y - L.Height * 0.5f) * WorkArea.y;
                P2.x += (L.X + L.Width * 0.5f) * WorkArea.x;
                P2.y += (L.Y + L.Height * 0.5f) * WorkArea.y;

                ImU32 Color = ImGui::ColorConvertFloat4ToU32(Categories[L.CatID].CatColor);
                ImGui::GetWindowDrawList()->AddRect(P1, P2, Color, 0, 0, 2);
                char buff[32];
                sprintf(buff, "%s %.2f", Categories[L.CatID].CatName.c_str(), L.Conf);
                ImVec2 TxtSize = ImGui::CalcTextSize(buff);
                ImGui::GetWindowDrawList()->AddRectFilled(P1 + ImVec2(0, -TxtSize.y), P1 + ImVec2(TxtSize.x, 0), Color);
                ImGui::GetWindowDrawList()->AddText(P1 + ImVec2(0, -TxtSize.y),
                                                    ImGui::ColorConvertFloat4ToU32({1, 1, 1, 1}), buff);
            }
        }
        // also render fish way hint
        if (SelectedAnalysisType == 0) // "Pass"
        {
            ImVec2 FSP1 = StartPos;
            ImVec2 FEP1 = StartPos;
            ImVec2 FSP2, FEP2;
            ImU32 Col = ImGui::ColorConvertFloat4ToU32(FishWayHintColor);
            float W = 1280.f;
            float H = 720.f;
            if (IsVertical)
            {
                FSP1.x += FishWayPos[0] * W;
                FSP2 = FSP1;
                FSP2.y += H;
                FEP1.x += FishWayPos[1] * W;
                FEP2 = FEP1;
                FEP2.y += H;
                ImGui::GetWindowDrawList()->AddText(FSP1 + ImVec2(0, -ImGui::GetFontSize()), Col, "Start");
                ImGui::GetWindowDrawList()->AddText(FEP1 + ImVec2(0, -ImGui::GetFontSize()), Col, "End");
            }
            else
            {
                FSP1.y +=  FishWayPos[0] * H;
                FSP2 = FSP1;
                FSP2.x += W;
                FEP1.y += FishWayPos[1] * H;
                FEP2 = FEP1;
                FEP2.x += W;
                ImGui::GetWindowDrawList()->AddText(FSP2, Col, "Start");
                ImGui::GetWindowDrawList()->AddText(FEP2, Col, "End");
            }
            // spdlog::info("start:{}  end:{} ", FishWayPos[0], FishWayPos[1]);
            // spdlog::info("{} {}, {} {} ", FSP1.x, FSP1.y, FSP2.x, FSP2.y);
            ImGui::GetWindowDrawList()->AddLine(FSP1, FSP2, Col, 3);
            ImGui::GetWindowDrawList()->AddLine(FEP1, FEP2, Col, 3);
        }
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
        FrameToGL(frame, FrameNumber);
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
        FrameToGL(frame, CurrentFrame);
        CurrentFrame += 1;
        //check should release cap and stop play
        if (CurrentFrame == EndFrame)
        {
            IsPlaying = false;
            Cap.release();
        }
    }

    void Detection::FrameToGL(cv::Mat Data, int FrameCount)
    {
        // Resize will can save gpu utilization
        cv::resize(Data, Data, cv::Size((int)WorkArea.x, (int)WorkArea.y));
        cv::cvtColor(Data, Data, cv::COLOR_BGR2RGB); // test just rgb?

        // TODO: nothing is drawn!!! so bad...

        // TODO: draw detection box!
        // Position was Just wrong in opencv // final trial in imgui drawlist...
        /*if (LoadedLabels.count(FrameCount))
        {
            for (const FLabelData& L : LoadedLabels[FrameCount])
            {
                cv::Point P1, P2;
                P1.x = int((L.X - L.Width * 0.5f) * WorkArea.x);
                P1.y = int((L.Y - L.Height * 0.5f) * WorkArea.y);
                P2.x = int((L.X + L.Width * 0.5f) * WorkArea.x);
                P2.y = int((L.Y + L.Height * 0.5f) * WorkArea.y);

                ImVec4 ImColor = Categories[L.CatID].CatColor;
                cv::Scalar Color = CV_RGB(int(ImColor.x * 255),int(ImColor.y * 255), int(ImColor.z * 255));
                cv::rectangle(Data, P1, P2, Color, 2);
                cv::putText(Data, Categories[L.CatID].CatName, P1 + cv::Point{0, -12}, cv::FONT_HERSHEY_PLAIN, 1.0, Color);
                // cv::addText(Data, "Fish", P1 + cv::Point(0, -12), cv::FONT_HERSHEY_DUPLEX);
            }
        }*/

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
        if (RectPlayEnd.x > RectPlayStart.x + 30)
        {
            char FrameTxt[8];
            sprintf(FrameTxt, "%d", CurrentFrame);
            ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 16,
                                                RectPlayEnd + ImVec2(strlen(FrameTxt) * -16.f, -18.f), Color, FrameTxt);
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
        if (ImGui::RadioButton("Vertical", IsVertical))
        {
            IsVertical = true;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Horizontal", !IsVertical))
        {
            IsVertical = false;
        }
        ImGui::SameLine();
        ImGui::SliderFloat2("Fish way start / end", FishWayPos, 0.f, 1.f);
        ImGui::SameLine();
        ImGui::ColorEdit3("##hint", (float*)&FishWayHintColor, ImGuiColorEditFlags_NoInputs);

        // ImGui::SetNextItemWidth(240.f);
        // ImGui::DragFloat2("Start P1", FishWayStartP1, 0.01f, 0, 1);
        // ImGui::SameLine();
        // ImGui::SetNextItemWidth(240.f);
        // ImGui::DragFloat2("Start P2", FishWayStartP2, 0.01f, 0, 1);
        // ImGui::SetNextItemWidth(240.f);
        // ImGui::DragFloat2("End P1", FishWayEndP1, 0.01f, 0, 1);
        // ImGui::SameLine();
        // ImGui::SetNextItemWidth(240.f);
        // ImGui::DragFloat2("End P2", FishWayEndP2, 0.01f, 0, 1);

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
