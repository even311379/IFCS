#include "Utils.h"
#include "Setting.h"

#include <ctime>
#include <fstream>
#include <istream>
#include <sstream>
#include <random>
#include <locale>
#include <codecvt>

#include "imgui_internal.h"
#include "Style.h"


// #define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

namespace IFCS
{
    namespace Utils
    {
        std::string GetCurrentTimeString(bool IsStyled, bool IsFileName)
        {
            time_t rawtime;
            tm* timeinfo;
            char buffer[80];
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            if (IsFileName)
                strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", timeinfo);
            else
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

        bool InsensitiveStringCompare(const std::string& InStr1, const std::string& InStr2)
        {
            return std::equal(InStr1.begin(), InStr1.end(),
                              InStr2.begin(), InStr2.end(),
                              [](char a, char b)
                              {
                                  return tolower(a) == tolower(b);
                              });
        }

        // TODO: fixed pattern that path will never end with "/" !!!
        std::string ChangePathSlash(const std::string& InString)
        {
            std::string Temp = InString;
            std::replace(Temp.begin(), Temp.end(), '\\', '/');
            return Temp;
        }

        // std::wstring ToWString(const std::string& InString)
        // {
        //     // Solution to convert string to wstring: https://stackoverflow.com/a/18597384
        //     std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        //     return converter.from_bytes(InString);
        // }

        // std::wstring ToWString(const char* InChars)
        // {
        //     return ToWString(std::string(InChars));
        // }

        std::string ConvertWideToANSI(const std::wstring& wstr)
        {
            int count = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), int(wstr.length()), NULL, 0, NULL, NULL);
            std::string str(count, 0);
            WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &str[0], count, NULL, NULL);
            return str;
        }

        std::wstring ConvertAnsiToWide(const std::string& str)
        {
            int count = MultiByteToWideChar(CP_ACP, 0, str.c_str(), int(str.length()), NULL, 0);
            std::wstring wstr(count, 0);
            MultiByteToWideChar(CP_ACP, 0, str.c_str(), int(str.length()), &wstr[0], count);
            return wstr;
        }

        std::string ConvertWideToUtf8(const std::wstring& wstr)
        {
            int count = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), int(wstr.length()), NULL, 0, NULL, NULL);
            std::string str(count, 0);
            WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], count, NULL, NULL);
            return str;
        }

        std::wstring ConvertUtf8ToWide(const std::string& str)
        {
            int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), int(str.length()), NULL, 0);
            std::wstring wstr(count, 0);
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), int(str.length()), &wstr[0], count);
            return wstr;
        }

        void AddSimpleTooltip(const char* Desc, float WrapSize, float Offset)
        {
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            {
                if (Offset != 0.f)
                {
                    ImVec2 TooltipPos = ImGui::GetIO().MousePos;
                    TooltipPos.x += Offset;
                    ImGui::SetNextWindowPos(TooltipPos);
                }
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * WrapSize);
                ImGui::Text(Desc);
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }

        bool SelectableInput(const char* str_id, bool selected, ImGuiSelectableFlags flags, char* buf, size_t buf_size)
        {
            ImGuiContext& g = *GImGui;
            ImGuiWindow* window = g.CurrentWindow;
            ImVec2 pos_before = window->DC.CursorPos;
            // ImVec2 pos_before = ImGui::GetCursorPos();

            ImGui::PushID(str_id);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                                ImVec2(g.Style.ItemSpacing.x, g.Style.FramePadding.y * 2.0f));
            bool ret = ImGui::Selectable("##Selectable", selected,
                                         flags | ImGuiSelectableFlags_AllowDoubleClick |
                                         ImGuiSelectableFlags_AllowItemOverlap);
            ImGui::PopStyleVar();

            ImGuiID id = ImGui::GetID("##Input");
            bool temp_input_is_active = ImGui::TempInputIsActive(id);
            bool temp_input_start = ret ? ImGui::IsMouseDoubleClicked(0) : false;

            if (temp_input_start)
                ImGui::SetActiveID(id, ImGui::GetCurrentWindow());

            if (temp_input_is_active || temp_input_start)
            {
                ImVec2 pos_after = window->DC.CursorPos;
                // ImVec2 pos_after = ImGui::GetCursorPos();
                window->DC.CursorPos = pos_before;
                ImRect ItemRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
                ret = ImGui::TempInputText(ItemRect, id, "##Input", buf, (int)buf_size,
                                           ImGuiInputTextFlags_None);
                window->DC.CursorPos = pos_after;
            }
            else
            {
                ImGui::GetWindowDrawList()->AddText(pos_before, ImGui::GetColorU32(ImGuiCol_Text), buf);
            }

            ImGui::PopID();
            return ret;
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

        float Distance(float X1, float Y1, float X2, float Y2)
        {
            return (float)std::pow(std::pow(X1 - X2, 2) + std::pow(Y1 - Y2, 2), 0.5);
        }

        void GetAbsRectMinMax(ImVec2 p0, ImVec2 p1, ImVec2& OutMin, ImVec2& OutMax)
        {
            OutMin.x = std::min(p0.x, p1.x);
            OutMin.y = std::min(p0.y, p1.y);
            OutMax.x = std::max(p0.x, p1.x);
            OutMax.y = std::max(p0.y, p1.y);
        }

        bool IsPointInsideRect(const ImVec2& ToCheck, const ImVec2& RectP0, const ImVec2& RectP1)
        {
            ImVec2 AbsMin, AbsMax;
            GetAbsRectMinMax(RectP0, RectP1, AbsMin, AbsMax);
            return ToCheck.x >= AbsMin.x && ToCheck.x <= AbsMax.x && ToCheck.y >= AbsMin.y && ToCheck.y <= AbsMax.y;
        }
    }
}
