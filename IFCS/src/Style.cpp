#include "Style.h"

namespace IFCS
{
    namespace Style
    {
        ImVec4 EMPTY()
        {
            return ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        }

        ImVec4 WHITE()
        {
            return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        }

        ImVec4 BLACK()
        {
            return ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        }

        ImVec4 GRAY(int weight, ETheme ActiveTheme)
        {
            if (ActiveTheme == ETheme::Light)
            {
                switch (weight)
                {
                case 50: return ImGui::ColorConvertU32ToFloat4(Color(0xFFFFFF));
                case 75: return ImGui::ColorConvertU32ToFloat4(Color(0xFAFAFA));
                case 100: return ImGui::ColorConvertU32ToFloat4(Color(0xF5F5F5));
                case 200: return ImGui::ColorConvertU32ToFloat4(Color(0xEAEAEA));
                case 300: return ImGui::ColorConvertU32ToFloat4(Color(0xE1E1E1));
                case 400: return ImGui::ColorConvertU32ToFloat4(Color(0xCACACA));
                case 500: return ImGui::ColorConvertU32ToFloat4(Color(0xB3B3B3));
                case 600: return ImGui::ColorConvertU32ToFloat4(Color(0x8E8E8E));
                case 700: return ImGui::ColorConvertU32ToFloat4(Color(0x707070));
                case 800: return ImGui::ColorConvertU32ToFloat4(Color(0x4B4B4B));
                case 900: return ImGui::ColorConvertU32ToFloat4(Color(0x2C2C2C));
                default: return EMPTY();
                }
            }
            switch (weight)
            {
            case 50: return ImGui::ColorConvertU32ToFloat4(Color(0x252525));
            case 75: return ImGui::ColorConvertU32ToFloat4(Color(0x2F2F2F));
            case 100: return ImGui::ColorConvertU32ToFloat4(Color(0x323232));
            case 200: return ImGui::ColorConvertU32ToFloat4(Color(0x393939));
            case 300: return ImGui::ColorConvertU32ToFloat4(Color(0x3E3E3E));
            case 400: return ImGui::ColorConvertU32ToFloat4(Color(0x4D4D4D));
            case 500: return ImGui::ColorConvertU32ToFloat4(Color(0x5C5C5C));
            case 600: return ImGui::ColorConvertU32ToFloat4(Color(0x7B7B7B));
            case 700: return ImGui::ColorConvertU32ToFloat4(Color(0x999999));
            case 800: return ImGui::ColorConvertU32ToFloat4(Color(0xCDCDCD));
            case 900: return ImGui::ColorConvertU32ToFloat4(Color(0xFFFFFF));
            default: return EMPTY();
            }
        }

        ImVec4 BLUE(int weight, ETheme ActiveTheme)
        {
            if (ActiveTheme == ETheme::Light)
            {
                switch (weight)
                {
                case 400: return ImGui::ColorConvertU32ToFloat4(Color(0x2680EB));
                case 500: return ImGui::ColorConvertU32ToFloat4(Color(0x1473E6));
                case 600: return ImGui::ColorConvertU32ToFloat4(Color(0x0D66D0));
                case 700: return ImGui::ColorConvertU32ToFloat4(Color(0x095ABA));
                default: return EMPTY();
                }
            }
            switch (weight)
            {
            case 400: return ImGui::ColorConvertU32ToFloat4(Color(0x2680EB));
            case 500: return ImGui::ColorConvertU32ToFloat4(Color(0x378EF0));
            case 600: return ImGui::ColorConvertU32ToFloat4(Color(0x4B9CF5));
            case 700: return ImGui::ColorConvertU32ToFloat4(Color(0x5AA9FA));
            default: return EMPTY();
            }
        }

