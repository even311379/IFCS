#include "Prediction.h"

#include "imgui_internal.h"
#include "Setting.h"
#include "ImguiNotify/font_awesome_5.h"
#include "Spectrum/imgui_spectrum.h"
#include "Implot/implot.h"

namespace IFCS
{
    void Prediction::RenderContent()
    {
        if (ImGui::TreeNodeEx("Make Prediction", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const char* FakeModels[] = {
                "model1", "model2", "model3"
            };
            const char* FakeClips[] = {
                "Clip1", "Clip2", "Clip3"
            };
            ImGui::Combo("Choose Model", &SelectedModelIdx, FakeModels, IM_ARRAYSIZE(FakeModels));
            ImGui::Combo("Choose Clip", &SelectedClipIdx, FakeClips, IM_ARRAYSIZE(FakeClips));
            ImGui::DragFloat("Confidence", &Confidence, 0.05f, 0.05f, 1.0f, "%.2f");
            if (ImGui::Button("Predict"))
            {
                MakePrediction();
            }
            ImGui::TreePop();
        }
        // choose model
        // choose clip
        // run & save as prediction ... then move the result to project folder?

        // choose prediction
        const char* Predictions[] = {
            "Model1 - Clip1 - Conf50",
            "Model1 - Clip2 - Conf40",
            "Model2 - Clip1 - Conf25",
        };

        static float FakeProportion[] = {0.3f, 0.1f, 1.0f, 0.6f, 0.85f};
        static int FakeSum[] = {2018, 673, 6726, 4036, 5717};
        if (ImGui::TreeNodeEx("Prediction Analysis", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Combo("Choose Prediction", &SelectedPrediction, Predictions, IM_ARRAYSIZE(Predictions));

            if (ImGui::Button(PlayIcon, {96, 32}))
            {
                IsPlaying = !IsPlaying;
                if (IsPlaying)
                    PlayIcon = ICON_FA_PAUSE;
                else
                    PlayIcon = ICON_FA_PLAY;
            }
            ImGui::SameLine();
            DrawPlayRange();
            // specify line
            ImGui::Text("Set Fish way start and end:");
            ImGui::DragFloat2("Start P1", FishWayStartP1, 0.01f, 0, 1);
            ImGui::SameLine();
            ImGui::DragFloat2("Start P2", FishWayStartP2, 0.01f, 0, 1);
            ImGui::DragFloat2("End P1", FishWayEndP1, 0.01f, 0, 1);
            ImGui::SameLine();
            ImGui::DragFloat2("End P2", FishWayEndP2, 0.01f, 0, 1);
            ImGui::DragInt("Per Unit Frame Size", &PUFS, 1, 1, TotalClipFrameSize);

            static ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;
            if (ImGui::TreeNode("Number of fish inside fish way"))
            {
                if (ImGui::BeginTable("##Table1", 3, flags, ImVec2(-1, 0)))
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
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Number of fish pass fish way"))
            {
                if (ImGui::BeginTable("##Table2", 3, flags, ImVec2(-1, 0)))
                {
                    ImGui::TableSetupColumn("Group", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                    ImGui::TableSetupColumn("Total Sum", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                    ImGui::TableSetupColumn("Accumulated sum");
                    ImGui::TableHeadersRow();
                    if (IsPlaying)
                        PlayOffset = (PlayOffset + 1) % TotalClipFrameSize; // speed control? ... need to slow down... 
                    for (size_t row = 0; row < Results.size(); row++)
                    {
                        const char* id = Results[row].CategoryDisplayName.c_str();
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text(id);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%d", FakeSum[row]);
                        ImGui::TableSetColumnIndex(2);
                        char buf[32];
                        sprintf(buf, "%d", FakeSum[row]);
                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Results[row].Color);
                        ImGui::ProgressBar(FakeProportion[row], ImVec2(-1, 0), buf);
                        ImGui::PopStyleColor();
                    }
                    ImGui::EndTable();
                }
                ImGui::TreePop();
            }
        }
        // TODO: add save as png? or other format?
    }

    void Prediction::MakePrediction()
    {
        // trigger python code?
    }

    void Prediction::DrawPlayRange()
    {
        ImGuiWindow* Win = ImGui::GetCurrentWindow();
        ImVec2 CurrentPos = ImGui::GetCursorScreenPos();
        ImU32 Color = ImGui::ColorConvertFloat4ToU32(Spectrum::BLUE(600, Setting::Get().Theme == ETheme::Light));
        const float AvailWidth = ImGui::GetContentRegionAvail().x * 0.95f;
        ImVec2 RectStart = CurrentPos + ImVec2(0, 17);
        ImVec2 RectEnd = CurrentPos + ImVec2(AvailWidth, 22);
        Win->DrawList->AddRectFilled(RectStart, RectEnd, Color, 20.f);
        const ImVec2 ControlPos1 = CurrentPos + ImVec2(PlayTimePos1 * AvailWidth - 6.f, 5);
        Win->DrawList->AddRectFilled(ControlPos1, ControlPos1 + ImVec2(12.f, 25), Color, 6.f);
        ImGui::SetCursorScreenPos(ControlPos1);
        ImGui::InvisibleButton("PlayTimeControlMin", ImVec2(12.f, 20));
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (ImGui::IsMouseDragging(0) && ImGui::IsItemActive())
        {
            PlayTimePos1 += ImGui::GetIO().MouseDelta.x / AvailWidth;
            PlayTimePos1 = Utils::Round(PlayTimePos1, 2);
            if (PlayTimePos1 <= 0) PlayTimePos1 = 0;
            if (PlayTimePos1 >= 1) PlayTimePos1 = 1;
            if (PlayTimePos1 > PlayTimePos2)
            {
                const float temp = PlayTimePos2;
                PlayTimePos2 = PlayTimePos1;
                PlayTimePos1 = temp;
            }
        }
        const ImVec2 ControlPos2 = CurrentPos + ImVec2(PlayTimePos2 * AvailWidth - 6.f, 5);
        Win->DrawList->AddRectFilled(ControlPos2, ControlPos2 + ImVec2(12.f, 25), Color, 6.f);
        ImGui::SetCursorScreenPos(ControlPos2);
        ImGui::InvisibleButton("PlayTimeControlMax", ImVec2(12.f, 20));
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (ImGui::IsMouseDragging(0) && ImGui::IsItemActive())
        {
            PlayTimePos2 += ImGui::GetIO().MouseDelta.x / AvailWidth;
            PlayTimePos2 = Utils::Round(PlayTimePos2, 2);
            if (PlayTimePos2 <= 0) PlayTimePos2 = 0;
            if (PlayTimePos2 >= 1) PlayTimePos2 = 1;
            if (PlayTimePos1 > PlayTimePos2)
            {
                const float temp = PlayTimePos2;
                PlayTimePos2 = PlayTimePos1;
                PlayTimePos1 = temp;
            }
        }
    }

    // implement analysis of predictions
    void Prediction::Analysis()
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
    void Prediction::Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose)
    {
        Panel::Setup(InName, InShouldOpen, InFlags, InCanClose);
        Analysis();
    }
}
