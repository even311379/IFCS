#include "Detection.h"

#include "DataBrowser.h"
#include "Setting.h"
#include "Style.h"
#include "Modals.h"

#include "ImguiNotify/font_awesome_5.h"
#include "Implot/implot.h"
#include "imgui_stdlib.h"
#include "Imspinner/imspinner.h"

#include <fstream>
#include <variant>

#include "yaml-cpp/yaml.h"
#include "opencv2/opencv.hpp"
#include <GLFW/glfw3.h>


namespace IFCS
{
    void Detection::Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose)
    {
        Panel::Setup(InName, InShouldOpen, InFlags, InCanClose);
    }

    // TODO: this is just a cool feature? not sure whether it is 100% required...
    void Detection::ClearCacheIndividuals()
    {
    }

    void Detection::RenderContent()
    {
        if (!Setting::Get().IsEnvSet())
        {
            ImGui::Text("Environment not setup yet!");
            if (ImGui::Button("Open setting to set it now?"))
            {
                Modals::Get().IsModalOpen_Setting = true;
            }
            return;
        }
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
                ImGui::BeginChildFrame(ImGui::GetID("DetectScript"), ImVec2(0, ImGui::GetTextLineHeight() * 6));
                ImGui::TextWrapped((SetPathScript + "\n" + DetectScript).c_str());
                ImGui::EndChildFrame();

                if (ImGui::Button("Detect", ImVec2(0, 0)))
                {
                    MakeDetection();
                }
            }
            ImGui::Text("Detection Log");
            ImGui::BeginChildFrame(ImGui::GetID("DetectLog"), ImVec2(AvailWidth, ImGui::GetTextLineHeight() * 4));
            ImGui::TextWrapped("%s", DetectionLog.c_str());
            ImGui::EndChildFrame();
            ImGui::TreePop();
        }

        if (IsDetecting)
        {
            ImGui::Text("Wait for detection...");
            ImSpinner::SpinnerBarsScaleMiddle("Spinner1", 6, ImColor(Style::BLUE(400, Setting::Get().Theme)));
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
            return;
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
                        Analysis(Name, it->second);
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            static const char* AnalysisType[] = {"Pass", "In Screen"};
            ImGui::SetNextItemWidth(240.f);
            ImGui::Combo("Choose Display Type", &SelectedAnalysisType, AnalysisType, IM_ARRAYSIZE(AnalysisType));
            if (IsAnalyzing)
            {
                ImGui::Text("Wait for analyzing...");
                ImSpinner::SpinnerBarsScaleMiddle("Spinner2", 6, ImColor(Style::BLUE(400, Setting::Get().Theme)));
                if (AnalyzeFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
                {
                    CurrentFrame = 0;
                    UpdateFrame(CurrentFrame, true);
                    UpdateRoiScreenData();
                    IsAnalyzing = false;
                }
            }
            ImGui::SameLine();
            ImGui::Checkbox("Display helper lines?", &DisplayHelperLines);
            // if not select... no need to render other stuff
            if (strlen(SelectedDetection) == 0 || IsAnalyzing)
            {
                ImGui::TreePop();
                return;
            }
            // category colors...
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
            if (ImGui::Button(PlayIcon, {120, 32}))
            {
                if (CurrentFrame == EndFrame)
                    CurrentFrame = StartFrame;
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
            if (ImGui::DragInt("##PlayStart", &StartFrame, 1, 0, EndFrame - 1))
            {
                if (CurrentFrame < StartFrame) CurrentFrame = StartFrame;
                if (StartFrame > EndFrame) StartFrame = EndFrame - 1;
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
                if (EndFrame < StartFrame) EndFrame = StartFrame + 1;
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
        ImU32 HintCol = ImGui::ColorConvertFloat4ToU32(HintColor);
        if (SelectedAnalysisType == 0 && DisplayHelperLines) // "Pass"
        {
            ImVec2 FSP1 = StartPos;
            ImVec2 FEP1 = StartPos;
            ImVec2 FSP2, FEP2;
            float W = WorkArea.x;
            float H = WorkArea.y;
            if (IsVertical)
            {
                FSP1.y += FishWayPos[0] * H;
                FSP2 = FSP1;
                FSP2.x += W;
                FEP1.y += FishWayPos[1] * H;
                FEP2 = FEP1;
                FEP2.x += W;
                ImGui::GetWindowDrawList()->AddText(FSP2, HintCol, "Start");
                ImGui::GetWindowDrawList()->AddText(FEP2, HintCol, "End");
            }
            else
            {
                FSP1.x += FishWayPos[0] * W;
                FSP2 = FSP1;
                FSP2.y += H;
                FEP1.x += FishWayPos[1] * W;
                FEP2 = FEP1;
                FEP2.y += H;
                ImGui::GetWindowDrawList()->AddText(FSP1 + ImVec2(0, -ImGui::GetFontSize()), HintCol, "Start");
                ImGui::GetWindowDrawList()->AddText(FEP1 + ImVec2(0, -ImGui::GetFontSize()), HintCol, "End");
            }
            ImGui::GetWindowDrawList()->AddLine(FSP1, FSP2, HintCol, 3);
            ImGui::GetWindowDrawList()->AddLine(FEP1, FEP2, HintCol, 3);
        }
        if (SelectedAnalysisType == 1 && DisplayHelperLines && bSetROI)
        {
            ImVec2 Pos1(StartPos.x + RoiRegion[0] * WorkArea.x, StartPos.y + RoiRegion[1] * WorkArea.y);
            ImVec2 Pos2(StartPos.x + RoiRegion[2] * WorkArea.x, StartPos.y + RoiRegion[3] * WorkArea.y);
            ImGui::GetWindowDrawList()->AddRect(Pos1, Pos2, HintCol);
            ImGui::GetWindowDrawList()->AddCircleFilled(Pos1, 8, HintCol);
            ImGui::GetWindowDrawList()->AddCircleFilled(Pos2, 8, HintCol);
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
                ClipWidth = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
                FIndividualData::Width = ClipWidth;
                ClipHeight = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
                FIndividualData::Height = ClipHeight;
            }
            cap.set(cv::CAP_PROP_POS_FRAMES, FrameNumber);
            cap >> frame;
            // cv::resize(frame, frame, cv::Size(1280, 720)); // 16 : 9
            cap.release();
        }
        UpdateFrameImpl(frame);
    }

    // TODO: performance so bad!!, cpu usage goes to 5X%, because of its a 4k video? ...
    // CUDA acceleration may help... with GPU version opencv...
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
        UpdateFrameImpl(frame);
        CurrentFrame += 1;
        //check should release cap and stop play
        if (CurrentFrame == EndFrame)
        {
            IsPlaying = false;
            Cap.release();
        }
    }

    void Detection::UpdateFrameImpl(cv::Mat Data)
    {
        UpdateCurrentCatCount();
        UpdateAccumulatedPasses();
        // Resize will can save gpu utilization
        cv::resize(Data, Data, cv::Size((int)WorkArea.x, (int)WorkArea.y));
        cv::cvtColor(Data, Data, cv::COLOR_BGR2RGB); // test just rgb?

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
        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            const int FrameAtPosition = int(
                (ImGui::GetMousePos().x - RectStart.x) / Width * (float)(EndFrame - StartFrame)) + StartFrame;
            ImGui::SetTooltip("%d", FrameAtPosition);
        }
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
        if (!IsIndividualDataLatest)
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Individual tracking data is not latest...");
        // specify line
        ImGui::Text("Set Fish way start and end:");
        if (ImGui::RadioButton("Vertical", IsVertical))
        {
            IsVertical = true;
            IsIndividualDataLatest = false;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Horizontal", !IsVertical))
        {
            IsVertical = false;
            IsIndividualDataLatest = false;
        }
        ImGui::SameLine();
        if (ImGui::SliderFloat2("Fish way start / end", FishWayPos, 0.f, 1.f))
        {
            IsIndividualDataLatest = false;
        }
        ImGui::SameLine();
        ImGui::ColorEdit3("##hint", (float*)&HintColor, ImGuiColorEditFlags_NoInputs);
        if (!IsIndividualDataLatest)
        {
            if (ImGui::Button("Update individual tracking"))
            {
                TrackIndividual();
            }
        }

        if (IndividualData.empty()) return;

        static ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;

        if (ImGui::BeginTable("##Pass", 3, flags, ImVec2(-1, 0)))
        {
            ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("Total Pass", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            char buf[64];
            sprintf(buf, "Accumulated Passes (Frame %d to %d)", StartFrame, CurrentFrame);
            ImGui::TableSetupColumn(buf);
            ImGui::TableHeadersRow();
            for (const auto& [Cat, Pass] : CurrentPass)
            {
                // TODO: choose color this way will crash!   use static color for now...
                // ImVec4 CatColor = std::find_if(std::begin(Categories), std::end(Categories), [=](const FCategoryData& C)
                // {
                //     return C.CatName == Cat;
                // })->CatColor;
                const char* id = Cat.c_str();
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text(id);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%d", Pass);
                ImGui::TableSetColumnIndex(2);
                char buf2[32];
                sprintf(buf2, "%d", Pass);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Style::BLUE());
                ImGui::ProgressBar((float)Pass / (float)GetMaxPass(), ImVec2(-1, 0), buf2);
                ImGui::PopStyleColor();
            }
            ImGui::EndTable();
        }

        flags |= ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable |
            ImGuiTableFlags_SortMulti;
        ImGuiTableColumnFlags CF = ImGuiTableColumnFlags_DefaultSort;
        if (ImGui::BeginTable("##Individual", 7, flags, ImVec2(-1, ImGui::GetTextLineHeightWithSpacing() * 20)))
        {
            ImGui::TableSetupColumn("Category", CF, 0.f, IndividualColumnID_Category);
            ImGui::TableSetupColumn("Is Passed", CF, 0.f, IndividualColumnID_IsPassed);
            ImGui::TableSetupColumn("ApproxSpeed (pixel^2 / frame)", CF, 0.f, IndividualColumnID_ApproxSpeed);
            ImGui::TableSetupColumn("ApproxBodySize (pixel^2)", CF, 0.f, IndividualColumnID_ApproxBodySize);
            ImGui::TableSetupColumn("Enter Frame", CF, 0.f, IndividualColumnID_EnterFrame);
            ImGui::TableSetupColumn("Leave Frame", CF, 0.f, IndividualColumnID_LeaveFrame);
            ImGui::TableSetupColumn("##view", ImGuiTableColumnFlags_NoSort, 0.f);
            ImGui::TableHeadersRow();

            if (ImGuiTableSortSpecs* SortSpec = ImGui::TableGetSortSpecs())
            {
                if (SortSpec->SpecsDirty)
                {
                    FIndividualData::CurrentSortSepcs = SortSpec;
                    if (IndividualData.size() > 1)
                        qsort(&IndividualData[0], IndividualData.size(), sizeof(IndividualData[0]),
                              FIndividualData::CompareWithSortSpecs);
                    FIndividualData::CurrentSortSepcs = NULL;
                    SortSpec->SpecsDirty = false;
                }
            }

            // use clipper?
            ImGuiListClipper Clipper;
            Clipper.Begin((int)IndividualData.size());
            while (Clipper.Step())
                for (int RowN = Clipper.DisplayStart; RowN < Clipper.DisplayEnd; RowN++)
                {
                    FIndividualData* D = &IndividualData[RowN];
                    ImGui::PushID(RowN);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text(D->GetName().c_str());
                    ImGui::TableNextColumn();
                    // TODO: replace with badge widget?
                    if (D->IsCompleted)
                        ImGui::TextColored(Style::GREEN(400, Setting::Get().Theme), "Pass");
                    else
                        ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Unknown");
                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", D->GetApproxSpeed());
                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", D->GetApproxBodySize());
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", D->GetEnterFrame());
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", D->GetLeaveFrame());
                    ImGui::TableNextColumn();
                    ImGui::Button("View");
                    Utils::AddSimpleTooltip(
                        "[Upcoming feature] View image and frame where this individual come from...");
                    ImGui::PopID();
                }
            ImGui::EndTable();
        }
    }

    // TODO: simple is better... so I'll just use body size only to check individual?
    bool Detection::IsSizeSimilar(const FLabelData& Label1, const FLabelData& Label2)
    {
        // expose these vars to users?
        // the same individual should have same size between frames, but but to camera angle and it could be slightly different
        static float BodySizeBuffer = 0.2f;

        float S1 = Label1.GetApproxBodySize(ClipWidth, ClipHeight);
        float S2 = Label2.GetApproxBodySize(ClipWidth, ClipHeight);

        return (S1 < S2 * (1 + BodySizeBuffer) && S1 > S2 * (1 - BodySizeBuffer)) ||
            (S2 < S1 * (1 + BodySizeBuffer) && S2 > S1 * (1 - BodySizeBuffer));
    }

    // TODO: this algorithm is still wrong!! 40% until its good?
    void Detection::TrackIndividual()
    {
        // spdlog::info("Maybe loaded labels are wrong? size: {} (was moved?)", LoadedLabels.size());
        // for (auto [F, LL] : LoadedLabels)
        // {
        //     for (auto L : LL)
        //     {
        //         spdlog::info("Frame:{}, X:{}, Y:{}", F, L.X, L.Y);
        //     }
        // }

        float MinPos = std::min(FishWayPos[0], FishWayPos[1]);
        float MaxPos = std::max(FishWayPos[0], FishWayPos[1]);

        // if two labels are very far... they might be different individual...
        static float MaxPerFrameSpeed;
        MaxPerFrameSpeed = 0.2f * Utils::Distance(0, 0, (float)FIndividualData::Width, (float)FIndividualData::Height);

        // use cached individual images? use another ai individual check algorithm?
        static int DisappearTime = 10;

        IndividualData.clear();
        std::vector<FIndividualData> TempTrackData;
        for (const auto& [F, LL] : LoadedLabels)
        {
            for (const FLabelData& L : LL)
            {
                // check if inside
                bool IsInFishway;
                bool IsLeaving;
                if (IsVertical)
                {
                    IsInFishway = (L.Y >= MinPos) && (L.Y <= MaxPos);
                    IsLeaving = FishWayPos[0] > FishWayPos[1] ? L.Y < FishWayPos[1] : L.Y > FishWayPos[1];
                }
                else
                {
                    IsInFishway = (L.X >= MinPos) && (L.X <= MaxPos);
                    IsLeaving = FishWayPos[0] > FishWayPos[1] ? L.X < FishWayPos[1] : L.X > FishWayPos[1];
                }

                // TODO: lots of miss count... when a lots of fish comes together...

                // use Just enter??
                
                // check has already tracked any
                if (IsInFishway)
                {
                    if (TempTrackData.empty())
                    {
                        TempTrackData.emplace_back(F, L);
                    }
                    else
                    {
                        // check should add new individual or add info
                        for (auto& Data : TempTrackData)
                        {
                            auto it = Data.Info.end();
                            --it;
                            // block body size as condition
                            // if (IsSizeSimilar(it->second, L) && (it->second.Distance(L, ClipWidth, ClipHeight) / (float)
                            //     (F - it->first) < MaxPerFrameSpeed))
                            if (it->second.Distance(L, ClipWidth, ClipHeight) / (float)(F - it->first) <
                                MaxPerFrameSpeed)
                            {
                                Data.AddInfo(F, L);
                                break; // TODO: add break here is 100% wrong!!!!
                            }
                        }
                    }
                }
                // check if this one is JUST leave the area...
                else if (IsLeaving)
                {
                    for (auto& Data : TempTrackData)
                    {
                        auto it = Data.Info.end();
                        --it;
                        // if (IsSizeSimilar(it->second, L) && (it->second.Distance(L, ClipWidth, ClipHeight) / (float)
                        //     (F - it->first) < MaxPerFrameSpeed))
                        if (it->second.Distance(L, ClipWidth, ClipHeight) / (float)(F - it->first) < MaxPerFrameSpeed)
                        {
                            Data.IsCompleted = true;
                            break;
                        }
                    }
                }
            } // end of per frame

            // check which existing one should append

            for (const auto& Data : TempTrackData)
            {
                // check is completed or should untrack
                if (std::prev(Data.Info.end())->first + DisappearTime < F || Data.IsCompleted)
                {
                    IndividualData.push_back(Data);
                }
            }
            TempTrackData.erase(
                std::remove_if(
                    TempTrackData.begin(),
                    TempTrackData.end(),
                    [=](const FIndividualData& Data)
                    {
                        bool R = (std::prev(Data.Info.end())->first + DisappearTime < F || Data.IsCompleted);
                        if (R)
                            spdlog::info("Was removed? {}, {}", std::prev(Data.Info.end())->first + DisappearTime, F);
                        return (std::prev(Data.Info.end())->first + DisappearTime < F || Data.IsCompleted);
                    }),
                TempTrackData.end());
        }

        // get total pass
        TotalPass.clear();
        for (const auto& Data : IndividualData)
        {
            if (!Data.IsCompleted) continue;
            std::string Cat = Data.GetName();
            if (TotalPass.count(Cat))
            {
                TotalPass[Cat] += 1;
            }
            else
            {
                TotalPass[Cat] = 0;
            }
        }

        IsIndividualDataLatest = true;
    }

    // TODO: this is just a cool feature? not sure whether it is 100% required...
    void Detection::GenerateCachedIndividualImages()
    {
    }

    void Detection::UpdateAccumulatedPasses()
    {
        CurrentPass.clear();
        std::vector<std::string> CatNames;
        for (const auto& Cat : Categories)
        {
            CatNames.push_back(Cat.CatName);
            CurrentCatCount[Cat.CatName] = 0;
        }
        for (const auto& Data : IndividualData)
        {
            if (!Data.IsCompleted) continue;
            const int LeaveFrame = Data.GetLeaveFrame();
            if (LeaveFrame <= CurrentFrame && LeaveFrame >= StartFrame)
            {
                CurrentPass[Data.GetName()] += 1;
            }
        }
    }

    int Detection::GetMaxPass()
    {
        int MaxPass = 0;
        for (const auto& [Cat, Pass] : TotalPass)
        {
            if (Pass > MaxPass) MaxPass = Pass;
        }
        return MaxPass;
    }

    void Detection::RenderAnaylysisWidgets_InScreen()
    {
        if (ImGui::Checkbox("Set ROI?", &bSetROI))
        {
            UpdateRoiScreenData();
        }
        if (bSetROI)
        {
            if (ImGui::SliderFloat4("Roi Region (x1, y1, x2, y2)", RoiRegion, 0.f, 1.0f))
            {
                UpdateRoiScreenData();
            }
            ImGui::SameLine();
            ImGui::ColorEdit3("##hint", (float*)&HintColor, ImGuiColorEditFlags_NoInputs);
        }
        // ImGui::SetNextItemWidth(240.f);
        // ImGui::DragInt("Per Unit Frame Size", &PUFS, 1, 1, TotalClipFrameSize);
        static ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;

        if (ImGui::BeginTable("##InScreenSize", 4, flags, ImVec2(-1, 0)))
        {
            // TODO: make Graph column align with play progress bar
            ImGui::TableSetupColumn("Group", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Max", ImGuiTableColumnFlags_WidthFixed, 125.0f);
            ImGui::TableSetupColumn("##Graphs");
            ImGui::TableSetupColumn("Current", ImGuiTableColumnFlags_WidthFixed, 125.0f);
            ImGui::TableHeadersRow();
            ImPlot::PushColormap(ImPlotColormap_Plasma);
            size_t row = 0;
            for (const auto& [Cat, Data] : InScreenRoiData)
            {
                ImVec4 CatColor;
                for (auto [c, col] : Categories)
                {
                    if (c == Cat)
                    {
                        CatColor = col;
                        break;
                    }
                }
                const char* id = Cat.c_str();

                const int MaxD = Utils::Max(Data);
                // spdlog::info("is size the same? Data {} VS Total frames {} ", Data.size(), TotalClipFrameSize);
                // the size diff cost me 2 hours to fix...
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text(id);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%d", MaxD);
                ImGui::TableSetColumnIndex(2);
                // TODO: drag rect will crash when set roi frequently... maybe draw the line myself!
                // HX1 = CurrentFrame; HY1 = MaxD; HX2 = CurrentFrame + 0.5f; HY2 = 0;
                ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
                if (ImPlot::BeginPlot(id, ImVec2(-1, 35), ImPlotFlags_CanvasOnly | ImPlotFlags_NoChild))
                {
                    ImPlot::SetupAxes(0, 0, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                    ImPlot::SetupAxesLimits(0, TotalClipFrameSize, 0, MaxD, ImGuiCond_Always);
                    ImPlot::SetNextLineStyle(CatColor);
                    ImPlot::SetNextFillStyle(CatColor, 0.25);
                    ImPlot::PlotLine(id, Data.data(), TotalClipFrameSize, 1, 0, ImPlotLineFlags_Shaded, 0);
                    // ImPlot::DragRect(ImGui::GetID("CFHint"), &HX1,&HY1, &HX2, &HY2, HintColor,
                    //                  ImPlotDragToolFlags_NoInputs);
                    // ImPlot::PlotLine(id, Values, 10, 1, 0, ImPlotLineFlags_Shaded, 0);
                    ImPlot::EndPlot();
                }
                ImPlot::PopStyleVar();
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%d", CurrentCatCount[Cat]);
                row++;
            }
            ImGui::EndTable();
        }
    }

    void Detection::UpdateRoiScreenData()
    {
        InScreenRoiData.clear();
        for (auto C : Categories)
            InScreenRoiData[C.CatName] = std::vector<int>();
        std::map<std::string, int> PerCatCountOneFrame;
        // for (const auto& [F, LL] : LoadedLabels)
        for (int i = 1; i < TotalClipFrameSize + 1; i++)
        {
            if (LoadedLabels.count(i) == 0)
            {
                for (const auto& C : Categories)
                {
                    InScreenRoiData[C.CatName].push_back(0);
                }
                continue;
            }
            auto LL = LoadedLabels[i];
            for (auto C : Categories)
            {
                PerCatCountOneFrame[C.CatName] = 0;
            }
            for (const auto& L : LL)
            {
                if (bSetROI)
                {
                    if (L.X >= std::min(RoiRegion[0], RoiRegion[2]) && L.X <= std::max(RoiRegion[0], RoiRegion[2]) &&
                        L.Y >= std::min(RoiRegion[1], RoiRegion[3]) && L.Y <= std::max(RoiRegion[1], RoiRegion[3]))
                    {
                        PerCatCountOneFrame[Categories[L.CatID].CatName] += 1;
                    }
                }
                else
                {
                    PerCatCountOneFrame[Categories[L.CatID].CatName] += 1;
                }
            }
            for (const auto& [Cat, Count] : PerCatCountOneFrame)
            {
                InScreenRoiData[Cat].push_back(Count);
            }
        }
    }

    void Detection::UpdateCurrentCatCount()
    {
        for (auto [k, v] : CurrentCatCount)
        {
            CurrentCatCount[k] = 0;
        }
        if (LoadedLabels.count(CurrentFrame))
        {
            for (const FLabelData& L : LoadedLabels[CurrentFrame])
            {
                if (bSetROI)
                {
                    if (L.X >= std::min(RoiRegion[0], RoiRegion[2]) && L.X <= std::max(RoiRegion[0], RoiRegion[2]) &&
                        L.Y >= std::min(RoiRegion[1], RoiRegion[3]) && L.Y <= std::max(RoiRegion[1], RoiRegion[3]))
                    {
                        CurrentCatCount[Categories[L.CatID].CatName] += 1;
                    }
                }
                else
                {
                    CurrentCatCount[Categories[L.CatID].CatName] += 1;
                }
            }
        }
    }

    void Detection::Analysis(const std::string& DName, const YAML::Node& DataNode)
    {
        if (IsAnalyzing) return;
        auto func = [=]()
        {
            DetectionClip = DataNode["TargetClip"].as<std::string>();
            LoadedLabels.clear();
            std::string Path = Setting::Get().ProjectPath + "/Detections/" + DName + "/labels";
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
            CurrentCatCount.clear();
            FIndividualData::CategoryNames.clear();
            for (const auto& C : DataNode["Categories"].as<std::vector<std::string>>())
            {
                Categories.push_back({C, Utils::RandomPickColor(Setting::Get().Theme)});
                CurrentCatCount[C] = 0;
                FIndividualData::CategoryNames.push_back(C);
            }
            // ClearCacheIndividuals();
            // GenerateCachedIndividualImages();
            // TrackIndividual();
            // IsIndividualDataLatest = true;
        };
        IsAnalyzing = true;

        AnalyzeFuture = std::async(std::launch::async, func);
    }
}