        ImVec4 RED(int weight, ETheme ActiveTheme)
        {
            if (ActiveTheme == ETheme::Light)
            {
                switch (weight)
                {
                case 400: return ImGui::ColorConvertU32ToFloat4(Color(0xE34850));
                case 500: return ImGui::ColorConvertU32ToFloat4(Color(0xD7373F));
                case 600: return ImGui::ColorConvertU32ToFloat4(Color(0xC9252D));
                case 700: return ImGui::ColorConvertU32ToFloat4(Color(0xBB121A));
                default: return EMPTY();
                }
            }
            switch (weight)
            {
            case 400: return ImGui::ColorConvertU32ToFloat4(Color(0xE34850));
            case 500: return ImGui::ColorConvertU32ToFloat4(Color(0xEC5B62));
            case 600: return ImGui::ColorConvertU32ToFloat4(Color(0xF76D74));
            case 700: return ImGui::ColorConvertU32ToFloat4(Color(0xFF7B82));
            default: return EMPTY();
            }
        }

        ImVec4 ORANGE(int weight, ETheme ActiveTheme)
        {
            if (ActiveTheme == ETheme::Light)
            {
                switch (weight)
                {
                case 400: return ImGui::ColorConvertU32ToFloat4(Color(0xE68619));
                case 500: return ImGui::ColorConvertU32ToFloat4(Color(0xDA7B11));
                case 600: return ImGui::ColorConvertU32ToFloat4(Color(0xCB6F10));
                case 700: return ImGui::ColorConvertU32ToFloat4(Color(0xBD640D));
                default: return EMPTY();
                }
            }
            switch (weight)
            {
            case 400: return ImGui::ColorConvertU32ToFloat4(Color(0xE68619));
            case 500: return ImGui::ColorConvertU32ToFloat4(Color(0xF29423));
            case 600: return ImGui::ColorConvertU32ToFloat4(Color(0xF9A43F));
            case 700: return ImGui::ColorConvertU32ToFloat4(Color(0xFFB55B));
            default: return EMPTY();
            }
        }

        ImVec4 GREEN(int weight, ETheme ActiveTheme)
        {
            if (ActiveTheme == ETheme::Light)
            {
                switch (weight)
                {
                case 400: return ImGui::ColorConvertU32ToFloat4(Color(0x2D9D78));
                case 500: return ImGui::ColorConvertU32ToFloat4(Color(0x268E6C));
                case 600: return ImGui::ColorConvertU32ToFloat4(Color(0x12805C));
                case 700: return ImGui::ColorConvertU32ToFloat4(Color(0x107154));
                default: return EMPTY();
                }
            }
            switch (weight)
            {
            case 400: return ImGui::ColorConvertU32ToFloat4(Color(0x2D9D78));
            case 500: return ImGui::ColorConvertU32ToFloat4(Color(0x33AB84));
            case 600: return ImGui::ColorConvertU32ToFloat4(Color(0x39B990));
            case 700: return ImGui::ColorConvertU32ToFloat4(Color(0x3FC89C));
            default: return EMPTY();
            }
        }

        ImVec4 INDIGO(int weight, ETheme ActiveTheme)
        {
            if (ActiveTheme == ETheme::Light)
            {
                switch (weight)
                {
                case 400: return ImGui::ColorConvertU32ToFloat4(Color(0x6767EC));
                case 500: return ImGui::ColorConvertU32ToFloat4(Color(0x5C5CE0));
                case 600: return ImGui::ColorConvertU32ToFloat4(Color(0x5151D3));
                case 700: return ImGui::ColorConvertU32ToFloat4(Color(0x4646C6));
                default: return EMPTY();
                }
            }
            switch (weight)
            {
            case 400: return ImGui::ColorConvertU32ToFloat4(Color(0x6767EC));
            case 500: return ImGui::ColorConvertU32ToFloat4(Color(0x7575F1));
            case 600: return ImGui::ColorConvertU32ToFloat4(Color(0x8282F6));
            case 700: return ImGui::ColorConvertU32ToFloat4(Color(0x9090FA));
            default: return EMPTY();
            }
        }

