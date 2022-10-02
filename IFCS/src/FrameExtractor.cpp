#include "FrameExtractor.h"

// #include <GL/gl.h>

#include "Log.h"
#include "Setting.h"
#include "Utils.h"

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
        ClipPath = file_path;
        cv::Mat frame;
        {
            cv::VideoCapture cap(file_path);
            if (!cap.isOpened())
            {
                LogPanel::Get().AddLog(ELogLevel::Error, "Fail to load video");
                return;
            }
            std::vector<std::string> temp = Utils::Split(file_path, '\\');
            ClipName = temp.back();
            Resolution[0] = cap.get(cv::CAP_PROP_FRAME_WIDTH);
            Resolution[1] = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
            FrameRate = cap.get(cv::CAP_PROP_FPS);
            FrameCount = cap.get(cv::CAP_PROP_FRAME_COUNT);
            float i = 1.0f;
            while (FrameCount > pow(10, i))
            {
                i += 1.0f;
            }
            FrameCountDigits = int(i) - 1;

            cap.set(cv::CAP_PROP_POS_FRAMES, 1);
            cap >> frame;
            cv::resize(frame, frame, cv::Size(960, 540)); // 16 : 9
            cv::cvtColor(frame, frame, cv::COLOR_BGR2RGBA);


            cap.release();
        }
        UpdateThumbnailFromFrame(frame);
        VideoSize = {static_cast<float>(frame.cols), static_cast<float>(frame.rows)};
        HasLoadAnyThumbnail = true;
    }

    void FrameExtractor::RenderContent()
    {
        if (!HasLoadAnyThumbnail) return;
        // so tedious to set strings good... c++ rocks....
        float i = 1.0f;
        while (CurrentFrame > pow(10, i))
        {
            i += 1.0f;
        }
        int CurrentFrameDigits = int(i) - 1;
        std::string temp = std::to_string(CurrentFrame);
        for (int i = 0; i < FrameCountDigits - CurrentFrameDigits; i++)
        {
            temp = std::string("0") + temp;
        }
        std::string Title = ClipName + "......(" + temp + "/" + std::to_string(FrameCount) + ")";
        ImGui::PushFont(Setting::Get().TitleFont);
        float TitleWidth = ImGui::CalcTextSize(Title.c_str()).x;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - TitleWidth) * 0.5f);
        ImGui::Text(Title.c_str());
        ImGui::PopFont();
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - VideoSize.x) * 0.5f);
        ImGui::Image((void*)(intptr_t)Thumbnail, VideoSize);
        // ImGui::SetCursorPos((ImVec2{(ImGui::GetWindowWidth() - VideoSize.x) * 0.5f, VideoSize.y + 40}));
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - VideoSize.x) * 0.5f);

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


        // if (BeginTimeline("MyTimeline", (1 - TimelinePan) / TimelineZoom * float(FrameCount), 6, N_regions))
        if (BeginTimeline(regions.size() / 2))
        // label, max_value, num_visible_rows, opt_exact_num_rows (for item culling)
        {
            for (int r = 0; r < regions.size() / 2; r++)
            {
                TimelineEvent(r);
            }
        }
        EndTimeline();
        TimelineControlButtons();

        // ImGui::Text("Basic Info");
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

        // if (opt_exact_num_rows > 0)
        // {
        //     // Item culling
        //     TimelineRows = opt_exact_num_rows;
        //     ImGui::CalcListClipping(TimelineRows, row_height, &TimelineDisplayStart, &TimelineDisplayEnd);
        //     ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (TimelineDisplayStart * row_height));
        // }
        return rv;
    }


    bool FrameExtractor::TimelineEvent(int N)
    {
        char id[64];
        snprintf(id, sizeof(id), "region %d", N);
        float values[2];
        float pan_amount = (TimelineZoom * TimelinePan * TimelineDisplayEnd);
        values[0] = float(regions[N*2]) / float(FrameCount) * TimelineZoom * TimelineDisplayEnd - pan_amount;
        values[1] = float(regions[N*2+1]) / float(FrameCount) * TimelineZoom * TimelineDisplayEnd - pan_amount;

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
                regions.erase(regions.begin() + 2*N);
                regions.erase(regions.begin() + 2*N); // do it twice with same index since it will shrink?
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
                ImGui::SetTooltip("%f", values[i]);
                {
                    ImVec2 a(pos.x, ImGui::GetWindowContentRegionMin().y + win->Pos.y + win->Scroll.y);
                    ImVec2 b(pos.x, ImGui::GetWindowContentRegionMax().y + win->Pos.y + win->Scroll.y);
                    win->DrawList->AddLine(a, b, line_color);
                }
                hovered = true;
            }
            if (active && isMouseDraggingZero)
            {
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
            regions[N*2] = (values[0] + pan_amount) / TimelineDisplayEnd / TimelineZoom * float(FrameCount);
            regions[N*2+1] = (values[1] + pan_amount) / TimelineDisplayEnd / TimelineZoom * float(FrameCount);
        }

        // TODO: clip end considering the zoom and pan
        // if (values[1] > (float)TimelineDisplayEnd)
        // {
        //     values[0] -= values[1] - (float)TimelineDisplayEnd;
        //     values[1] = (float)TimelineDisplayEnd;
        // }
        // if (values[0] < 0)
        // {
        //     values[1] -= values[0];
        //     values[0] = 0;
        // }

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

        // ImGui::Columns(1);
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
                           (float(i) / (float)NumVerticalGridLines / TimelineZoom + TimelinePan) * FrameCount);
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
                spdlog::info("mouse clicked in region...");
            }
            if (ImGui::IsMouseReleased(0))
            {
                spdlog::info("mouse released after move progress?? ... should update video frame?");
                UpdateVideoFrame();
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
            int CurrentFrameStart = FrameCount / TimelineZoom * TimelinePan;
            int CurrentFrameRange = FrameCount / TimelineZoom * (1 - TimelinePan);
            spdlog::info("frame start: {0} range: {1}... correct?", CurrentFrameStart, CurrentFrameRange);
            regions.push_back(CurrentFrameStart + int(0.3f * (float)CurrentFrameRange));
            regions.push_back(CurrentFrameStart + int(0.7f * (float)CurrentFrameRange));
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Add new region to perform frame extraction...");
        }
        ImGui::SameLine();
        ImGui::Dummy({columnOffset - 96, 0});
        ImGui::SameLine();
        
        // ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + columnOffset, ImGui::GetCursorPosY() + row_height * 3.5f));
        // ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + columnOffset, ImGui::GetCursorPosY() - row_height));
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
        ImGui::InputInt("frames to ", &NumFramesToExtract);
        ImGui::SameLine();
        if (ImGui::Button("Extract"))
        {
            spdlog::info("finally...");
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.f);

        // TODO: think of a better approach ...
        //ExtractionOptions = Utils::GetLocText("") + "\0" +Utils::GetLocText("") + "\0" +Utils::GetLocText("") + "\0\0";
        // ExtractionOptions = "re-extract\0delete_all\0hey\0\0";
        ExtractionOptions = "aaaa\0bbbb\0cccc\0dddd\0eeee\0\0";
        ImGui::Combo("##ExtractionOptionsCombo", &SelectedExtractionOption, "aaaa\0bbbb\0cccc\0dddd\0eeee\0\0");
