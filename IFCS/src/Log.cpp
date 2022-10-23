#include "Log.h"
#include "imgui.h"
#include <set>
#include "Utils.h"
#include "Style.h"
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

    // TODO: add format log?
    void LogPanel::AddLog(ELogLevel Level, const char* Message)
    {
        Data.push_back({Level, Utils::GetCurrentTimeString(), Message, true});
        switch (Level)
        {
        case ELogLevel::Info: spdlog::info(Message);
            break;
        case ELogLevel::Warning: spdlog::warn(Message);
            break;
        case ELogLevel::Error: spdlog::error(Message);
            break;
        }
    }

    void LogPanel::ClearData()
    {
        Data.clear();
    }

    void LogPanel::RenderContent()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::CHECKBOX_BORDER_SIZE);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Style::CHECKBOX_ROUNDING);
        ImGui::PushStyleColor(ImGuiCol_Text, Style::BLUE(500));
        if (ImGui::Checkbox("INFO", &bShowInfo))
            FilterData();
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, Style::YELLOW(500));
        if (ImGui::Checkbox("WARNING", &bShowWarning))
            FilterData();
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, Style::RED(500));
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
            switch (D.LogLevel)
            {
            case ELogLevel::Info:
                snprintf(buff, sizeof(buff), "[INFO    %s] %s", D.TimeString.c_str(), D.Message.c_str());
                ImGui::Text(buff);
                break;
            case ELogLevel::Warning:
                snprintf(buff, sizeof(buff), "[WARNING %s] %s", D.TimeString.c_str(), D.Message.c_str());
                ImGui::TextColored(Style::YELLOW(500), buff);
                break;
            case ELogLevel::Error:
                snprintf(buff, sizeof(buff), "[ERROR   %s] %s", D.TimeString.c_str(), D.Message.c_str());
                ImGui::TextColored(Style::RED(500), buff);
                break;
            }
        }
    }
}


/*
 * TODO: make this work in my code bank...
 *
// log to str and return it
template<typename... Args>
static std::string log_to_str(const std::string &msg, const Args &... args)
{
    std::ostringstream oss;
    auto oss_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    spdlog::logger oss_logger("pattern_tester", oss_sink);
    oss_logger.set_level(spdlog::level::info);

    oss_logger.set_formatter(std::unique_ptr<spdlog::formatter>(new spdlog::pattern_formatter(args...)));

    oss_logger.info(msg);
    return oss.str();
}*/