        ImVec4 CELERY(int weight, ETheme ActiveTheme)
        {
            if (ActiveTheme == ETheme::Light)
            {
                switch (weight)
                {
                case 400: return ImGui::ColorConvertU32ToFloat4(Color(0x44B556));
                case 500: return ImGui::ColorConvertU32ToFloat4(Color(0x3DA74E));
                case 600: return ImGui::ColorConvertU32ToFloat4(Color(0x379947));
                case 700: return ImGui::ColorConvertU32ToFloat4(Color(0x318B40));
                default: return EMPTY();
                }
            }
            switch (weight)
            {
            case 400: return ImGui::ColorConvertU32ToFloat4(Color(0x44B556));
            case 500: return ImGui::ColorConvertU32ToFloat4(Color(0x4BC35F));
            case 600: return ImGui::ColorConvertU32ToFloat4(Color(0x51D267));
            case 700: return ImGui::ColorConvertU32ToFloat4(Color(0x58E06F));
            default: return EMPTY();
            }
        }

        ImVec4 MAGENTA(int weight, ETheme ActiveTheme)
        {
            if (ActiveTheme == ETheme::Light)
            {
                switch (weight)
                {
                case 400: return ImGui::ColorConvertU32ToFloat4(Color(0xD83790));
                case 500: return ImGui::ColorConvertU32ToFloat4(Color(0xCE2783));
                case 600: return ImGui::ColorConvertU32ToFloat4(Color(0xBC1C74));
                case 700: return ImGui::ColorConvertU32ToFloat4(Color(0xAE0E66));
                default: return EMPTY();
                }
            }
            switch (weight)
            {
            case 400: return ImGui::ColorConvertU32ToFloat4(Color(0xD83790));
            case 500: return ImGui::ColorConvertU32ToFloat4(Color(0xE2499D));
            case 600: return ImGui::ColorConvertU32ToFloat4(Color(0xEC5AAA));
            case 700: return ImGui::ColorConvertU32ToFloat4(Color(0xF56BB7));
            default: return EMPTY();
            }
        }

        ImVec4 YELLOW(int weight, ETheme ActiveTheme)
        {
            if (ActiveTheme == ETheme::Light)
            {
                switch (weight)
                {
                case 400: return ImGui::ColorConvertU32ToFloat4(Color(0xDFBF00));
                case 500: return ImGui::ColorConvertU32ToFloat4(Color(0xD2B200));
                case 600: return ImGui::ColorConvertU32ToFloat4(Color(0xC4A600));
                case 700: return ImGui::ColorConvertU32ToFloat4(Color(0xB79900));
                default: return EMPTY();
                }
            }
            switch (weight)
            {
            case 400: return ImGui::ColorConvertU32ToFloat4(Color(0xDFBF00));
            case 500: return ImGui::ColorConvertU32ToFloat4(Color(0xEDCC00));
            case 600: return ImGui::ColorConvertU32ToFloat4(Color(0xFAD900));
            case 700: return ImGui::ColorConvertU32ToFloat4(Color(0xFFE22E));
            default: return EMPTY();
            }
        }

        ImVec4 FUCHSIA(int weight, ETheme ActiveTheme)
        {
            if (ActiveTheme == ETheme::Light)
            {
                switch (weight)
                {
                case 400: return ImGui::ColorConvertU32ToFloat4(Color(0xC038CC));
                case 500: return ImGui::ColorConvertU32ToFloat4(Color(0xB130BD));
                case 600: return ImGui::ColorConvertU32ToFloat4(Color(0xA228AD));
                case 700: return ImGui::ColorConvertU32ToFloat4(Color(0x93219E));
                default: return EMPTY();
                }
            }
            switch (weight)
            {
            case 400: return ImGui::ColorConvertU32ToFloat4(Color(0xC038CC));
            case 500: return ImGui::ColorConvertU32ToFloat4(Color(0xCF3EDC));
            case 600: return ImGui::ColorConvertU32ToFloat4(Color(0xD951E5));
            case 700: return ImGui::ColorConvertU32ToFloat4(Color(0xE366EF));
            default: return EMPTY();
            }
        }

