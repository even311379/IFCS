#include "Utils.h"
#include "Setting.h"

#include <fstream>
#include <iostream>
#include <istream>
#include <sstream>
#include <ctime>
#include <random>

#include "Spectrum/imgui_spectrum.h"


namespace IFCS
{
    namespace Utils
    {
        char* GetCurrentTimeString()
        {
            time_t curr_time;
            char out_string[200];

            time(&curr_time);
            tm* LocalTime = localtime(&curr_time);
            strftime(out_string, 100, "%T %Y/%m/%d", LocalTime);
            return out_string;
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

        ImVec4 RandomPickColor(bool IsLight)
        {
            std::array<ImVec4, 48> ColorOptions =
            {
                Spectrum::BLUE(400, IsLight),
                Spectrum::BLUE(500, IsLight),
                Spectrum::BLUE(600, IsLight),
                Spectrum::BLUE(700, IsLight),
                Spectrum::RED(400, IsLight),
                Spectrum::RED(500, IsLight),
                Spectrum::RED(600, IsLight),
                Spectrum::RED(700, IsLight),
                Spectrum::ORANGE(400, IsLight),
                Spectrum::ORANGE(500, IsLight),
                Spectrum::ORANGE(600, IsLight),
                Spectrum::ORANGE(700, IsLight),
                Spectrum::GREEN(400, IsLight),
                Spectrum::GREEN(500, IsLight),
                Spectrum::GREEN(600, IsLight),
                Spectrum::GREEN(700, IsLight),
                Spectrum::INDIGO(400, IsLight),
                Spectrum::INDIGO(500, IsLight),
                Spectrum::INDIGO(600, IsLight),
                Spectrum::INDIGO(700, IsLight),
                Spectrum::CELERY(400, IsLight),
                Spectrum::CELERY(500, IsLight),
                Spectrum::CELERY(600, IsLight),
                Spectrum::CELERY(700, IsLight),
                Spectrum::MAGENTA(400, IsLight),
                Spectrum::MAGENTA(500, IsLight),
                Spectrum::MAGENTA(600, IsLight),
                Spectrum::MAGENTA(700, IsLight),
                Spectrum::YELLOW(400, IsLight),
                Spectrum::YELLOW(500, IsLight),
                Spectrum::YELLOW(600, IsLight),
                Spectrum::YELLOW(700, IsLight),
                Spectrum::FUCHSIA(400, IsLight),
                Spectrum::FUCHSIA(500, IsLight),
                Spectrum::FUCHSIA(600, IsLight),
                Spectrum::FUCHSIA(700, IsLight),
                Spectrum::SEAFOAM(400, IsLight),
                Spectrum::SEAFOAM(500, IsLight),
                Spectrum::SEAFOAM(600, IsLight),
                Spectrum::SEAFOAM(700, IsLight),
                Spectrum::CHARTREUSE(400, IsLight),
                Spectrum::CHARTREUSE(500, IsLight),
                Spectrum::CHARTREUSE(600, IsLight),
                Spectrum::CHARTREUSE(700, IsLight),
                Spectrum::PURPLE(400, IsLight),
                Spectrum::PURPLE(500, IsLight),
                Spectrum::PURPLE(600, IsLight),
                Spectrum::PURPLE(700, IsLight)
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
    }
}
