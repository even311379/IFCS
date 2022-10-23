#include "FrameExtractor.h"

#include <fstream>
#include <numeric>
#include <random>


#include "Log.h"
#include "Setting.h"
#include "Utils.h"
#include "DataBrowser.h"

#include "ImguiNotify/font_awesome_5.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include <GLFW/glfw3.h>

#include "yaml-cpp/yaml.h"
#include "opencv2/opencv.hpp"
#include <spdlog/spdlog.h>

// #include <shellapi.h>


namespace IFCS
{
    void FrameExtractor::OnFrameLoaded()
    {
        YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/ExtractionRegions.yaml"));
        ClipInfo = DataBrowser::Get().SelectedClipInfo;
        ClipInfoIsLoaded = true;
        Regions.clear();
        if (Data[ClipInfo.ClipPath])
        {
            Regions = Data[ClipInfo.ClipPath].as<std::vector<float>>();
        }
        UpdateCurrentFrameInfo();
    }

    // TODO: about hide rendering when unfocused? to save resource? to prevent click....
    // wow... clicks define here will get triggered even if its in behind... Need detaied test... will it triger thing in other docked?
    void FrameExtractor::RenderContent()
    {
        if (!ClipInfoIsLoaded) return;

        std::string Title = DataBrowser::Get().FrameTitle;
        ImGui::PushFont(Setting::Get().TitleFont);
        float TitleWidth = ImGui::CalcTextSize(Title.c_str()).x;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - TitleWidth) * 0.5f);
        ImGui::Text(Title.c_str());
        ImGui::PopFont();
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 1024) * 0.5f);
        ImGui::Image((void*)(intptr_t)DataBrowser::Get().LoadedFramePtr, ImVec2(1024, 576));
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 1024) * 0.5f);

        if (ImGui::BeginTable("table_clip_info", 4))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::BulletText("Clip resolution");
            ImGui::TableNextColumn();
            ImGui::Text("%d X %d", ClipInfo.Width, ClipInfo.Height);
            ImGui::TableNextColumn();
            ImGui::BulletText("clip length");
            ImGui::TableNextColumn();
            int minute = ClipInfo.FrameCount / int(ClipInfo.FPS) / 60;
            int sec = (ClipInfo.FrameCount / int(ClipInfo.FPS)) % 60;
            ImGui::Text("%dm%ds", minute, sec);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::BulletText("Frame rate");
            ImGui::TableNextColumn();
            ImGui::Text("%f", ClipInfo.FPS);
            ImGui::TableNextColumn();
            ImGui::BulletText("frame count");
            ImGui::TableNextColumn();
            ImGui::Text("%d", ClipInfo.FrameCount);
            ImGui::EndTable();
        }

        int N_Regions = (int)Regions.size() / 2;
        if (BeginTimeline(N_Regions))
        {
            for (int r = 0; r < N_Regions; r++)
            {
                TimelineEvent(r);
            }
        }
        EndTimeline();
        TimelineControlButtons();
        ProcessingVideoPlay();
    }


    bool FrameExtractor::BeginTimeline(int opt_exact_num_rows)
    {
        temp_TimelineIndex = -1;

        const int num_visible_rows = 6;
        const float row_height = ImGui::GetTextLineHeightWithSpacing();
        const bool rv = ImGui::BeginChild("Extraction Timelines", ImVec2(0, row_height * num_visible_rows), false);
        ImGui::PushStyleColor(ImGuiCol_Border, GImGui->Style.Colors[ImGuiCol_Border]);
        ImGui::Columns(2, "Extraction Timelines");
        const float contentRegionWidth = ImGui::GetWindowContentRegionWidth();
        if (ImGui::GetColumnOffset(1) >= contentRegionWidth * 0.48f)
            ImGui::SetColumnOffset(1, contentRegionWidth * 0.15f);

        return rv;
    }


    bool FrameExtractor::TimelineEvent(int N)
    {
        char ID[64];
        sprintf(ID, "region %d", N);
        float Values[2];
        Values[0] = (float)Regions[N * 2];
        Values[1] = (float)Regions[N * 2 + 1];
        ++temp_TimelineIndex;
        if (TimelineRows > 0 && (temp_TimelineIndex < TimelineDisplayStart || temp_TimelineIndex >= TimelineDisplayEnd))
            return false; // item culling

        ImGuiWindow* Win = ImGui::GetCurrentWindow();
        const float TIMELINE_RADIUS = 6;
        const ImU32 InactiveColor = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
        const ImU32 ActiveColor = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonHovered]);
        const ImU32 LineColor = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Tab]);
        bool Changed = false;
        bool Hovered = false;

        ImGui::Text("%s", ID);
        bool JustDeleteRegion = false;
        if (ImGui::BeginPopupContextItem(ID))
        {
            if (ImGui::Button("Delete this region?"))
            {
                Regions.erase(Regions.begin() + 2 * N);
                Regions.erase(Regions.begin() + 2 * N); // do it twice with same index since it will shrink?
                JustDeleteRegion = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::NextColumn();

        const float ColumnOffset = ImGui::GetColumnOffset(1);
        const float ColumnWidth = ImGui::GetColumnWidth(1) - GImGui->Style.ScrollbarSize;
        const ImVec2 CursorPos(ImGui::GetWindowContentRegionMin().x + Win->Pos.x + ColumnOffset - TIMELINE_RADIUS,
                               Win->DC.CursorPos.y);
        const bool isMouseDraggingZero = ImGui::IsMouseDragging(0);

        for (int i = 0; i < 2; ++i)
        {
            if ((int)Values[i] < CurrentFrameStart) continue;
            if ((int)Values[i] > CurrentFrameStart + CurrentFrameRange) continue;
            ImVec2 Pos = CursorPos;
            Pos.x += ColumnWidth * (Values[i] - CurrentFrameStart) / CurrentFrameRange + TIMELINE_RADIUS;
            Pos.y += TIMELINE_RADIUS;

            ImGui::SetCursorScreenPos(Pos - ImVec2(TIMELINE_RADIUS, TIMELINE_RADIUS));
            ImGui::PushID(i);
            ImGui::InvisibleButton(ID, ImVec2(2 * TIMELINE_RADIUS, 2 * TIMELINE_RADIUS));
            bool active = ImGui::IsItemActive();
            if (active || ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%.0f", Values[i]);
                {
                    ImVec2 a(Pos.x, ImGui::GetWindowContentRegionMin().y + Win->Pos.y + Win->Scroll.y);
                    ImVec2 b(Pos.x, ImGui::GetWindowContentRegionMax().y + Win->Pos.y + Win->Scroll.y);
                    Win->DrawList->AddLine(a, b, LineColor);
                }
                Hovered = true;
            }
            if (active && isMouseDraggingZero)
            {
                Values[i] += ImGui::GetIO().MouseDelta.x / ColumnWidth * (float)CurrentFrameRange;
                Changed = Hovered = true;
            }

            ImGui::PopID();
            Win->DrawList->AddCircleFilled(
                Pos, TIMELINE_RADIUS, ImGui::IsItemActive() || ImGui::IsItemHovered() ? ActiveColor : InactiveColor);
        }

        ImVec2 Start = CursorPos;
        Start.x += ColumnWidth * (Values[0] - CurrentFrameStart) / float(CurrentFrameRange) + 2 * TIMELINE_RADIUS;
        Start.y += TIMELINE_RADIUS * 0.5f;
        ImVec2 End = Start + ImVec2(
            ColumnWidth * (Values[1] - Values[0]) / (float)CurrentFrameRange - 2 * TIMELINE_RADIUS,
            TIMELINE_RADIUS);
        ImGui::PushID(-1);
        ImGui::SetCursorScreenPos(Start);
        ImGui::InvisibleButton(ID, End - Start);
        if ((ImGui::IsItemActive() && isMouseDraggingZero))
        {
            const float deltaX = ImGui::GetIO().MouseDelta.x / ColumnWidth * (float)CurrentFrameRange;
            Values[0] += deltaX;
            Values[1] += deltaX;
            Changed = Hovered = true;
        }
        else if (ImGui::IsItemHovered()) Hovered = true;
        ImGui::PopID();

        ImGui::SetCursorScreenPos(CursorPos + ImVec2(0, ImGui::GetTextLineHeightWithSpacing()));
        Win->DrawList->AddRectFilled(
            Start, End, ImGui::IsItemActive() || ImGui::IsItemHovered() ? ActiveColor : InactiveColor);

        if (Values[0] > Values[1])
        {
            float tmp = Values[0];
            Values[0] = Values[1];
            Values[1] = tmp;
        }
        if (!JustDeleteRegion)
        {
            Values[0] = std::max(Values[0], 1.f);
            Values[1] = std::min(Values[1], (float)ClipInfo.FrameCount);
            Regions[N * 2] = Values[0];
            Regions[N * 2 + 1] = Values[1];
        }
        if (Hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        ImGui::NextColumn();
        return Changed;
    }

    //num vrt grid lines = 5 
    void FrameExtractor::EndTimeline()
    {
        const int NumVerticalGridLines = 5;
        const ImU32 TimelineRunningColor = IM_COL32(0, 128, 0, 200);
        const float row_height = ImGui::GetTextLineHeightWithSpacing();
        if (TimelineRows > 0)
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (TimelineRows - TimelineDisplayEnd) * row_height);
        ImGui::NextColumn();
        const ImGuiWindow* Win = ImGui::GetCurrentWindow();
        const float ColumnOffset = ImGui::GetColumnOffset(1);
        const float ColumnWidth = ImGui::GetColumnWidth(1) - GImGui->Style.ScrollbarSize;
        const float HorizontalInterval = ColumnWidth / NumVerticalGridLines;

        const ImU32 Color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
        const ImU32 LineColor = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Border]);
        const ImU32 TextColor = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Text]);
        const ImU32 MovingLineColor = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonActive]);
        const float Rounding = GImGui->Style.ScrollbarRounding;
        const float StartY = ImGui::GetWindowHeight() + Win->Pos.y;

        // Draw black vertical lines (inside scrolling area)
        for (int i = 1; i <= NumVerticalGridLines; ++i)
        {
            ImVec2 a = ImGui::GetWindowContentRegionMin() + Win->Pos;
            a.x += i * HorizontalInterval + ColumnOffset;
            Win->DrawList->AddLine(a, ImVec2(a.x, StartY), LineColor);
        }

        // Draw moving vertical line
        if (PlayProgress > 0.f && PlayProgress < TimelineDisplayEnd)
        {
            ImVec2 a = ImGui::GetWindowContentRegionMin() + Win->Pos;
            a.x += ColumnWidth * (PlayProgress / TimelineDisplayEnd) + ColumnOffset;
            Win->DrawList->AddLine(a, ImVec2(a.x, StartY), MovingLineColor, 2.0f); // change thickness to 2
        }

        if (ImGui::IsWindowHovered())
        {
            if (ImGui::GetMousePos().x > (ImGui::GetWindowPos().x + ColumnOffset))
            {
                CheckZoomInput();
            }
        }

        ImGui::PopStyleColor();
        ImGui::EndChild();

        // Draw bottom axis ribbon (outside scrolling region)
        Win = ImGui::GetCurrentWindow();
        const ImVec2 Start(ImGui::GetCursorScreenPos().x + ColumnOffset, ImGui::GetCursorScreenPos().y);
        const ImVec2 End(Start.x + ColumnWidth, Start.y + row_height);
        if (PlayProgress <= 0) Win->DrawList->AddRectFilled(Start, End, Color, Rounding);
        else if (PlayProgress > TimelineDisplayEnd)
            Win->DrawList->AddRectFilled(
                Start, End, TimelineRunningColor, Rounding);
        else
        {
            ImVec2 Median(Start.x + ColumnWidth * (PlayProgress / TimelineDisplayEnd), End.y);
            Win->DrawList->AddRectFilled(Start, Median, TimelineRunningColor, Rounding, 1 | 8);
            Median.y = Start.y;
            Win->DrawList->AddRectFilled(Median, End, Color, Rounding, 2 | 4);
            Win->DrawList->AddLine(Median, ImVec2(Median.x, End.y), MovingLineColor);
        }

        char tmp[256] = "";
        for (int i = 0; i < NumVerticalGridLines; ++i)
        {
            ImVec2 a = Start;
            a.x += i * HorizontalInterval;
            a.y = Start.y;

            ImFormatString(tmp, sizeof(tmp), "%.0f",
                           (float(i) / (float)NumVerticalGridLines / TimelineZoom + TimelinePan) * ClipInfo.FrameCount);
            Win->DrawList->AddText(a, TextColor, tmp);
        }

        ImGui::SetCursorScreenPos(Start);
        ImGui::InvisibleButton("##ProgressHover", End - Start);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsMouseClicked(0) || ImGui::IsMouseDragging(0))
            {
                // TODO: after it, need to click play button twice to start play? why?
                JustPaused = true;
                PlayProgress = (ImGui::GetMousePos().x - Start.x) / (End.x - Start.x) * TimelineDisplayEnd;
            }
            if (ImGui::IsMouseReleased(0))
            {
                DataBrowser::Get().LoadFrame(CurrentFrame);
            }
        }

        // draw pan-zoom control bar
        const ImU32 BGColor = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_TextDisabled]);
        const ImVec2 PZStart = {Start.x, End.y + row_height};
        const ImVec2 PZEnd = {PZStart.x + ColumnWidth, PZStart.y + row_height};
        Win->DrawList->AddRectFilled(PZStart, PZEnd, BGColor, Rounding); // bg bar
        const ImVec2 PZControlStart = {PZStart.x + TimelinePan * ColumnWidth, PZStart.y};
        const ImVec2 PZControlEnd = {PZControlStart.x + (1.f / float(TimelineZoom)) * ColumnWidth, PZEnd.y};
        Win->DrawList->AddRectFilled(PZControlStart, PZControlEnd, Color, Rounding); // control bar
        ImGui::SetCursorScreenPos(PZControlStart);
        ImGui::InvisibleButton("##HH", PZControlEnd - PZControlStart);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsMouseDragging(0))
            {
                TimelinePan += ImGui::GetIO().MouseDelta.x / ColumnWidth;
                float MaxPan = 1.f - (1.f / float(TimelineZoom));
                if (TimelinePan >= MaxPan) TimelinePan = MaxPan;
                if (TimelinePan <= 0) TimelinePan = 0;
                UpdateCurrentFrameInfo();
            }
            CheckZoomInput();
        }
        ImGui::SetCursorPos(ImVec2(0, ImGui::GetCursorPosY() + row_height * 1.5f));
        if (ImGui::Button(ICON_FA_PLUS, ImVec2(96, 24)))
        {
            Regions.push_back((float)CurrentFrameStart + int(0.3f * (float)CurrentFrameRange));
            Regions.push_back((float)CurrentFrameStart + int(0.7f * (float)CurrentFrameRange));
        }
        Utils::AddSimpleTooltip("Add new region to perform frame extraction...");
        ImGui::SameLine();
        // TODO: add combo button here to choose which region to remove
        // hide functions in right click is really bad...
        ImGui::Dummy({ColumnOffset - 96, 0});
        ImGui::SameLine();
    }

    void FrameExtractor::TimelineControlButtons()
    {
        static char* PlayIcon;
        PlayIcon = IsPlaying ? ICON_FA_PAUSE : ICON_FA_PLAY;
        if (ImGui::Button(PlayIcon, {96, 24}))
        {
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
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Play / Pause clip");
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_SYNC, {96, 24}))
        {
            TimelinePan = 0.0f;
            TimelineZoom = 1.0f;
            UpdateCurrentFrameInfo();
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Reset zoom and pan for this clip");
        }
        ImGui::SameLine();
        // TODO: Add zoom control... hide it in ctrl + wheel it too unhuman...
        ImGui::Dummy({200, 0});
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.f);
        int MaxExtract = 0;
        for (size_t i = 0; i < Regions.size() / 2; i++)
        {
            MaxExtract += (int)(Regions[i * 2 + 1] - Regions[i * 2] + 1);
        }

        if (NumFramesToExtract > MaxExtract)
            NumFramesToExtract = MaxExtract;
        if (Regions.size() == 0) MaxExtract = 1;
        ImGui::DragInt("frames to ", &NumFramesToExtract, 1, 1, MaxExtract);
        ImGui::SameLine();
        if (ImGui::Button("Extract"))
        {
            PerformExtraction();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(240.f);

        // TODO: can not load this way... all become ????? ...
        /*symotion-prefix)
        const char* Options[] = {
            LOCTEXT("FrameExtractor.ModeOption1"),LOCTEXT("FrameExtractor.ModeOption2"),
            LOCTEXT("FrameExtractor.ModeOption3")
        };
        */
        const char* Options[] = {
            "Remove all previous extracts, then extract", "No remove, extract to requested numbers",
            "Remove frames without annotations, then extract to requested numbers", "中文會變亂碼嗎?"
        };
        ImGui::Combo("##ExtractionOptionsCombo", &SelectedExtractionOption, Options, IM_ARRAYSIZE(Options));
    }

    /*
     * multi thread way may actually worked...
     * the mistake could be just due to the error setup in cap property...
     * however, I've implement it in main thread... so just let it be...
     */
    void FrameExtractor::ProcessingVideoPlay()
    {
        if (!IsPlaying) return;
        // check if receive stop play

        // open cap
        static cv::VideoCapture Cap;
        if (JustPlayed)
        {
            Cap.open(ClipInfo.ClipPath);

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
        if (TimePassed < (1 / ClipInfo.FPS)) return;
        TimePassed = 0.f;
        // update frame
        cv::Mat frame;
        Cap.set(cv::CAP_PROP_POS_FRAMES, CurrentFrame);
        Cap >> frame;
        // cv::resize(frame, frame, cv::Size(1280, 720)); // 16 : 9 // no need to resize?
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGBA);
        DataBrowser::Get().LoadedFramePtr = 0;
        glGenTextures(1, &DataBrowser::Get().LoadedFramePtr);
        glBindTexture(GL_TEXTURE_2D, DataBrowser::Get().LoadedFramePtr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); // Same
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame.cols, frame.rows, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, frame.data);
        CurrentFrame += 1;
        PlayProgress += (1.0f / (float)CurrentFrameRange) * 100.0f;
        //check should release cap and stop play
        if (CurrentFrame == CurrentFrameEnd)
        {
            IsPlaying = false;
            Cap.release();
        }
    }

    void FrameExtractor::CheckZoomInput()
    {
        float Wheel = ImGui::GetIO().MouseWheel;
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && Wheel != 0.f)
        {
            TimelineZoom += Wheel;
            if (TimelineZoom < 1) TimelineZoom = 1.f;
            if (TimelineZoom > 50) TimelineZoom = 50.f;
            // zoom may affect pan...
            float MaxPan = 1.f - (1.f / float(TimelineZoom));
            if (TimelinePan >= MaxPan) TimelinePan = MaxPan;
            UpdateCurrentFrameInfo();
        }
    }

    void FrameExtractor::UpdateCurrentFrameInfo()
    {
        // CurrentFrameStart = int((float)ClipInfo.FrameCount / TimelineZoom * TimelinePan) + 1;
        CurrentFrameStart = int((float)ClipInfo.FrameCount * TimelinePan) + 1;
        CurrentFrameRange = int((float)ClipInfo.FrameCount / TimelineZoom);
        CurrentFrame = int(CurrentFrameStart + PlayProgress / 100.0f * CurrentFrameRange);
        CurrentFrameEnd = CurrentFrameStart + CurrentFrameRange - 1;
    }

    void FrameExtractor::PerformExtraction()
    {
        // save extraction range?
        //// read old and override
        YAML::Node RegionToExtractData = YAML::LoadFile(
            Setting::Get().ProjectPath + std::string("/Data/ExtractionRegions.yaml"));
        RegionToExtractData[ClipInfo.ClipPath] = YAML::Node(YAML::NodeType::Sequence);
        for (float r : Regions)
            RegionToExtractData[ClipInfo.ClipPath].push_back((int)r); // when save make it to int!!
        YAML::Emitter Out;
        Out << RegionToExtractData;
        std::ofstream fout(Setting::Get().ProjectPath + std::string("/Data/ExtractionRegions.yaml"));
        fout << Out.c_str();

        // TODO: grab  combo options... so far, just use remove all and extract to that amount...
        // save extracted frames
        // concat ranges, suffle, slice... done?
        // concat with ordered unique frames
        std::set<int> PossibleFrames;
        for (int R = 0; R < (int)Regions.size() / 2; R++)
        {
            std::vector<int> v((int)Regions[R * 2 + 1] - (int)Regions[R * 2] + 1);
            std::iota(v.begin(), v.end(), (int)Regions[R * 2]);
            for (int i : v)
                PossibleFrames.insert(i);
        }

        // sample
        std::vector<int> ExtractedFrames;
        std::sample(PossibleFrames.begin(), PossibleFrames.end(), std::back_inserter(ExtractedFrames),
                    NumFramesToExtract, std::mt19937_64{std::random_device{}()});
        std::sort(ExtractedFrames.begin(), ExtractedFrames.end()); // is sort required?

        // write to YAML!
        YAML::Node AnnotationData = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/Annotation.yaml"));
        // handle extract for existing clip
        // follows extraction option 1... clear anything existing...
        YAML::Node NewFramesNode;
        for (int j : ExtractedFrames)
        {
            NewFramesNode[std::to_string(j)] = YAML::Node(YAML::NodeType::Map);
        }

        AnnotationData[ClipInfo.ClipPath] = NewFramesNode;

        YAML::Emitter Out2;
        Out2 << AnnotationData;
        std::ofstream fout2(Setting::Get().ProjectPath + std::string("/Data/Annotation.yaml"));
        fout2 << Out2.c_str();
    }
}
