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

#include <shellapi.h>

namespace IFCS
{
    void FrameExtractor::LoadData()
    {
        YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + std::string("/Data/ExtractionRegions.yaml"));
        ClipInfo = DataBrowser::Get().SelectedClipInfo;
        ClipInfoIsLoaded = true;
        Regions.clear();
        if (Data[ClipInfo.ClipPath])
        {
            Regions = Data[ClipInfo.ClipPath].as<std::vector<int>>();
        }
    }

    // TODO: about hide rendering when unfocused? to save resource? to prevent click.... wow... clicks define here will get triggered even if its in behind...
    void FrameExtractor::RenderContent()
    {

        // spdlog::info("should not see me if I am docked behine...");
        if (!ClipInfoIsLoaded) return;

        std::string Title = DataBrowser::Get().FrameTitle;
        ImGui::PushFont(Setting::Get().TitleFont);
        float TitleWidth = ImGui::CalcTextSize(Title.c_str()).x;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - TitleWidth) * 0.5f);
        ImGui::Text(Title.c_str());
        ImGui::PopFont();
        // TODO: can not fit with (1280, 720) (no enough y space...)
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
        if (BeginTimeline( N_Regions))
        {
            for (int r = 0; r < N_Regions; r++)
            {
                TimelineEvent(r);
            }
        }
        EndTimeline();
        TimelineControlButtons();
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
        char id[64];
        snprintf(id, sizeof(id), "region %d", N);
        float values[2];
        float pan_amount = (TimelineZoom * TimelinePan * TimelineDisplayEnd);
        values[0] = float(Regions[N * 2]) / float(ClipInfo.FrameCount) * TimelineZoom * TimelineDisplayEnd - pan_amount;
        values[1] = float(Regions[N * 2 + 1]) / float(ClipInfo.FrameCount) * TimelineZoom * TimelineDisplayEnd - pan_amount;

        ++temp_TimelineIndex;
        if (TimelineRows > 0 && (temp_TimelineIndex < TimelineDisplayStart || temp_TimelineIndex >= TimelineDisplayEnd))
            return false; // item culling

        ImGuiWindow* win = ImGui::GetCurrentWindow();
        const float TIMELINE_RADIUS = 6;
        const ImU32 inactive_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
        const ImU32 active_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonHovered]);
        const ImU32 line_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Tab]);
        bool changed = false;
        bool hovered = false;

        ImGui::Text("%s", id);
        bool JustDeleteRegion = false;
        if (ImGui::BeginPopupContextItem(id))
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

        const float columnOffset = ImGui::GetColumnOffset(1);
        const float columnWidth = ImGui::GetColumnWidth(1) - GImGui->Style.ScrollbarSize;
        ImVec2 cursor_pos(ImGui::GetWindowContentRegionMin().x + win->Pos.x + columnOffset - TIMELINE_RADIUS,
                          win->DC.CursorPos.y);
        bool mustMoveBothEnds = false;
        const bool isMouseDraggingZero = ImGui::IsMouseDragging(0);

        for (int i = 0; i < 2; ++i)
        {
            ImVec2 pos = cursor_pos;
            pos.x += columnWidth * values[i] / TimelineDisplayEnd + TIMELINE_RADIUS;
            pos.y += TIMELINE_RADIUS;

            ImGui::SetCursorScreenPos(pos - ImVec2(TIMELINE_RADIUS, TIMELINE_RADIUS));
            ImGui::PushID(i);
            ImGui::InvisibleButton(id, ImVec2(2 * TIMELINE_RADIUS, 2 * TIMELINE_RADIUS));
            bool active = ImGui::IsItemActive();
            if (active || ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%d", ValueToFrame(values[i]));
                {
                    ImVec2 a(pos.x, ImGui::GetWindowContentRegionMin().y + win->Pos.y + win->Scroll.y);
                    ImVec2 b(pos.x, ImGui::GetWindowContentRegionMax().y + win->Pos.y + win->Scroll.y);
                    win->DrawList->AddLine(a, b, line_color);
                }
                hovered = true;
            }
            if (active && isMouseDraggingZero)
            {
                // TODO: the drag speed is mismatched when zoom
                values[i] += ImGui::GetIO().MouseDelta.x / columnWidth * static_cast<float>(TimelineDisplayEnd);
                changed = hovered = true;
            }

            ImGui::PopID();
            win->DrawList->AddCircleFilled(
                pos, TIMELINE_RADIUS, ImGui::IsItemActive() || ImGui::IsItemHovered() ? active_color : inactive_color);
        }

        ImVec2 start = cursor_pos;
        start.x += columnWidth * values[0] / float(TimelineDisplayEnd) + 2 * TIMELINE_RADIUS;
        start.y += TIMELINE_RADIUS * 0.5f;
        ImVec2 end = start + ImVec2(columnWidth * (values[1] - values[0]) / TimelineDisplayEnd - 2 * TIMELINE_RADIUS,
                                    TIMELINE_RADIUS);
        ImGui::PushID(-1);
        ImGui::SetCursorScreenPos(start);
        ImGui::InvisibleButton(id, end - start);
        if ((ImGui::IsItemActive() && isMouseDraggingZero) || mustMoveBothEnds)
        {
            const float deltaX = ImGui::GetIO().MouseDelta.x / columnWidth * TimelineDisplayEnd;
            values[0] += deltaX;
            values[1] += deltaX;
            changed = hovered = true;
        }
        else if (ImGui::IsItemHovered()) hovered = true;
        ImGui::PopID();

        ImGui::SetCursorScreenPos(cursor_pos + ImVec2(0, ImGui::GetTextLineHeightWithSpacing()));
        win->DrawList->AddRectFilled(
            start, end, ImGui::IsItemActive() || ImGui::IsItemHovered() ? active_color : inactive_color);

        if (values[0] > values[1])
        {
            float tmp = values[0];
            values[0] = values[1];
            values[1] = tmp;
        }
        if (!JustDeleteRegion)
        {
            int MinRegionFrame = ValueToFrame(values[0]);
            int MaxRegionFrame = ValueToFrame(values[1]);
            if (MaxRegionFrame > ClipInfo.FrameCount)
            {
                MaxRegionFrame = ClipInfo.FrameCount;
            }
            if (MinRegionFrame < 0)
            {
                MinRegionFrame = 0;
            }
            Regions[N * 2] = MinRegionFrame;
            Regions[N * 2 + 1] = MaxRegionFrame; 
        }


        if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        ImGui::NextColumn();
        return changed;
    }

    //num vrt grid lines = 5 
    void FrameExtractor::EndTimeline()
    {
        const int NumVerticalGridLines = 5;
        ImU32 timeline_running_color = IM_COL32(0, 128, 0, 200);
        const float row_height = ImGui::GetTextLineHeightWithSpacing();
        if (TimelineRows > 0)
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (TimelineRows - TimelineDisplayEnd) * row_height);
        ImGui::NextColumn();
        ImGuiWindow* win = ImGui::GetCurrentWindow();
        const float columnOffset = ImGui::GetColumnOffset(1);
        const float columnWidth = ImGui::GetColumnWidth(1) - GImGui->Style.ScrollbarSize;
        const float horizontal_interval = columnWidth / NumVerticalGridLines;

        ImU32 color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
        ImU32 line_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Border]);
        ImU32 text_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Text]);
        ImU32 moving_line_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonActive]);
        const float rounding = GImGui->Style.ScrollbarRounding;
        const float startY = ImGui::GetWindowHeight() + win->Pos.y;

        // Draw black vertical lines (inside scrolling area)
        for (int i = 1; i <= NumVerticalGridLines; ++i)
        {
            ImVec2 a = ImGui::GetWindowContentRegionMin() + win->Pos;
            a.x += i * horizontal_interval + columnOffset;
            win->DrawList->AddLine(a, ImVec2(a.x, startY), line_color);
        }

        // Draw moving vertical line
        if (PlayProgress > 0.f && PlayProgress < TimelineDisplayEnd)
        {
            ImVec2 a = ImGui::GetWindowContentRegionMin() + win->Pos;
            a.x += columnWidth * (PlayProgress / TimelineDisplayEnd) + columnOffset;
            win->DrawList->AddLine(a, ImVec2(a.x, startY), moving_line_color, 2.0f); // change thickness to 2
        }

        if (ImGui::IsWindowHovered())
        {
            if (ImGui::GetMousePos().x > (ImGui::GetWindowPos().x + columnOffset))
            {
                CheckZoomInput();
            }
        }

        ImGui::PopStyleColor();
        ImGui::EndChild();

        // Draw bottom axis ribbon (outside scrolling region)
        win = ImGui::GetCurrentWindow();
        ImVec2 start(ImGui::GetCursorScreenPos().x + columnOffset, ImGui::GetCursorScreenPos().y);
        ImVec2 end(start.x + columnWidth, start.y + row_height);
        if (PlayProgress <= 0) win->DrawList->AddRectFilled(start, end, color, rounding);
        else if (PlayProgress > TimelineDisplayEnd)
            win->DrawList->AddRectFilled(
                start, end, timeline_running_color, rounding);
        else
        {
            ImVec2 median(start.x + columnWidth * (PlayProgress / TimelineDisplayEnd), end.y);
            win->DrawList->AddRectFilled(start, median, timeline_running_color, rounding, 1 | 8);
            median.y = start.y;
            win->DrawList->AddRectFilled(median, end, color, rounding, 2 | 4);
            win->DrawList->AddLine(median, ImVec2(median.x, end.y), moving_line_color);
        }

        char tmp[256] = "";
        for (int i = 0; i < NumVerticalGridLines; ++i)
        {
            ImVec2 a = start;
            a.x += i * horizontal_interval;
            a.y = start.y;

            ImFormatString(tmp, sizeof(tmp), "%.0f",
                           (float(i) / (float)NumVerticalGridLines / TimelineZoom + TimelinePan) * ClipInfo.FrameCount);
            win->DrawList->AddText(a, text_color, tmp);
        } // use with:
        ImVec2 mouse_pos_projected_on_segment = ImLineClosestPoint(start, end, ImGui::GetMousePos());
        ImVec2 mouse_pos_delta_to_segment = mouse_pos_projected_on_segment - ImGui::GetMousePos();
        bool is_segment_hovered = (ImLengthSqr(mouse_pos_delta_to_segment) <= 10 * 10);
        if (is_segment_hovered)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsMouseClicked(0) || ImGui::IsMouseDragging(0))
            {
                IsPlaying = false;
                PlayIcon = ICON_FA_PLAY;
                PlayProgress = (ImGui::GetMousePos().x - start.x) / (end.x - start.x) * TimelineDisplayEnd;
            }
            if (ImGui::IsMouseReleased(0))
            {
                // TODO: will crash occasionally... why?
                spdlog::info("Update frame view");
                int CurrentFrameStart = int((float)ClipInfo.FrameCount / TimelineZoom * TimelinePan);
                int CurrentFrameRange = int((float)ClipInfo.FrameCount / TimelineZoom * (1 - TimelinePan));
                int CurrentFrame = int(CurrentFrameStart + PlayProgress / 100 * CurrentFrameRange - 1);
                DataBrowser::Get().LoadFrame(CurrentFrame);
            }
        }
        // draw pan-zoom control bar

        ImU32 bg_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_TextDisabled]);
        ImVec2 pz_start = {start.x, end.y + row_height};
        ImVec2 pz_end = {pz_start.x + columnWidth, pz_start.y + row_height};
        win->DrawList->AddRectFilled(pz_start, pz_end, bg_color, rounding); // bg bar
        ImVec2 pz_control_start = {pz_start.x + TimelinePan * columnWidth, pz_start.y};
        ImVec2 pz_control_end = {pz_control_start.x + (1.f / float(TimelineZoom)) * columnWidth, pz_end.y};
        win->DrawList->AddRectFilled(pz_control_start, pz_control_end, color, rounding); // control bar

        // TODO: this hover detection is too complex... there is simpler way..., make zoom able area on gray area too?
        mouse_pos_projected_on_segment = ImLineClosestPoint(pz_control_start, pz_control_end, ImGui::GetMousePos());
        mouse_pos_delta_to_segment = mouse_pos_projected_on_segment - ImGui::GetMousePos();
        is_segment_hovered = (ImLengthSqr(mouse_pos_delta_to_segment) <= 30 * 30);
        if (is_segment_hovered)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsMouseDragging(0))
            {
                TimelinePan += ImGui::GetIO().MouseDelta.x / columnWidth;
                float MaxPan = 1.f - (1.f / float(TimelineZoom));
                if (TimelinePan >= MaxPan) TimelinePan = MaxPan;
                if (TimelinePan <= 0) TimelinePan = 0;
            }
            CheckZoomInput();
        }
        ImGui::SetCursorPos(ImVec2(0, ImGui::GetCursorPosY() + row_height * 4.0f));
        if (ImGui::Button(ICON_FA_PLUS, ImVec2(96, 24)))
        {
            int CurrentFrameStart = int((float)ClipInfo.FrameCount / TimelineZoom * TimelinePan);
            int CurrentFrameRange = int((float)ClipInfo.FrameCount / TimelineZoom * (1 - TimelinePan));
            spdlog::info("frame start: {0} range: {1}... correct?", CurrentFrameStart, CurrentFrameRange);
            Regions.push_back(CurrentFrameStart + int(0.3f * (float)CurrentFrameRange));
            Regions.push_back(CurrentFrameStart + int(0.7f * (float)CurrentFrameRange));
        }
        Utils::AddSimpleTooltip("Add new region to perform frame extraction...");
        ImGui::SameLine();
        ImGui::Dummy({columnOffset - 96, 0});
        ImGui::SameLine();

    }

    void FrameExtractor::TimelineControlButtons()
    {
        if (ImGui::Button(PlayIcon, {96, 24}))
        {
            IsPlaying = !IsPlaying;
            if (IsPlaying)
            {
                PlayIcon = ICON_FA_PAUSE;
                PlayClip(); // runs an opencv subprocess?
            }
            else
            {
                PlayIcon = ICON_FA_PLAY;
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
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Reset zoom and pan for this clip");
        }
        ImGui::SameLine();
        ImGui::Dummy({200, 0});
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.f);
        int MaxExtract = 0;
        for (size_t i = 0; i < Regions.size()/2; i++)
        {
            MaxExtract += (Regions[i*2 + 1] - Regions[i*2] + 1);
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

    int FrameExtractor::ValueToFrame(const float ValueInTimeline) const
    {
        const float pan_amount = (TimelineZoom * TimelinePan * TimelineDisplayEnd);
        return int((ValueInTimeline + pan_amount) / TimelineDisplayEnd / TimelineZoom * float(ClipInfo.FrameCount));
    }

    void FrameExtractor::CheckZoomInput()
    {
        float wheel = ImGui::GetIO().MouseWheel;
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && wheel != 0.f)
        {
            TimelineZoom += wheel;
            if (TimelineZoom < 1) TimelineZoom = 1.f;
            if (TimelineZoom > 50) TimelineZoom = 50;
            TimelineDisplayEnd = 100 * TimelineZoom;
        }
    }


    // TODO: async...
    void FrameExtractor::PlayClip()
    {
    }

    void FrameExtractor::PerformExtraction()
    {
        // save extraction range?
        //// read old and override
        YAML::Node RegionToExtractData = YAML::LoadFile(
            Setting::Get().ProjectPath + std::string("/Data/ExtractionRegions.yaml"));
        RegionToExtractData[ClipInfo.ClipPath] = YAML::Node(YAML::NodeType::Sequence);
        for (int r : Regions)
            RegionToExtractData[ClipInfo.ClipPath].push_back(r);
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
            std::vector<int> v(Regions[R * 2 + 1] - Regions[R * 2] + 1);
            std::iota(v.begin(), v.end(), Regions[R * 2]);
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
