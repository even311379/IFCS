#include "Utils.h"
#include "Setting.h"

#include <ctime>
#include <fstream>
#include <chrono>
#include <iostream>
#include <istream>
#include <sstream>
#include <random>

#include "Style.h"


namespace IFCS
{
    namespace Utils
    {
        // char* GetCurrentTimeString()
        // {
        //     time_t curr_time;
        //     char out_string[200];
        //
        //     time(&curr_time);
        //     tm* LocalTime = localtime(&curr_time);
        //     strftime(out_string, 100, "%T %Y/%m/%d", LocalTime);
        //     return out_string;
        // }

        std::string GetCurrentTimeString(bool IsStyled)
        {
            time_t rawtime;
            tm* timeinfo;
            char buffer[80];
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", timeinfo);
            if (IsStyled)
                return "[" + std::string{buffer} + "]";
            return std::string{buffer};
        }

        CSV LoadCSV(const char* FilePath, bool bContainsHeader)
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
                while (std::getline(sline, item, ';'))
                {
                    Row.push_back(item);
                }
                Out.push_back(Row);
                line_num ++;
            }
            return Out;
        }

        const std::string GetLocText(const char* LocID, ESupportedLanguage language)
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

        const std::string GetLocText(const char* LocID)
        {
            Setting& s = Setting::Get();
            return GetLocText(LocID, s.PreferredLanguage);
        }

        void AddSimpleTooltip(const char* Desc, float WrapSize)
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

        std::vector<std::string> Split(const std::string& str, const char& delimiter)
        {
            std::vector<std::string> result;
            std::stringstream ss(str);
            std::string tok;

            while (std::getline(ss, tok, delimiter))
            {
                result.push_back(tok);
            }
            return result;
        }

        ImVec4 RandomPickColor(ETheme ActiveTheme)
        {
            std::array<ImVec4, 48> ColorOptions =
            {
                Style::BLUE(400, ActiveTheme),
                Style::BLUE(500, ActiveTheme),
                Style::BLUE(600, ActiveTheme),
                Style::BLUE(700, ActiveTheme),
                Style::RED(400, ActiveTheme),
                Style::RED(500, ActiveTheme),
                Style::RED(600, ActiveTheme),
                Style::RED(700, ActiveTheme),
                Style::ORANGE(400, ActiveTheme),
                Style::ORANGE(500, ActiveTheme),
                Style::ORANGE(600, ActiveTheme),
                Style::ORANGE(700, ActiveTheme),
                Style::GREEN(400, ActiveTheme),
                Style::GREEN(500, ActiveTheme),
                Style::GREEN(600, ActiveTheme),
                Style::GREEN(700, ActiveTheme),
                Style::INDIGO(400, ActiveTheme),
                Style::INDIGO(500, ActiveTheme),
                Style::INDIGO(600, ActiveTheme),
                Style::INDIGO(700, ActiveTheme),
                Style::CELERY(400, ActiveTheme),
                Style::CELERY(500, ActiveTheme),
                Style::CELERY(600, ActiveTheme),
                Style::CELERY(700, ActiveTheme),
                Style::MAGENTA(400, ActiveTheme),
                Style::MAGENTA(500, ActiveTheme),
                Style::MAGENTA(600, ActiveTheme),
                Style::MAGENTA(700, ActiveTheme),
                Style::YELLOW(400, ActiveTheme),
                Style::YELLOW(500, ActiveTheme),
                Style::YELLOW(600, ActiveTheme),
                Style::YELLOW(700, ActiveTheme),
                Style::FUCHSIA(400, ActiveTheme),
                Style::FUCHSIA(500, ActiveTheme),
                Style::FUCHSIA(600, ActiveTheme),
                Style::FUCHSIA(700, ActiveTheme),
                Style::SEAFOAM(400, ActiveTheme),
                Style::SEAFOAM(500, ActiveTheme),
                Style::SEAFOAM(600, ActiveTheme),
                Style::SEAFOAM(700, ActiveTheme),
                Style::CHARTREUSE(400, ActiveTheme),
                Style::CHARTREUSE(500, ActiveTheme),
                Style::CHARTREUSE(600, ActiveTheme),
                Style::CHARTREUSE(700, ActiveTheme),
                Style::PURPLE(400, ActiveTheme),
                Style::PURPLE(500, ActiveTheme),
                Style::PURPLE(600, ActiveTheme),
                Style::PURPLE(700, ActiveTheme)
            };
            std::mt19937_64 gen{std::random_device{}()}; // generates random numbers
            std::uniform_int_distribution<std::size_t> dist(0, ColorOptions.size() - 1);
            return ColorOptions[dist(gen)];
        }

        // copy from https://stackoverflow.com/a/7560564/5579437
        int RandomIntInRange(int Min, int Max)
        {
            std::random_device rd; // obtain a random number from hardware
            std::mt19937 gen(rd()); // seed the generator
            std::uniform_int_distribution<> distr(Min, Max); // define the range
            return distr(gen);
        }

        bool RandomBool()
        {
            return RandomIntInRange(0, 1);
        }

        auto Round(const float& InValue, const int& N_Digits) -> float
        {
            const float v = std::nearbyint(InValue * static_cast<float>(std::pow(10.f, N_Digits)));
            return v / static_cast<float>(std::pow(10, N_Digits));
        }

        bool FloatCompare(float A, float B, float Tolerance)
        {
            return (std::fabs(A - B) < Tolerance);
        }

        // float Mean(const std::vector<int>& InData)
        // {
        //     int Sum = 0;
        //     for (const auto& i : InData)
        //         Sum += i;
        //     return (float)Sum / (float)InData.size();
        // }
        //
        // int Max(const std::vector<int>& InData)
        // {
        //     int out = -99999;
        //     for (const int i : InData)
        //     {
        //         if (i > out) out = i;
        //     }
        //     return out;
        // }

        float Distance(float X1, float Y1, float X2, float Y2)
        {
            return (float)std::pow(std::pow(X1-X2, 2 ) + std::pow(Y1-Y2, 2), 0.5);
        }
    }
}
