#include "FrameExtractor.h"

// #include <GL/gl.h>

#include "Log.h"
#include "Setting.h"
#include "ImguiNotify/font_awesome_5.h"
#include "opencv2/opencv.hpp"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include "imgui_internal.h"
#include <shellapi.h>
#include "DataBrowser.h"

namespace IFCS
{
    void FrameExtractor::LoadFrame1AsThumbnail(const std::string& file_path)
    {
        cv::Mat frame;
        {
            cv::VideoCapture cap(file_path);
            if (!cap.isOpened())
            {
                LogPanel::Get().AddLog(ELogLevel::Error, "Fail to load video");
                return;
            }

            Resolution[0] = cap.get(cv::CAP_PROP_FRAME_WIDTH);
            Resolution[1] = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
            FrameRate = cap.get(cv::CAP_PROP_FPS);
            FrameCount = cap.get(cv::CAP_PROP_FRAME_COUNT);

            cap.set(cv::CAP_PROP_POS_FRAMES, 1);
            cap >> frame;
            cv::resize(frame, frame, cv::Size(960, 540)); // 16 : 9
            cv::cvtColor(frame, frame, cv::COLOR_BGR2RGBA);


            cap.release();
        }
        glGenTextures(1, &Thumbnail);
        glBindTexture(GL_TEXTURE_2D, Thumbnail);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        // This is required on WebGL for non power-of-two textures
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); // Same

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame.cols, frame.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame.data);
        VideoSize = {static_cast<float>(frame.cols), static_cast<float>(frame.rows)};
        spdlog::info("size: %f, %f ", VideoSize.x, VideoSize.y);
        HasLoadAnyThumbnail = true;
    }

    void FrameExtractor::RenderContent()
    {
        if (!HasLoadAnyThumbnail) return;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - VideoSize.x) * 0.5f);
        ImGui::Image((void*)(intptr_t)Thumbnail, VideoSize);
        ImGui::SetCursorPos((ImVec2{(ImGui::GetWindowWidth() - VideoSize.x) * 0.5f, VideoSize.y + 40}));
        
        if (ImGui::BeginTable("table_clip_info", 4))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::BulletText("Clip resolution");
            ImGui::TableNextColumn();
            ImGui::Text("%d X %d", Resolution[0], Resolution[1]);
            ImGui::TableNextColumn();
            ImGui::BulletText("clip length");
            ImGui::TableNextColumn();
            int minute = FrameCount / int(FrameRate) / 60;
            int sec = (FrameCount / int(FrameRate)) % 60;
            ImGui::Text("%dm%ds", minute, sec);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::BulletText("Frame rate");
            ImGui::TableNextColumn();
            ImGui::Text("%f", FrameRate);
            ImGui::TableNextColumn();
            ImGui::BulletText("frame count");
            ImGui::TableNextColumn();
            ImGui::Text("%d", FrameCount);
            ImGui::EndTable();
        }

        if (ImGui::Button("Add Extraction regions:"))
        {
            regions.push_back(20.f);
            regions.push_back(30.f);
        }
        int N_regions = regions.size() / 2;
        // if (BeginTimeline("MyTimeline", (1 - TimelinePan) / TimelineZoom * float(FrameCount), 6, N_regions))
        if (BeginTimeline("MyTimeline", 200, 500, 6, N_regions))
        // label, max_value, num_visible_rows, opt_exact_num_rows (for item culling)
        {
            for (int r = 0; r < N_regions; r++)
            {
                char buff[32];
                sprintf(buff, "Region %d", r);
                TimelineEvent(buff, &regions[r * 2]);
            }
        }
        EndTimeline(5, PlayProgress);
        TimelineControlButtons();

        // ImGui::Text("Basic Info");
    }


    // static float s_max_timeline_value = 0.f;
    // static int s_timeline_num_rows = 0;
    // static int s_timeline_display_start = 0;
    // static int s_timeline_display_end = 0;
    // static int s_timeline_display_index = 0;

    bool FrameExtractor::BeginTimeline(const char* str_id, int num_visible_rows,
                                       int opt_exact_num_rows)
    {
        // reset global variables
        s_max_timeline_value = 0.f;
        s_timeline_num_rows = s_timeline_display_start = s_timeline_display_end = 0;
        s_timeline_display_index = -1;

        if (num_visible_rows <= 0) num_visible_rows = 5;
        const float row_height = ImGui::GetTextLineHeightWithSpacing();
        const bool rv = ImGui::BeginChild(
            str_id, ImVec2(0, num_visible_rows >= 0 ? (row_height * num_visible_rows) : -1.f),
            false);
        ImGui::PushStyleColor(ImGuiCol_Border, GImGui->Style.Colors[ImGuiCol_Border]);
        ImGui::Columns(2, str_id);
        const float contentRegionWidth = ImGui::GetWindowContentRegionWidth();
        if (ImGui::GetColumnOffset(1) >= contentRegionWidth * 0.48f)
            ImGui::SetColumnOffset(
                1, contentRegionWidth * 0.15f);
        s_max_timeline_value = max_value >= 0 ? max_value : (contentRegionWidth * 0.85f);

        // TODO: is this ok to set start?
        s_timeline_display_start = min_value;

        
        if (opt_exact_num_rows > 0)
        {
            // Item culling
            s_timeline_num_rows = opt_exact_num_rows;
            ImGui::CalcListClipping(s_timeline_num_rows, row_height, &s_timeline_display_start,
                                    &s_timeline_display_end);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (s_timeline_display_start * row_height));
        }
        return rv;
    }

    static const float TIMELINE_RADIUS = 6;

    bool FrameExtractor::TimelineEvent(const char* str_id, float values[2], bool keep_range_constant)
    {
        ++s_timeline_display_index;
        if (s_timeline_num_rows > 0 &&
            (s_timeline_display_index < s_timeline_display_start || s_timeline_display_index >= s_timeline_display_end))
            return false; // item culling

        ImGuiWindow* win = ImGui::GetCurrentWindow();
        const ImU32 inactive_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
        const ImU32 active_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonHovered]);
        const ImU32 line_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Border]);
        bool changed = false;
        bool hovered = false;
        bool active = false;

        ImGui::Text("%s", str_id);
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
            pos.x += columnWidth * values[i] / s_max_timeline_value + TIMELINE_RADIUS;
            pos.y += TIMELINE_RADIUS;

            ImGui::SetCursorScreenPos(pos - ImVec2(TIMELINE_RADIUS, TIMELINE_RADIUS));
            ImGui::PushID(i);
            ImGui::InvisibleButton(str_id, ImVec2(2 * TIMELINE_RADIUS, 2 * TIMELINE_RADIUS));
            active = ImGui::IsItemActive();
            if (active || ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%f", values[i]);
                if (!keep_range_constant)
                {
                    // @meshula:The item hovered line needs to be compensated for vertical scrolling. Thx!
                    ImVec2 a(pos.x, ImGui::GetWindowContentRegionMin().y + win->Pos.y + win->Scroll.y);
                    ImVec2 b(pos.x, ImGui::GetWindowContentRegionMax().y + win->Pos.y + win->Scroll.y);
                    // possible aternative:
                    //ImVec2 a(pos.x, win->Pos.y);
                    //ImVec2 b(pos.x, win->Pos.y+win->Size.y);
                    win->DrawList->AddLine(a, b, line_color);
                }
                hovered = true;
            }
            if (active && isMouseDraggingZero)
            {
                if (!keep_range_constant) values[i] += ImGui::GetIO().MouseDelta.x / columnWidth * s_max_timeline_value;
                else mustMoveBothEnds = true;
                changed = hovered = true;
            }

            ImGui::PopID();
            win->DrawList->AddCircleFilled(
                pos, TIMELINE_RADIUS, ImGui::IsItemActive() || ImGui::IsItemHovered() ? active_color : inactive_color);
        }

        ImVec2 start = cursor_pos;
        start.x += columnWidth * values[0] / s_max_timeline_value + 2 * TIMELINE_RADIUS;
        start.y += TIMELINE_RADIUS * 0.5f;
        ImVec2 end = start + ImVec2(columnWidth * (values[1] - values[0]) / s_max_timeline_value - 2 * TIMELINE_RADIUS,
                                    TIMELINE_RADIUS);

        ImGui::PushID(-1);
        ImGui::SetCursorScreenPos(start);
        ImGui::InvisibleButton(str_id, end - start);
        if ((ImGui::IsItemActive() && isMouseDraggingZero) || mustMoveBothEnds)
        {
            const float deltaX = ImGui::GetIO().MouseDelta.x / columnWidth * s_max_timeline_value;
            values[0] += deltaX;
            values[1] += deltaX;
            changed = hovered = true;
        }
        else if (ImGui::IsItemHovered()) hovered = true;
        ImGui::PopID();

        ImGui::SetCursorScreenPos(cursor_pos + ImVec2(0, ImGui::GetTextLineHeightWithSpacing()));

        win->DrawList->AddRectFilled(start, end, ImGui::IsItemActive() || ImGui::IsItemHovered()
                                                     ? active_color
                                                     : inactive_color);

        if (values[0] > values[1])
        {
            float tmp = values[0];
            values[0] = values[1];
            values[1] = tmp;
        }
        if (values[1] > s_max_timeline_value)
        {
            values[0] -= values[1] - s_max_timeline_value;
            values[1] = s_max_timeline_value;
        }
        if (values[0] < 0)
        {
            values[1] -= values[0];
            values[0] = 0;
        }

        if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        ImGui::NextColumn();
        return changed;
    }

    void FrameExtractor::EndTimeline(int num_vertical_grid_lines, float current_time, ImU32 timeline_running_color)
    {
        const float row_height = ImGui::GetTextLineHeightWithSpacing();
        if (s_timeline_num_rows > 0)
            ImGui::SetCursorPosY(
                ImGui::GetCursorPosY() + ((s_timeline_num_rows - s_timeline_display_end) * row_height));

        ImGui::NextColumn();
        ImGuiWindow* win = ImGui::GetCurrentWindow();

        const float columnOffset = ImGui::GetColumnOffset(1);
        const float columnWidth = ImGui::GetColumnWidth(1) - GImGui->Style.ScrollbarSize;
        const float horizontal_interval = columnWidth / num_vertical_grid_lines;

        ImU32 color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
        ImU32 line_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Border]);
        ImU32 text_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Text]);
        ImU32 moving_line_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonActive]);
        const float rounding = GImGui->Style.ScrollbarRounding;
        const float startY = ImGui::GetWindowHeight() + win->Pos.y;

        // Draw black vertical lines (inside scrolling area)
        for (int i = 1; i <= num_vertical_grid_lines; ++i)
        {
            ImVec2 a = ImGui::GetWindowContentRegionMin() + win->Pos;
            a.x += i * horizontal_interval + columnOffset;
            win->DrawList->AddLine(a, ImVec2(a.x, startY), line_color);
        }

        // Draw moving vertical line
        if (current_time > 0.f && current_time < s_max_timeline_value)
        {
            ImVec2 a = ImGui::GetWindowContentRegionMin() + win->Pos;
            a.x += columnWidth * (current_time / s_max_timeline_value) + columnOffset;
            win->DrawList->AddLine(a, ImVec2(a.x, startY), moving_line_color, 2.0f); // change thickness to 2
        }

        if (ImGui::IsWindowHovered())
        {
            if (ImGui::GetMousePos().x > (ImGui::GetWindowPos().x + columnOffset))
            {
                float wheel = ImGui::GetIO().MouseWheel;
                if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && wheel != 0.f)
                {
                    TimelineZoom += wheel;
                    if (TimelineZoom < 1) TimelineZoom = 1.f;
                    if (TimelineZoom > 50) TimelineZoom = 50;
                    spdlog::info("zoom is triggered ?? {0}, wheel: {1}", TimelineZoom, wheel);
                }
            }
        }

        // ImGui::Columns(1);
        ImGui::PopStyleColor();
        ImGui::EndChild();

        // Draw bottom axis ribbon (outside scrolling region)
        win = ImGui::GetCurrentWindow();
        ImVec2 start(ImGui::GetCursorScreenPos().x + columnOffset, ImGui::GetCursorScreenPos().y);
        ImVec2 end(start.x + columnWidth, start.y + row_height);
        if (current_time <= 0) win->DrawList->AddRectFilled(start, end, color, rounding);
        else if (current_time > s_max_timeline_value)
            win->DrawList->AddRectFilled(
                start, end, timeline_running_color, rounding);
        else
        {
            ImVec2 median(start.x + columnWidth * (current_time / s_max_timeline_value), end.y);
            win->DrawList->AddRectFilled(start, median, timeline_running_color, rounding, 1 | 8);
            median.y = start.y;
            win->DrawList->AddRectFilled(median, end, color, rounding, 2 | 4);
            win->DrawList->AddLine(median, ImVec2(median.x, end.y), moving_line_color);
        }

        char tmp[256] = "";
        for (int i = 0; i < num_vertical_grid_lines; ++i)
        {
            ImVec2 a = start;
            a.x += i * horizontal_interval;
            a.y = start.y;

            ImFormatString(tmp, sizeof(tmp), "%.2f", i * s_max_timeline_value / num_vertical_grid_lines);
            win->DrawList->AddText(a, text_color, tmp);
        } // use with:
        ImVec2 mouse_pos_projected_on_segment = ImLineClosestPoint(start, end, ImGui::GetMousePos());
        ImVec2 mouse_pos_delta_to_segment = mouse_pos_projected_on_segment - ImGui::GetMousePos();
        bool is_segment_hovered = (ImLengthSqr(mouse_pos_delta_to_segment) <= 10 * 10);
        if (is_segment_hovered)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsMouseClicked(0))
            {
                spdlog::info("mouse clicked in region...");
                IsPlaying = false;
                PlayIcon = ICON_FA_PLAY;
                PlayProgress = (ImGui::GetMousePos().x - start.x) / (end.x - start.x) * 50.0f;
            }
        }

        // draw pan-zoom control bar

        ImU32 bg_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_TextDisabled]);
        ImVec2 pz_start = {start.x, end.y + row_height * 0.5f};
        ImVec2 pz_end = {pz_start.x + columnWidth, pz_start.y + row_height * 0.5f};
        win->DrawList->AddRectFilled(pz_start, pz_end, bg_color, rounding); // bg bar
        ImVec2 pz_control_start = {pz_start.x + TimelinePan, pz_start.y};
        ImVec2 pz_control_end = {
            pz_control_start.x + (columnWidth - TimelinePan) / TimelineZoom, pz_control_start.y + row_height * 0.5f
        };
        win->DrawList->AddRectFilled(pz_control_start, pz_control_end, color, rounding); // control bar
        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + columnOffset, ImGui::GetCursorPosY() + row_height * 2.5f));

        mouse_pos_projected_on_segment = ImLineClosestPoint(pz_control_start, pz_control_end, ImGui::GetMousePos());
        mouse_pos_delta_to_segment = mouse_pos_projected_on_segment - ImGui::GetMousePos();
        is_segment_hovered = (ImLengthSqr(mouse_pos_delta_to_segment) <= 10 * 10);
        if (is_segment_hovered)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            spdlog::info("hover on control bar region...");
            if (ImGui::IsMouseDragging(0))
            {
                TimelinePan += ImGui::GetIO().MouseDelta.x / columnWidth * s_max_timeline_value;
                // else mustMoveBothEnds = true;
                // changed = hovered = true;
                // if (ImGui::IsMouseClicked(0))
                // {
                //     IsPlaying = false;
                //     PlayIcon = ICON_FA_PLAY;
                //     PlayProgress = (ImGui::GetMousePos().x - start.x) / (end.x - start.x) * 50.0f;
                // }
            }
        }
    }

    void FrameExtractor::TimelineControlButtons()
    {
        if (ImGui::Button(PlayIcon, {96, 24}))
        {
            IsPlaying = !IsPlaying;
            if (IsPlaying)
                PlayIcon = ICON_FA_PAUSE;
            else
            {
                PlayIcon = ICON_FA_PLAY;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_SYNC, {96, 24}))
        {
            TimelinePan = 0.0f;
            TimelineZoom = 1.0f;
        }
    }
}