        // Addition add one for button
        ImVec4 SEAFOAM(int weight, ETheme ActiveTheme)
        {
            if (ActiveTheme == ETheme::Light)
            {
                switch (weight)
                {
                case 400: return ImGui::ColorConvertU32ToFloat4(Color(0x1B959A));
                case 500: return ImGui::ColorConvertU32ToFloat4(Color(0x16878C));
                case 600: return ImGui::ColorConvertU32ToFloat4(Color(0xF7997D));
                case 700: return ImGui::ColorConvertU32ToFloat4(Color(0x098C6F));
                case 1000: return {0 / 255.f, 99 / 255.f, 95 / 255.f, 1.f};
                default: return EMPTY();
                }
            }
            switch (weight)
            {
            case 400: return ImGui::ColorConvertU32ToFloat4(Color(0x1B959A));
            case 500: return ImGui::ColorConvertU32ToFloat4(Color(0x20A3A8));
            case 600: return ImGui::ColorConvertU32ToFloat4(Color(0x23B2B8));
            case 700: return ImGui::ColorConvertU32ToFloat4(Color(0x26C0C7));
            case 1000: return {93 / 255.f, 214 / 255.f, 207 / 255.f, 1.f};
            default: return EMPTY();
            }
        }

        ImVec4 CHARTREUSE(int weight, ETheme ActiveTheme)
        {
            if (ActiveTheme == ETheme::Light)
            {
                switch (weight)
                {
                case 400: return ImGui::ColorConvertU32ToFloat4(Color(0x85D044));
                case 500: return ImGui::ColorConvertU32ToFloat4(Color(0x7CC33F));
                case 600: return ImGui::ColorConvertU32ToFloat4(Color(0x73B53A));
                case 700: return ImGui::ColorConvertU32ToFloat4(Color(0x6AA834));
                default: return EMPTY();
                }
            }
            switch (weight)
            {
            case 400: return ImGui::ColorConvertU32ToFloat4(Color(0x85D044));
            case 500: return ImGui::ColorConvertU32ToFloat4(Color(0x8EDE46));
            case 600: return ImGui::ColorConvertU32ToFloat4(Color(0x9BEC54));
            case 700: return ImGui::ColorConvertU32ToFloat4(Color(0xA3F858));
            default: return EMPTY();
            }
        }

        ImVec4 PURPLE(int weight, ETheme ActiveTheme)
        {
            if (ActiveTheme == ETheme::Light)
            {
                switch (weight)
                {
                case 400: return ImGui::ColorConvertU32ToFloat4(Color(0x9256D9));
                case 500: return ImGui::ColorConvertU32ToFloat4(Color(0x864CCC));
                case 600: return ImGui::ColorConvertU32ToFloat4(Color(0x7A42BF));
                case 700: return ImGui::ColorConvertU32ToFloat4(Color(0x6F38B1));
                default: return EMPTY();
                }
            }
            switch (weight)
            {
            case 400: return ImGui::ColorConvertU32ToFloat4(Color(0x9256D9));
            case 500: return ImGui::ColorConvertU32ToFloat4(Color(0x9D41E1));
            case 600: return ImGui::ColorConvertU32ToFloat4(Color(0xA873E9));
            case 700: return ImGui::ColorConvertU32ToFloat4(Color(0xB483F0));
            default: return EMPTY();
            }
        }

