# pragma once
#include <ctime>
#include <fstream>
#include <iostream>
#include <istream>
#include <sstream>
#include <string>
#include <spdlog/spdlog.h>
#include "Common.h"
#include "Setting.h"

/*
 * TODO: as long as I convert string to c_str() inside this utility function rather then in ImGui implementation...
 * the encoding just fucked up... no way to fix it...
 */

namespace IFCS
{
    namespace Utils 
    {
        static char* GetCurrentTimeString()
        {
            time_t curr_time;
            char out_string[200];

            time(&curr_time);
            tm* LocalTime = localtime(&curr_time);
            strftime(out_string, 100, "%T %Y/%m/%d", LocalTime);
            return out_string;
        }

        typedef std::vector<std::vector<std::string>> CSV;

        static CSV LoadCSV(const char* FilePath, bool bContainsHeader = true)
        {
            CSV Out;
            std::ifstream infile(FilePath);
            std::string line;
            int line_num = 0;
            while (std::getline(infile, line))
            {
                if (bContainsHeader && line_num == 0)
                {
                    line_num++;
                    continue;
                }
                std::vector<std::string> Row;
                std::stringstream sline(line);
                std::string item;
                while (std::getline(sline, item, ','))
                {
                    Row.push_back(item);
                }
                Out.push_back(Row);
                line_num ++;
            }
            return Out;
        }


        static const std::string GetLocText(const char* LocID, ESupportedLanguage language)
        {
            CSV data = LoadCSV("Resources/localization.csv", true);
            for (auto& row : data)
            {
                if (row.size() == 0) continue;
                if (row[0] == LocID)
                {
                    switch (language)
                    {
                    case ESupportedLanguage::English:
                        return row[1];
                    case ESupportedLanguage::TraditionalChinese:
                        return row[2];
                    }
                }
            }
            assert(0 && "key missing in localized text");
            return "";
        }

        static const std::string GetLocText(const char* LocID)
        {
            Setting& s = Setting::Get();
            return GetLocText(LocID, s.PreferredLanguage);
        }


        static void AddSimpleTooltip(const char* Desc, float WrapSize = 35.0f)
        {
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * WrapSize);
                ImGui::Text(Desc);
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }
        
        template<typename C, typename T>
        static bool Contains(C&& c, T e) { 
            return std::find(std::begin(c), std::end(c), e) != std::end(c);
        };
    }
}
