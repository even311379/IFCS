# pragma once


#include "IFCS_Types.h"

namespace IFCS
{
    namespace Utils
    {
        std::string GetCurrentTimeString(bool IsStyled = false, bool IsFileName=false);

        typedef std::vector<std::vector<std::string>> CSV;

        CSV LoadCSV(const char* FilePath, bool bContainsHeader = true);

        const std::string GetLocText(const char* LocID, ESupportedLanguage language);

        const std::string GetLocText(const char* LocID);

        bool InsensitiveStringCompare(const std::string& InStr1, const std::string& InStr2);

        // ImFileDialog always result in wrong path format... 
        std::string ChangePathSlash(const std::string& InString);

        // std::wstring ConvertUtf8ToWide(const std::string& str);

        void AddSimpleTooltip(const char* Desc, float WrapSize = 35.0f, float Offset = 0.f);

        // copy from https://github.com/ocornut/imgui/issues/2718#issuecomment-591057202   NOT WORKING... flashed and return to uneditable format...
        bool SelectableInput(const char* str_id, bool selected, ImGuiSelectableFlags flags, char* buf, size_t buf_size);

        template <typename C, typename T>
        bool Contains(C&& c, T e)
        {
            return std::find(std::begin(c), std::end(c), e) != std::end(c);
        };

        std::vector<std::string> Split(const std::string& str, const char& delimiter);

        ImVec4 RandomPickColor(ETheme ActiveTheme = ETheme::Light);

        int RandomIntInRange(int Min, int Max);
        bool RandomBool();
        float Round(const float& InValue, const int& N_Digits);
        bool FloatCompare(float A, float B, float Tolerance = 0.005f);

        // common math
        template <typename T>
        float Mean(const std::vector<T>& InData)
        {
            T Sum = 0;
            for (const auto& i : InData)
                Sum += i;
            return (float)Sum / (float)InData.size();
        }

        template <typename T>
        T Max(const std::vector<T>& InData)
        {
            int out = -99999;
            for (const int i : InData)
            {
                if (i > out) out = i;
            }
            return (T)out;
        }

        float Distance(float X1, float Y1, float X2, float Y2);

        void GetAbsRectMinMax(ImVec2 p0, ImVec2 p1, ImVec2& OutMin, ImVec2& OutMax);

        bool IsPointInsideRect(const ImVec2& ToCheck, const ImVec2& RectP0, const ImVec2& RectP1);

        // opencv to opengl helper
    }
}

/*
 * Meyers Singleton
 * Ref from: https://shengyu7697.github.io/cpp-singleton-pattern/
 */
#define MAKE_SINGLETON(class_name) \
    class_name()=default; \
    static class_name& Get() \
    { \
        static class_name singleton; \
        return singleton; \
    }\
    class_name(class_name const&) = delete;\
    void operator=(class_name const&) = delete;

#define LOCTEXT(key) Utils::GetLocText(key).c_str()