        void ApplyTheme(ETheme ActiveTheme)
        {
            ImGuiStyle* style = &ImGui::GetStyle();
            style->GrabRounding = 4.0f;
            style->DisplaySafeAreaPadding = ImVec2(3, 6);

            ImVec4* colors = style->Colors;
            colors[ImGuiCol_Text] = GRAY(800, ActiveTheme); // text on hovered controls is gray900
            colors[ImGuiCol_TextDisabled] = GRAY(500, ActiveTheme);
            colors[ImGuiCol_WindowBg] = GRAY(100, ActiveTheme);
            colors[ImGuiCol_ChildBg] = EMPTY();
            colors[ImGuiCol_PopupBg] = GRAY(50, ActiveTheme);
            // not sure about this. Note: applies to tooltips too.
            colors[ImGuiCol_Border] = GRAY(300, ActiveTheme);
            colors[ImGuiCol_BorderShadow] = EMPTY(); // We don't want shadows. Ever.

            // the color is so bad... can not see the frame at all...
            colors[ImGuiCol_FrameBg] = GRAY(300, ActiveTheme);
            // this isnt right, spectrum does not do this, but it's a good fallback
            colors[ImGuiCol_FrameBgHovered] = GRAY(400, ActiveTheme);
            colors[ImGuiCol_FrameBgActive] = GRAY(500, ActiveTheme);


            colors[ImGuiCol_TitleBg] = GRAY(300, ActiveTheme);
            // those titlebar values are totally made up, spectrum does not have this.
            colors[ImGuiCol_TitleBgActive] = GRAY(200, ActiveTheme);
            colors[ImGuiCol_TitleBgCollapsed] = GRAY(400, ActiveTheme);
            colors[ImGuiCol_MenuBarBg] = GRAY(100, ActiveTheme);
            colors[ImGuiCol_ScrollbarBg] = GRAY(100, ActiveTheme); // same as regular background
            colors[ImGuiCol_ScrollbarGrab] = GRAY();
            colors[ImGuiCol_ScrollbarGrabHovered] = GRAY(600, ActiveTheme);
            colors[ImGuiCol_ScrollbarGrabActive] = GRAY(700, ActiveTheme);
            colors[ImGuiCol_CheckMark] = BLUE(500, ActiveTheme);
            colors[ImGuiCol_SliderGrab] = GRAY(700, ActiveTheme);
            colors[ImGuiCol_SliderGrabActive] = GRAY(800, ActiveTheme);

            // the default btn is way too bad..
            // colors[ImGuiCol_Button] = GRAY(75, ActiveTheme); // match default button to Spectrum's 'Action Button'.
            // colors[ImGuiCol_ButtonHovered] = GRAY(50, ActiveTheme);
            // colors[ImGuiCol_ButtonActive] = GRAY(200, ActiveTheme);

            // change to seafoam 500, 600, 700
            colors[ImGuiCol_Button] = SEAFOAM(400, ActiveTheme);
            colors[ImGuiCol_ButtonHovered] = SEAFOAM(600, ActiveTheme);
            colors[ImGuiCol_ButtonActive] = SEAFOAM(700, ActiveTheme);
            // match default button to Spectrum's 'Action Button'.

            colors[ImGuiCol_Header] = BLUE(400, ActiveTheme);
            colors[ImGuiCol_HeaderHovered] = BLUE(500, ActiveTheme);
            colors[ImGuiCol_HeaderActive] = BLUE(600, ActiveTheme);
            colors[ImGuiCol_Separator] = GRAY(400, ActiveTheme);
            colors[ImGuiCol_SeparatorHovered] = GRAY(600, ActiveTheme);
            colors[ImGuiCol_SeparatorActive] = GRAY(700, ActiveTheme);
            colors[ImGuiCol_ResizeGrip] = GRAY(400, ActiveTheme);
            colors[ImGuiCol_ResizeGripHovered] = GRAY(600, ActiveTheme);
            colors[ImGuiCol_ResizeGripActive] = GRAY(700, ActiveTheme);
            colors[ImGuiCol_PlotLines] = BLUE(400, ActiveTheme);
            colors[ImGuiCol_PlotLinesHovered] = BLUE(600, ActiveTheme);
            colors[ImGuiCol_PlotHistogram] = BLUE(400, ActiveTheme);
            colors[ImGuiCol_PlotHistogramHovered] = BLUE(600, ActiveTheme);
            colors[ImGuiCol_TextSelectedBg] = ImVec4(1, 0.921569f, 0.501961f, 0.2f);
            colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
            if (ActiveTheme == ETheme::Light)
                colors[ImGuiCol_NavHighlight] = ImVec4(0.173f, 0.173f, 0.173f, 0.04f);
            else
                colors[ImGuiCol_NavHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.04f);

            colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
            colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
            colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

            // more missing colors...
            colors[ImGuiCol_Tab] = BLUE(600, ActiveTheme);
            colors[ImGuiCol_TabHovered] = BLUE(700, ActiveTheme);
            colors[ImGuiCol_TabActive] = BLUE(400, ActiveTheme);
            colors[ImGuiCol_TabUnfocused] = BLUE(500, ActiveTheme);
            colors[ImGuiCol_TabUnfocusedActive] = BLUE(400, ActiveTheme);
            
            colors[ImGuiCol_TableHeaderBg] = GRAY(400, ActiveTheme);
        }
    }
}
