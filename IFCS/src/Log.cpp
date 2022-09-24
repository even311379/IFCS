#include "Log.h"
#include "imgui.h"
#include <set>
#include "Utils.h"
#include "Spectrum/imgui_spectrum.h"
#include "spdlog/spdlog.h"

namespace IFCS
{
    void LogPanel::ClearDisplay()
    {
        for (FLogData& D : Data)
        {
            D.ShouldDisplay = false;
        }
    }

    void LogPanel::FilterData()
    {
        std::set<ELogLevel> FilterLevel;
        if (bShowInfo) FilterLevel.insert(ELogLevel::Info);
        if (bShowWarning) FilterLevel.insert(ELogLevel::Warning);
        if (bShowError) FilterLevel.insert(ELogLevel::Error);
        for (FLogData& D : Data)
        {
            D.ShouldDisplay = true;
            if (FilterLevel.count(D.LogLevel) == 0)
            {
                D.ShouldDisplay = false;
                continue;
            }
            if (strlen(sFilterMessage) == 0) continue;
            if (D.Message.find(std::string(sFilterMessage)) == std::string::npos)
            {
                D.ShouldDisplay = false;
            }
        }
    }

    void LogPanel::AddLog(ELogLevel Level, const char* Message)
    {
        Data.push_back({Level, Utils::GetCurrentTimeString(), Message, true});
    }

    void LogPanel::ClearData()
    {
        Data.clear();
    }

    void LogPanel::RenderContent()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Spectrum::CHECKBOX_BORDER_SIZE);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Spectrum::CHECKBOX_ROUNDING);
        ImGui::PushStyleColor(ImGuiCol_Text, Spectrum::BLUE(500));
        if (ImGui::Checkbox("INFO", &bShowInfo))
            FilterData();
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, Spectrum::YELLOW(500));
        if (ImGui::Checkbox("WARNING", &bShowWarning))
            FilterData();
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, Spectrum::RED(500));
        if (ImGui::Checkbox("ERROR", &bShowError))
            FilterData();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        ImGui::SameLine();
        ImGui::PushItemWidth(ImGui::GetFontSize() * 24);
        if (ImGui::InputText("Filter message", sFilterMessage, IM_ARRAYSIZE(sFilterMessage)))
        {
            FilterData();
        }
        ImGui::PopItemWidth();
        ImGui::Separator();
        for (FLogData& D : Data)
        {
            if (!D.ShouldDisplay) continue;
            char buff[300];
            // TODO: where to query theme? 
            switch (D.LogLevel) {
            case ELogLevel::Info:
                    snprintf(buff, sizeof(buff), "[INFO    %s] %s", D.TimeString.c_str(), D.Message.c_str());
                    ImGui::Text(buff);
                    break;
            case ELogLevel::Warning:
                    snprintf(buff, sizeof(buff), "[WARNING %s] %s", D.TimeString.c_str(), D.Message.c_str());
                    ImGui::TextColored(Spectrum::YELLOW(500), buff);
                    break;
            case ELogLevel::Error:
                    snprintf(buff, sizeof(buff), "[ERROR   %s] %s", D.TimeString.c_str(), D.Message.c_str());
                    ImGui::TextColored(Spectrum::RED(500), buff);
                    break;
            }
            
        }
    }
}