// add combo to choose extract modes?        ImGui::Combo();
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

    void FrameExtractor::UpdateThumbnailFromFrame(cv::Mat mat)
    {
        Thumbnail = 0;
        glGenTextures(1, &Thumbnail);
        glBindTexture(GL_TEXTURE_2D, Thumbnail);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        // This is required on WebGL for non power-of-two textures
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); // Same
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mat.cols, mat.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, mat.data);
    }

    void FrameExtractor::UpdateVideoFrame()
    {
        // make PlayProgress to current frame
        int CurrentFrameStart = FrameCount / TimelineZoom * TimelinePan;
        int CurrentFrameRange = FrameCount / TimelineZoom * (1 - TimelinePan);
        CurrentFrame = CurrentFrameStart + PlayProgress / 100 * CurrentFrameRange - 1;
        spdlog::info("current frame: {0}", CurrentFrame);
        // update texture
        cv::Mat frame;
        {
            cv::VideoCapture cap(ClipPath);
            if (!cap.isOpened())
            {
                LogPanel::Get().AddLog(ELogLevel::Error, "Fail to load video");
                return;
            }
            cap.set(cv::CAP_PROP_POS_FRAMES, CurrentFrame);
            cap >> frame;
            resize(frame, frame, cv::Size(960, 540)); // 16 : 9
            cvtColor(frame, frame, cv::COLOR_BGR2RGBA);
            cap.release();
        }
        UpdateThumbnailFromFrame(frame);
    }

    // TODO: async...
    void FrameExtractor::PlayClip()
    {
    }
    // TODO: grab  combo options...
    void FrameExtractor::PerformExtraction()
    {
    }
}
