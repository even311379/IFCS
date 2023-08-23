#include "Detection.h"

#include "DataBrowser.h"
#include "Setting.h"
#include "Style.h"
#include "Modals.h"

#include "Imspinner/imspinner.h"
#include "IconFontCppHeaders/IconsFontAwesome5.h"
#include "Implot/implot.h"
#include "misc/cpp/imgui_stdlib.h"

#include <fstream>
#include <variant>
#include <regex>

#include "yaml-cpp/yaml.h"
#include "opencv2/opencv.hpp"
#include <GLFW/glfw3.h>

#define DB DataBrowser::Get()

namespace IFCS
{
    static std::array<bool, 8> Devices;

    void Detection::Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose)
    {
        Panel::Setup(InName, InShouldOpen, InFlags, InCanClose);
        Devices[0] = true;
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
        if (ImGui::BeginTabBar("DetectionBar"))
        {
            if (ImGui::BeginTabItem("Make Detection"))
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

                if (ImGui::InputInt("Image Size (Test)", &ImageSize, 32, 128))
                {
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
                if (ImGui::TreeNode("Advanced"))
                {
                    // cuda device
                    ImGui::Text("Cuda devices to use:");
                    if (ImGui::Checkbox("0", &Devices[0])) UpdateDetectionScript();
                    ImGui::SameLine();
                    if (ImGui::Checkbox("1", &Devices[1])) UpdateDetectionScript();
                    ImGui::SameLine();
                    if (ImGui::Checkbox("2", &Devices[2])) UpdateDetectionScript();
                    ImGui::SameLine();
                    if (ImGui::Checkbox("3", &Devices[3])) UpdateDetectionScript();
                    ImGui::SameLine();
                    if (ImGui::Checkbox("4", &Devices[4])) UpdateDetectionScript();
                    ImGui::SameLine();
                    if (ImGui::Checkbox("5", &Devices[5])) UpdateDetectionScript();
                    ImGui::SameLine();
                    if (ImGui::Checkbox("6", &Devices[6])) UpdateDetectionScript();
                    ImGui::SameLine();
                    if (ImGui::Checkbox("7", &Devices[7])) UpdateDetectionScript();

                    ImGui::TreePop();
                }
                if (IsDetectionNameUsed)
                    ImGui::TextColored(Style::RED(400, Setting::Get().Theme),
                                       "s% has already is use... Try another name!",
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
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Analysis"))
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
                ImGui::SameLine();
                ImGui::Checkbox("Display helper lines?", &DisplayHelperLines);
                if (IsAnalyzing)
                {
                    ImGui::Text("Wait for analyzing...");
                    ImSpinner::SpinnerBarsScaleMiddle("Spinner2", 6, ImColor(Style::BLUE(400, Setting::Get().Theme)));
                    if (AnalyzeFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
                    {
                        OnAnalyzeFinished();
                    }
                }
                // if not select... no need to render other stuff
                if (strlen(SelectedDetection) == 0 || IsAnalyzing)
                {
                    ImGui::EndTabItem();
                    ImGui::EndTabBar();
                    return;
                }

                int N = 0;
                for (auto& Cat : Categories)
                {
                    ImGui::ColorEdit3(Cat.CatName.c_str(), (float*)&Cat.CatColor, ImGuiColorEditFlags_NoInputs);
                    N++;
                    if (N < Categories.size() && N % 6 != 0)
                    {
                        ImGui::SameLine();
                    }
                }
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 1280) * 0.5f);
                ImVec2 StartPos = ImGui::GetCursorScreenPos();
                ImGui::Image((void*)(intptr_t)DB.LoadedFramePtr, ImVec2(1280, 720));
                RenderDetectionBox(StartPos);
                static char* PlayIcon;
                if (DB.DB.CurrentFrame == DB.VideoEndFrame)
                    PlayIcon = ICON_FA_SYNC;
                else if (IsPlaying)
                    PlayIcon = ICON_FA_PAUSE;
                else
                    PlayIcon = ICON_FA_PLAY;
                if (ImGui::Button(PlayIcon, {120, 32}))
                {
                    if (DB.CurrentFrame == DB.VideoEndFrame)
                        DB.CurrentFrame = DB.VideoStartFrame;
                    IsPlaying = !IsPlaying;
                }
                ImGui::SameLine();
                static ImVec2 NewFramePad(4, (32 - ImGui::GetFontSize()) / 2);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, NewFramePad);
                ImGui::SetNextItemWidth(120.f);
                if (ImGui::DragInt("##PlayStart", &DB.VideoStartFrame, 1, 0, DB.VideoEndFrame - 1))
                {
                    if (DB.VideoStartFrame < 0) DB.VideoStartFrame = 0;
                    if (DB.VideoStartFrame > DB.VideoEndFrame) DB.VideoStartFrame = DB.VideoEndFrame - 1;
                    if (DB.CurrentFrame < DB.VideoStartFrame) DB.CurrentFrame = DB.VideoStartFrame;
                }
                ImGui::PopStyleVar();
                ImGui::SameLine();
                DrawPlayRange();
                ImGui::SameLine();
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, NewFramePad);
                ImGui::SetNextItemWidth(120.f);
                if (ImGui::DragInt("##PlayEnd", &DB.VideoEndFrame, 1, DB.VideoStartFrame + 1, TotalClipFrameSize))
                {
                    DB.CurrentFrame = DB.VideoStartFrame;
                    if (DB.VideoEndFrame > TotalClipFrameSize) DB.VideoEndFrame = TotalClipFrameSize;
                }
                ImGui::PopStyleVar();
                if (IsLoadingVideo)
                {
                    ImSpinner::SpinnerBarsScaleMiddle("Spinner3", 6, ImColor(Style::BLUE(400, Setting::Get().Theme)));
                    ImGui::Text("Video Loading...");
                    int NumCoreFinished = 0;
                    for (size_t T = 0; T < Setting::Get().CoresToUse; T++)
                    {
                        if (DataBrowser::Get().LoadingVideoTasks[T].wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
                        {
                            NumCoreFinished++;
                        }
                    }
                    if (NumCoreFinished == Setting::Get().CoresToUse)
                    {
                        IsLoadingVideo = false;
                        if (DB.ForceCloseLoading)
                        {
                            DB.ForceCloseLoading = false;
                            DB.LoadingVideoBlock(IsLoadingVideo, 0, DetectionClip);
                        }
                    }
                }
                if (SelectedAnalysisType == 0)
                {
                    RenderAnalysisWidgets_Pass();
                }
                else
                {
                    RenderAnalysisWidgets_InScreen();
                }

                ProcessingVideoPlay();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
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
        SetPathScript = "cd /d " + Setting::Get().YoloV7Path; // add /d to allow change drivers  ex C:/ to D:/ ... this is very change code in windows system...
        DetectScript = "";
        DetectScript += Setting::Get().PythonPath + "/python detect.py";
        DetectScript += " --weights " + ProjectPath + "/Models/" + SelectedModelName + "/weights/best.pt";
        DetectScript += " --conf-thres " + std::to_string(Confidence).substr(0, 4);
        DetectScript += " --iou-thres " + std::to_string(IOU).substr(0, 4);
        DetectScript += " --img-size " + std::to_string(ImageSize);
        DetectScript += " --project " + ProjectPath + "/Detections";
        DetectScript += " --name " + DetectionName;
        DetectScript += " --source " + ProjectPath + "/Clips/" + SelectedClip;
        std::string temp;
        int i = 0;
        for (bool b : Devices)
        {
            if (b) temp += std::to_string(i) + ",";
            i++;
        }
        DetectScript += " --device " + temp.substr(0, temp.size() - 1);
        DetectScript += " --save-txt --save-conf";
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

    static bool Enable_FISHWAY_MIDDLE = false;

    void Detection::RenderDetectionBox(ImVec2 StartPos)
    {
        if (LoadedLabels.count(DB.CurrentFrame))
        {
            for (const FLabelData& L : LoadedLabels[DB.CurrentFrame])
            {
                if (bSetROI)
                {
                    if (L.X < std::min(RoiRegion[0], RoiRegion[2]) || L.X > std::max(RoiRegion[0], RoiRegion[2]) ||
                        L.Y < std::min(RoiRegion[1], RoiRegion[3]) || L.Y > std::max(RoiRegion[1], RoiRegion[3]))
                    {
                        continue;
                    }
                }
                
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
            ImVec2 FMP1 = StartPos;
            ImVec2 FSP2, FEP2, FMP2;
            float MaxPos = std::max(FishwayPos[0], FishwayPos[1]);
            float MinPos = std::min(FishwayPos[0], FishwayPos[1]);
            float W = WorkArea.x;
            float H = WorkArea.y;
            if (IsVertical)
            {
                FSP1.y += FishwayPos[0] * H;
                FSP2 = FSP1;
                FSP2.x += W;
                FEP1.y += FishwayPos[1] * H;
                FEP2 = FEP1;
                FEP2.x += W;
                ImGui::GetWindowDrawList()->AddText(FSP2, HintCol, "Start");
                ImGui::GetWindowDrawList()->AddText(FEP2, HintCol, "End");
                if ( MinPos < FishwayMiddle  && FishwayMiddle < MaxPos && Enable_FISHWAY_MIDDLE)
                {
                    FMP1.y += FishwayMiddle * H;
                    FMP2 = FMP1;
                    FMP2.x += W;
                    ImGui::GetWindowDrawList()->AddText(FMP2, HintCol, "Middle");
                }
            }
            else
            {
                FSP1.x += FishwayPos[0] * W;
                FSP2 = FSP1;
                FSP2.y += H;
                FEP1.x += FishwayPos[1] * W;
                FEP2 = FEP1;
                FEP2.y += H;
                ImGui::GetWindowDrawList()->AddText(FSP1 + ImVec2(0, -ImGui::GetFontSize()), HintCol, "Start");
                ImGui::GetWindowDrawList()->AddText(FEP1 + ImVec2(0, -ImGui::GetFontSize()), HintCol, "End");
                if ( MinPos < FishwayMiddle  && FishwayMiddle < MaxPos && Enable_FISHWAY_MIDDLE)
                {
                    FMP1.x += FishwayMiddle * W;
                    FMP2 = FMP1;
                    FMP2.y += H;
                    ImGui::GetWindowDrawList()->AddText(FMP1 + ImVec2(0, -ImGui::GetFontSize()), HintCol, "Middle");
                }
            }
            ImGui::GetWindowDrawList()->AddLine(FSP1, FSP2, HintCol, 3);
            ImGui::GetWindowDrawList()->AddLine(FEP1, FEP2, HintCol, 3);
            if ( MinPos < FishwayMiddle  && FishwayMiddle < MaxPos)
                ImGui::GetWindowDrawList()->AddLine(FMP1, FMP2, HintCol, 3);
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


    void Detection::ProcessingVideoPlay()
    {
        if (!IsPlaying) return;
        static float TimePassed = 0.f;
        TimePassed += (1.0f / ImGui::GetIO().Framerate);
        if (TimePassed < (1 / ClipFPS)) return;
        TimePassed = 0.f;
        DB.DisplayFrame(DB.CurrentFrame, DetectionClip);
        DB.CurrentFrame += 1;
        DB.LoadingVideoBlock(IsLoadingVideo, DB.CurrentFrame, DetectionClip);
        if (DB.CurrentFrame == DB.VideoEndFrame)
        {
            IsPlaying = false;
        }
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
        ImVec2 RectPlayEnd = RectStart + ImVec2(float(DB.CurrentFrame - DB.VideoStartFrame) / float(DB.VideoEndFrame - DB.VideoStartFrame) * Width,
                                                10);
        ImGui::GetWindowDrawList()->AddRectFilled(RectPlayStart, RectPlayEnd, PlayColor);
        // draw current frame text?
        if (RectPlayEnd.x > RectPlayStart.x + 30)
        {
            char FrameTxt[8];
            sprintf(FrameTxt, "%d", DB.CurrentFrame);
            ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 16,
                                                RectPlayEnd + ImVec2(strlen(FrameTxt) * -16.f, -18.f), Color, FrameTxt);
        }
        ImGui::SetCursorScreenPos(RectPlayStart);
        ImGui::InvisibleButton("PlayControl", ImVec2(Width, 15));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            const int FrameAtPosition = int(
                (ImGui::GetMousePos().x - RectStart.x) / Width * (float)(DB.VideoEndFrame - DB.VideoStartFrame)) + DB.VideoStartFrame;
            ImGui::SetTooltip("%d", FrameAtPosition);
        }
        if (ImGui::IsItemClicked(0))
        {
            DB.CurrentFrame = int((ImGui::GetMousePos().x - RectStart.x) / Width * (float)(DB.VideoEndFrame - DB.VideoStartFrame)) +
                DB.VideoStartFrame;
            if (DB.CurrentFrame < DB.VideoStartFrame) DB.CurrentFrame = DB.VideoStartFrame;
            IsPlaying = false;
            DB.DisplayFrame(DB.CurrentFrame, DetectionClip);
            DB.LoadingVideoBlock(IsLoadingVideo, DB.CurrentFrame, DetectionClip);
        }
    }

    static bool ENABLE_SPEED_THRESHOLD = true;
    static bool ENABLE_BODY_SIZE_BUFFER = true;
    static float MAX_SPEED_THRESHOLD = 0.1f;
    static float BODY_SIZE_BUFFER = 0.2f;
    static int NUM_BUFFER_FRAMES = 10;
    static std::string OtherCategoryName = "";

    void Detection::RenderAnalysisWidgets_Pass()
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
        ImGui::SetNextItemWidth(240.f);
        if (ImGui::SliderFloat2("Fish way start / end", FishwayPos, 0.f, 1.f))
        {
            IsIndividualDataLatest = false;
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Enable fishway middle?", &Enable_FISHWAY_MIDDLE))
        {
            FishwayMiddle = FishwayPos[1];
        }
        Utils::AddSimpleTooltip("Can further restrict enter zone (between start to middle) to track new individual./n"
                                "When you found repeated count near the end line area, turn on this value may help!");
        if (Enable_FISHWAY_MIDDLE)
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120.f);
            if (ImGui::SliderFloat("Fishway middle", &FishwayMiddle, 0.f, 1.f))
            {
                IsIndividualDataLatest = false;
            }
        }
        ImGui::SameLine();
        ImGui::ColorEdit3("##hint", (float*)&HintColor, ImGuiColorEditFlags_NoInputs);
        
        if (ImGui::TreeNode("Advanced pass count parameters"))
        {
            if (ImGui::Checkbox("Enable max speed threshold?", &ENABLE_SPEED_THRESHOLD))
            {
                IsIndividualDataLatest = false;
            }
            Utils::AddSimpleTooltip("A threshold value to decide how to assign prediction to individual. "
                "Same individual should locate at similar location, it's speed (pixel^2 / frames) should be smaller than"
                " speed threshold * sqrt(ClipHeight^2 + ClipWidth^2)");
            
            if (ImGui::Checkbox("Enable body size buffer", &ENABLE_BODY_SIZE_BUFFER))
            {
                IsIndividualDataLatest = false;
            }
            Utils::AddSimpleTooltip("A threshold value to decide how to assign prediction to individual. "
                "Same individual should have similar body size among frames. If this value is too small, same individual could be judged as different!");
            
            if (ENABLE_SPEED_THRESHOLD)
            {
                ImGui::SetNextItemWidth(240.f);
                if (ImGui::DragFloat("Max speed threshould", &MAX_SPEED_THRESHOLD, 0.01f, 0.01f, 0.5f, "%.2f"))
                {
                    IsIndividualDataLatest = false;
                }
            }
            
            if (ENABLE_BODY_SIZE_BUFFER)
            {
                ImGui::SetNextItemWidth(240.f);
                if (ImGui::DragFloat("Body size threshould", &BODY_SIZE_BUFFER, 0.01f, 0.01f, 0.5f, "%.2f"))
                {
                    IsIndividualDataLatest = false;
                }
            }
            
            ImGui::SetNextItemWidth(240.f);
            if (ImGui::DragInt("Frames buffer threshold", &NUM_BUFFER_FRAMES, 1, 1, 60))
            {
                IsIndividualDataLatest = false;
            }
            Utils::AddSimpleTooltip("A threshold value to remove individaul that only appear in single frame. "
                "If no new prediction for that individual after N frames are tracked, that individual could be a noise, and remove it");
            // TODO: it required combo box... so fucking tedious...
            // use other group name instead...
            ImGui::SetNextItemWidth(240.f);
            if (ImGui::BeginCombo("Other group", ""))
            {
                if (ImGui::Selectable(""))
                {
                    OtherCategoryName = "";
                    IsIndividualDataLatest = false;
                }
                for (const auto& Cat : Categories)
                {
                    if (ImGui::Selectable(Cat.CatName.c_str()))
                    {
                        OtherCategoryName = Cat.CatName;
                        IsIndividualDataLatest = false;
                    }
                }
                ImGui::EndCombo();
            }
            Utils::AddSimpleTooltip("Same individual will be tracked during different frames, however, during each frame, the model may predict it as different category! "
                "If other category name is set, the final category will decide by highest confidence value across the frames except the 'other category', otherwise, it will just pick one with "
                "highest confidence value!");
            ImGui::TreePop();
        }
        if (!IsIndividualDataLatest)
        {
            if (ImGui::Button("Update individual tracking"))
            {
                TrackIndividual();
            }
        }

        if (IndividualData.empty()) return;

        UpdateAccumulatedPasses();
        static ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;

        if (ImGui::BeginTable("##Pass", 3, flags, ImVec2(-1, 0)))
        {
            ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("Total Pass", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            char buf[64];
            sprintf(buf, "Accumulated Passes (Frame %d to %d)", DB.VideoStartFrame, DB.CurrentFrame);
            ImGui::TableSetupColumn(buf);
            ImGui::TableHeadersRow();
            for (const auto& [Cat, Pass] : CurrentPass)
            {
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
        if (ImGui::TreeNode("Individual Data"))
        {
            flags |= ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable |
                ImGuiTableFlags_SortMulti;
            ImGuiTableColumnFlags CF = ImGuiTableColumnFlags_DefaultSort;
            if (ImGui::BeginTable("##Individual", 7, flags, ImVec2(-1, ImGui::GetTextLineHeightWithSpacing() * 20)))
            {
                ImGui::TableSetupColumn("Category", CF, 0.f, IndividualColumnID_Category);
                ImGui::TableSetupColumn("Is Passed", CF, 0.f, IndividualColumnID_IsPassed);
                ImGui::TableSetupColumn("ApproxSpeed (pixel / frame)", CF, 0.f, IndividualColumnID_ApproxSpeed);
                ImGui::TableSetupColumn("ApproxBodySize (pixel^2)", CF, 0.f, IndividualColumnID_ApproxBodySize);
                ImGui::TableSetupColumn("Enter Frame", CF, 0.f, IndividualColumnID_EnterFrame);
                ImGui::TableSetupColumn("Leave Frame", CF, 0.f, IndividualColumnID_LeaveFrame);
                // ImGui::TableSetupColumn("##view", ImGuiTableColumnFlags_NoSort, 0.f);
                ImGui::TableHeadersRow();

                if (ImGuiTableSortSpecs* SortSpec = ImGui::TableGetSortSpecs())
                {
                    if (SortSpec->SpecsDirty)
                    {
                        FIndividualData::CurrentSortSpecs = SortSpec;
                        if (IndividualData.size() > 1)
                            qsort(&IndividualData[0], IndividualData.size(), sizeof(IndividualData[0]),
                                  FIndividualData::CompareWithSortSpecs);
                        FIndividualData::CurrentSortSpecs = NULL;
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
                        if (D->IsCompleted)
                            ImGui::TextColored(Style::GREEN(400, Setting::Get().Theme), "Pass");
                        else
                            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "???");
                        ImGui::TableNextColumn();
                        ImGui::Text("%.2f", D->GetApproxSpeed());
                        ImGui::TableNextColumn();
                        ImGui::Text("%.2f", D->GetApproxBodySize());
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", D->GetEnterFrame());
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", D->GetLeaveFrame());
                        ImGui::PopID();
                    }
                ImGui::EndTable();
            }
            ImGui::TreePop();
        }
        static bool IsGeneratingDemo = false;
        if (IsGeneratingDemo)
        {
            ImSpinner::SpinnerBarsScaleMiddle("Spinner3", 6, ImColor(Style::BLUE(400, Setting::Get().Theme)));
            if (GenerateDemoFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
            {
                IsGeneratingDemo = false;
            }
        }
        else if (ImGui::TreeNode("Generate demo video"))
        {
            static ImVec4 DemoTextColor = {1.f, 1.f, 1.f, 1.f};
            static bool ShouldExportTotal = false;
            ImGui::ColorEdit3("Demo text color", (float*)&DemoTextColor, ImGuiColorEditFlags_NoInputs);
            ImGui::Text("Select which categories to show:");
            for (auto& C : Categories)
            {
                ImGui::Checkbox(C.CatName.c_str(), &C.ShouldExportInDemo);    
            }
            ImGui::Checkbox("Total", &ShouldExportTotal);
            if (ImGui::Button("Generate", {240.f, 0.f}))
            {
                auto func = [=]()
                {
                    cv::VideoCapture cap(DetectionClip);
                    cv::VideoWriter writer(Setting::Get().ProjectPath+"/Detections/Demo_"+ Utils::GetCurrentTimeString(false, true) + ".mp4",
                        cv::VideoWriter::fourcc('m','p','4','v'), ClipFPS, cv::Size(ClipWidth, ClipHeight));
                    int frame_num = 0;
                    std::map<int, std::map<std::string, int>> PassData;
                    for (const auto&  Cat : Categories)
                    {
                        if (Cat.ShouldExportInDemo)
                            PassData[0][Cat.CatName] = 0;
                    }
                    if (ShouldExportTotal)
                        PassData[0]["Total"] = 0;
                    for (int i = 1; i < TotalClipFrameSize; i++)
                    {
                        PassData[i] = PassData[i-1];
                        for (const auto& Data : IndividualData)
                        {
                            if (Data.IsCompleted && Data.GetLeaveFrame() == i)
                            {
                                if (PassData[i].count(Data.GetName()))
                                    PassData[i][Data.GetName()] += 1;
                                if (ShouldExportTotal)
                                    PassData[i]["Total"] += 1;
                            }
                        }
                    }
                    
                    const static int CATEGORY_NAME_LENGTH = 12;
                    int LineHeight = cv::getTextSize("some random", cv::FONT_HERSHEY_SIMPLEX, 1.f, 1, nullptr).height * 2;
                    while (true)
                    {
                        cv::Mat frame;
                        cap >> frame;
                        if (frame.empty()) break;
                        if (LoadedLabels.count(frame_num))
                        {
                            for (auto L : LoadedLabels[frame_num])
                            {
                                int p1x = static_cast<int>((L.X - 0.5f*L.Width) * static_cast<float>(ClipWidth));
                                int p1y = static_cast<int>((L.Y - 0.5f*L.Height) * static_cast<float>(ClipHeight));
                                int p2x = static_cast<int>((L.X + 0.5f*L.Width) * static_cast<float>(ClipWidth));
                                int p2y = static_cast<int>((L.Y + 0.5f*L.Height) * static_cast<float>(ClipHeight));
                                ImVec4 CatColor = Categories[L.CatID].CatColor;
                                cv::Scalar CatColor_CV = cv::Scalar(int(CatColor.z*255),int(CatColor.y*255),int(CatColor.x*255));
                                cv::rectangle(frame, cv::Point(p1x, p1y), cv::Point(p2x, p2y), CatColor_CV ,2);
                                std::string CatName = Categories[L.CatID].CatName;
                                cv::Size TextSize = cv::getTextSize(CatName, cv::FONT_HERSHEY_SIMPLEX, 1.f, 2, nullptr);
                                cv::rectangle(frame, cv::Point(p1x, p1y), cv::Point(p1x + TextSize.width + 4, p1y - TextSize.height - 4), CatColor_CV, -1);
                                cv::putText(frame, CatName, cv::Point(p1x + 2, p1y - 2), cv::FONT_HERSHEY_SIMPLEX, 1.f, cv::Scalar(255,255,255), 1, cv::LINE_AA);
                            }
                        }
                        // TODO: since the font is not mono space... this manner still can not guarantee they align together... just forget about the perfection...
                        int L = 0;
                        for (const auto& [k, v]: PassData[frame_num])
                        {
                            if (k == "Total") continue;
                            size_t cat_l = k.size();
                            std::string cat_mod = cat_l < CATEGORY_NAME_LENGTH? k + std::string(CATEGORY_NAME_LENGTH-cat_l, ' ') : k;
                            std::string msg = cat_mod + ": " + std::to_string(v);
                            cv::putText(frame, msg, cv::Point(50, ClipHeight-50 - LineHeight * L), cv::FONT_HERSHEY_SIMPLEX, 1.f,
                                cv::Scalar(int(DemoTextColor.z*255),int(DemoTextColor.y*255) ,int(DemoTextColor.x*255)), 1, cv::LINE_AA);
                            L++;
                        }
                        std::string msg = "Total       : " + std::to_string(PassData[frame_num]["Total"]);
                        cv::putText(frame, msg, cv::Point(50, ClipHeight-50 - LineHeight * L), cv::FONT_HERSHEY_SIMPLEX, 1.f,
                            cv::Scalar(int(DemoTextColor.z*255),int(DemoTextColor.y*255) ,int(DemoTextColor.x*255)), 1, cv::LINE_AA);
                        
                        writer.write(frame);
                        frame_num ++;
                    }
                    cap.release();
                    writer.release();
                };
                IsGeneratingDemo = true;
                GenerateDemoFuture = std::async(std::launch::async, func);
            }
            ImGui::Text("Demo video will locate in %s", (Setting::Get().ProjectPath+"/Detections").c_str());
            ImGui::TreePop();
        }
    }

    bool Detection::IsSizeSimilar(const FLabelData& Label1, const FLabelData& Label2, int FrameDiff)
    {
        // expose these vars to users?
        // the same individual should have same size between frames, but but to camera angle and it could be slightly different
        // considering multiple frames....
        if (!ENABLE_BODY_SIZE_BUFFER) return true;

        float S1 = Label1.GetApproxBodySize(ClipWidth, ClipHeight);
        float S2 = Label2.GetApproxBodySize(ClipWidth, ClipHeight);

        return (S1 < S2 * std::pow(1 + BODY_SIZE_BUFFER, FrameDiff + 1) && S1 > S2 * std::pow(
                1 - BODY_SIZE_BUFFER, FrameDiff + 1)) ||
            (S2 < S1 * std::pow(1 + BODY_SIZE_BUFFER, FrameDiff + 1) && S2 > S1 * std::pow(
                1 - BODY_SIZE_BUFFER, FrameDiff + 1));
    }


    bool Detection::IsDistanceAcceptable(const FLabelData& Label1, const FLabelData& Label2, int FrameDiff)
    {
        // max speed per frame should never be faster than 0.1 * length of diagonal
        if (!ENABLE_SPEED_THRESHOLD) return true;
        const float Distance = Label1.Distance(Label2, ClipWidth, ClipHeight);
        return Distance / (FrameDiff) / std::pow(ClipWidth * ClipWidth + ClipHeight * ClipHeight, 0.5) <
            MAX_SPEED_THRESHOLD;
    }


    void Detection::TrackIndividual()
    {
        float MinPos = std::min(FishwayPos[0], FishwayPos[1]);
        float MaxPos = std::max(FishwayPos[0], FishwayPos[1]);
        if (MinPos < FishwayMiddle && FishwayMiddle < MaxPos)
        {
            MinPos = std::min(FishwayPos[0], FishwayMiddle);
            MaxPos = std::max(FishwayPos[0], FishwayMiddle);
        }
        IndividualData.clear();
        std::vector<FIndividualData> TempTrackData;
        for (const auto& [F, LL] : LoadedLabels)
        {
            for (const FLabelData& L : LL)
            {
                // check if inside
                bool IsInFishway;
                bool IsInLeavingArea;
                if (IsVertical)
                {
                    IsInFishway = (L.Y >= MinPos) && (L.Y <= MaxPos);
                    IsInLeavingArea = FishwayPos[0] > FishwayPos[1] ? L.Y < FishwayPos[1] : L.Y > FishwayPos[1];
                }
                else
                {
                    IsInFishway = (L.X >= MinPos) && (L.X <= MaxPos);
                    IsInLeavingArea = FishwayPos[0] > FishwayPos[1] ? L.X < FishwayPos[1] : L.X > FishwayPos[1];
                }

                // check has already tracked any
                if (IsInFishway)
                {
                    if (TempTrackData.empty())
                    {
                        TempTrackData.emplace_back(F, L);
                        TempTrackData[0].HasPicked = true;
                    }
                    else
                    {
                        int ClosestIdx = -1;
                        float ClosestDistance = 999999.f;
                        // check should add new individual or add info
                        int i = 0;
                        for (auto& Data : TempTrackData)
                        {
                            if (Data.HasPicked) continue;
                            auto it = Data.Info.end();
                            --it; // the last record in each tracking individual
                            const float Distance = it->second.Distance(L, ClipWidth, ClipHeight);
                            // get the closest one with size rule and speed rule met...
                            if (IsSizeSimilar(it->second, L, F - it->first) &&
                                IsDistanceAcceptable(it->second, L, F - it->first) &&
                                Distance <= ClosestDistance)
                            {
                                ClosestIdx = i;
                                ClosestDistance = Distance;
                            }
                            i++;
                        }
                        if (ClosestIdx == -1)
                        {
                            TempTrackData.emplace_back(F, L);
                            TempTrackData[TempTrackData.size() - 1].HasPicked = true;
                            // this bug !!! why there is no zero division error... 
                        }
                        else
                        {
                            TempTrackData[ClosestIdx].AddInfo(F, L);
                            TempTrackData[ClosestIdx].HasPicked = true;
                        }
                    }
                }
                
                // check if this one is JUST leave the area...
                else if (IsInLeavingArea)
                {
                    int ClosestIdx = -1;
                    float ClosestDistance = 999999.f;
                    // check should add new individual or add info
                    int i = 0;
                    for (auto& Data : TempTrackData)
                    {
                        if (Data.HasPicked) continue;
                        auto it = Data.Info.end();
                        --it; // the last record in each tracking individual
                        const float Distance = it->second.Distance(L, ClipWidth, ClipHeight);
                        // get the closest one with size rule and speed rule met...
                        if (IsSizeSimilar(it->second, L, F - it->first) &&
                            IsDistanceAcceptable(it->second, L, F - it->first) &&
                            Distance <= ClosestDistance)
                        {
                            ClosestIdx = i;
                            ClosestDistance = Distance;
                        }
                        i++;
                    }
                    if (ClosestIdx != -1)
                    {
                        TempTrackData[ClosestIdx].HasPicked = true;
                        TempTrackData[ClosestIdx].IsCompleted = true;
                    }
                }
            } // end of per frame

            // check which existing one should append

            for (auto& Data : TempTrackData)
            {
                // check is completed or should untrack
                Data.HasPicked = false;
                if (Data.Info.size() == 1) continue;
                if (std::prev(Data.Info.end())->first + NUM_BUFFER_FRAMES < F || Data.IsCompleted)
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
                        return (std::prev(Data.Info.end())->first + NUM_BUFFER_FRAMES < F || Data.IsCompleted);
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
               // TODO: should be TotalPass[Cat] = 1; ???
               // ... should be 1 and also need to fill actual 0?
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
            CurrentCatCount[Cat.CatName] = 0; // just for clean up the var
        }
        for (const auto& Data : IndividualData)
        {
            if (!Data.IsCompleted) continue;
            const int LeaveFrame = Data.GetLeaveFrame();
            if (LeaveFrame <= DB.CurrentFrame && LeaveFrame >= DB.VideoStartFrame)
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

    void Detection::RenderAnalysisWidgets_InScreen()
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
        UpdateCurrentCatCount();
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
                for (auto C : Categories)
                {
                    if (C.CatName == Cat)
                    {
                        CatColor = C.CatColor;
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
                // HX1 = DB.CurrentFrame; HY1 = MaxD; HX2 = DB.CurrentFrame + 0.5f; HY2 = 0;
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
        if (LoadedLabels.count(DB.CurrentFrame))
        {
            for (const FLabelData& L : LoadedLabels[DB.CurrentFrame])
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
        auto LoadLabels = [=]()
        {
            DetectionClip = DataNode["TargetClip"].as<std::string>();
            LoadedLabels.clear();
            std::string Path = Setting::Get().ProjectPath + "/Detections/" + DName + "/labels";
            for (const auto& Entry : std::filesystem::directory_iterator(Path))
            {
                std::string TxtName = Entry.path().filename().u8string();
                std::smatch m;

                // TODO: this regex make this bad...
                std::regex_search(TxtName, m, std::regex("_(\\d+).txt"));
                int FrameCount = std::stoi(m.str(1));

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
                LoadedLabels[FrameCount] = Labels;
            }
            Categories.clear();
            CurrentCatCount.clear();
            FIndividualData::CategoryNames.clear();
            FIndividualData::OtherName.clear();
            for (const auto& C : DataNode["Categories"].as<std::vector<std::string>>())
            {
                Categories.push_back({C, Utils::RandomPickColor(Setting::Get().Theme)});
                CurrentCatCount[C] = 0;
                FIndividualData::CategoryNames.push_back(C);
            }
            if (Utils::Contains(FIndividualData::CategoryNames, OtherCategoryName))
                FIndividualData::OtherName = OtherCategoryName;
        };
        IsAnalyzing = true;
        AnalyzeFuture = std::async(std::launch::async, LoadLabels);
    }

    void Detection::OnAnalyzeFinished()
    {
        IsAnalyzing = false;
        DB.VideoFrames.clear();
        DB.CurrentFrame = 0;
        cv::Mat Frame;
        cv::VideoCapture cap(DetectionClip);
        TotalClipFrameSize = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
        DB.VideoEndFrame = TotalClipFrameSize;
        ClipFPS = (float)cap.get(cv::CAP_PROP_FPS);
        ClipWidth = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
        FIndividualData::Width = ClipWidth;
        ClipHeight = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
        FIndividualData::Height = ClipHeight;
        cap >> Frame;
        cv::resize(Frame, Frame, cv::Size((int)WorkArea.x, (int)WorkArea.y)); // 16 : 9 // no need to resize?
        cv::cvtColor(Frame, Frame, cv::COLOR_BGR2RGB);
        cap.release();

        // call this later since clip param not set yet...
        UpdateRoiScreenData();
        
        DB.MatToGL(Frame);
        DB.LoadingVideoBlock(IsLoadingVideo, 0, DetectionClip);

    }
}
