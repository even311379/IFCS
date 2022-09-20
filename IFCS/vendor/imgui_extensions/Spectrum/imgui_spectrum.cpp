#include "imgui_spectrum.h"

#include "imgui.h"

namespace Spectrum {

    void StyleColorsSpectrum(bool is_light) {

        unsigned int EMPTY, WHITE, BLACK, GRAY200, GRAY300, GRAY400, GRAY500, GRAY600, GRAY700, GRAY800, GRAY900;
        unsigned int GRAY50, GRAY75, GRAY100;
        unsigned int BLUE400, BLUE500, BLUE600, BLUE700;
        unsigned int RED400, RED500, RED600, RED700;
        unsigned int ORANGE400, ORANGE500, ORANGE600, ORANGE700;
        unsigned int GREEN400, GREEN500, GREEN600, GREEN700;
        unsigned int INDIGO400, INDIGO500, INDIGO600, INDIGO700;
        unsigned int CELERY400, CELERY500, CELERY600, CELERY700;
        unsigned int MAGENTA400, MAGENTA500, MAGENTA600, MAGENTA700;
        unsigned int YELLOW400, YELLOW500, YELLOW600, YELLOW700;
        unsigned int FUCHSIA400, FUCHSIA500, FUCHSIA600, FUCHSIA700;
        unsigned int SEAFOAM400, SEAFOAM500, SEAFOAM600, SEAFOAM700;
        unsigned int CHARTREUSE400, CHARTREUSE500, CHARTREUSE600, CHARTREUSE700;
        unsigned int PURPLE400, PURPLE500, PURPLE600, PURPLE700;
        
        
        EMPTY = 0x00000000; // transparent
        WHITE = Color(0xFFFFFF);
        BLACK = Color(0x000000);
        GRAY200 = Color(0xF4F4F4);
        GRAY300 = Color(0xEAEAEA);
        GRAY400 = Color(0xD3D3D3);
        GRAY500 = Color(0xBCBCBC);
        GRAY600 = Color(0x959595);
        GRAY700 = Color(0x767676);
        GRAY800 = Color(0x505050);
        GRAY900 = Color(0x323232);
        BLUE400 = Color(0x378EF0);
        BLUE500 = Color(0x2680EB);
        BLUE600 = Color(0x1473E6);
        BLUE700 = Color(0x0D66D0);
        RED400 = Color(0xEC5B62);
        RED500 = Color(0xE34850);
        RED600 = Color(0xD7373F);
        RED700 = Color(0xC9252D);
        ORANGE400 = Color(0xF29423);
        ORANGE500 = Color(0xE68619);
        ORANGE600 = Color(0xDA7B11);
        ORANGE700 = Color(0xCB6F10);
        GREEN400 = Color(0x33AB84);
        GREEN500 = Color(0x2D9D78);
        GREEN600 = Color(0x268E6C);
        GREEN700 = Color(0x12805C);
        
        if (is_light)
        {
            GRAY50 = Color(0xFFFFFF);
            GRAY75 = Color(0xFAFAFA);
            GRAY100 = Color(0xF5F5F5);
            GRAY200 = Color(0xEAEAEA);
            GRAY300 = Color(0xE1E1E1);
            GRAY400 = Color(0xCACACA);
            GRAY500 = Color(0xB3B3B3);
            GRAY600 = Color(0x8E8E8E);
            GRAY700 = Color(0x707070);
            GRAY800 = Color(0x4B4B4B);
            GRAY900 = Color(0x2C2C2C);
            BLUE400 = Color(0x2680EB);
            BLUE500 = Color(0x1473E6);
            BLUE600 = Color(0x0D66D0);
            BLUE700 = Color(0x095ABA);
            RED400 = Color(0xE34850);
            RED500 = Color(0xD7373F);
            RED600 = Color(0xC9252D);
            RED700 = Color(0xBB121A);
            ORANGE400 = Color(0xE68619);
            ORANGE500 = Color(0xDA7B11);
            ORANGE600 = Color(0xCB6F10);
            ORANGE700 = Color(0xBD640D);
            GREEN400 = Color(0x2D9D78);
            GREEN500 = Color(0x268E6C);
            GREEN600 = Color(0x12805C);
            GREEN700 = Color(0x107154);
            INDIGO400 = Color(0x6767EC);
            INDIGO500 = Color(0x5C5CE0);
            INDIGO600 = Color(0x5151D3);
            INDIGO700 = Color(0x4646C6);
            CELERY400 = Color(0x44B556);
            CELERY500 = Color(0x3DA74E);
            CELERY600 = Color(0x379947);
            CELERY700 = Color(0x318B40);
            MAGENTA400 = Color(0xD83790);
            MAGENTA500 = Color(0xCE2783);
            MAGENTA600 = Color(0xBC1C74);
            MAGENTA700 = Color(0xAE0E66);
            YELLOW400 = Color(0xDFBF00);
            YELLOW500 = Color(0xD2B200);
            YELLOW600 = Color(0xC4A600);
            YELLOW700 = Color(0xB79900);
            FUCHSIA400 = Color(0xC038CC);
            FUCHSIA500 = Color(0xB130BD);
            FUCHSIA600 = Color(0xA228AD);
            FUCHSIA700 = Color(0x93219E);
            SEAFOAM400 = Color(0x1B959A);
            SEAFOAM500 = Color(0x16878C);
            SEAFOAM600 = Color(0x0F797D);
            SEAFOAM700 = Color(0x096C6F);
            CHARTREUSE400 = Color(0x85D044);
            CHARTREUSE500 = Color(0x7CC33F);
            CHARTREUSE600 = Color(0x73B53A);
            CHARTREUSE700 = Color(0x6AA834);
            PURPLE400 = Color(0x9256D9);
            PURPLE500 = Color(0x864CCC);
            PURPLE600 = Color(0x7A42BF);
            PURPLE700 = Color(0x6F38B1);
            
        }
        else
        {
            GRAY50 = Color(0x252525);
            GRAY75 = Color(0x2F2F2F);
            GRAY100 = Color(0x323232);
            GRAY200 = Color(0x393939);
            GRAY300 = Color(0x3E3E3E);
            GRAY400 = Color(0x4D4D4D);
            GRAY500 = Color(0x5C5C5C);
            GRAY600 = Color(0x7B7B7B);
            GRAY700 = Color(0x999999);
            GRAY800 = Color(0xCDCDCD);
            GRAY900 = Color(0xFFFFFF);
            BLUE400 = Color(0x2680EB);
            BLUE500 = Color(0x378EF0);
            BLUE600 = Color(0x4B9CF5);
            BLUE700 = Color(0x5AA9FA);
            RED400 = Color(0xE34850);
            RED500 = Color(0xEC5B62);
            RED600 = Color(0xF76D74);
            RED700 = Color(0xFF7B82);
            ORANGE400 = Color(0xE68619);
            ORANGE500 = Color(0xF29423);
            ORANGE600 = Color(0xF9A43F);
            ORANGE700 = Color(0xFFB55B);
            GREEN400 = Color(0x2D9D78);
            GREEN500 = Color(0x33AB84);
            GREEN600 = Color(0x39B990);
            GREEN700 = Color(0x3FC89C);
            INDIGO400 = Color(0x6767EC);
            INDIGO500 = Color(0x7575F1);
            INDIGO600 = Color(0x8282F6);
            INDIGO700 = Color(0x9090FA);
            CELERY400 = Color(0x44B556);
            CELERY500 = Color(0x4BC35F);
            CELERY600 = Color(0x51D267);
            CELERY700 = Color(0x58E06F);
            MAGENTA400 = Color(0xD83790);
            MAGENTA500 = Color(0xE2499D);
            MAGENTA600 = Color(0xEC5AAA);
            MAGENTA700 = Color(0xF56BB7);
            YELLOW400 = Color(0xDFBF00);
            YELLOW500 = Color(0xEDCC00);
            YELLOW600 = Color(0xFAD900);
            YELLOW700 = Color(0xFFE22E);
            FUCHSIA400 = Color(0xC038CC);
            FUCHSIA500 = Color(0xCF3EDC);
            FUCHSIA600 = Color(0xD951E5);
            FUCHSIA700 = Color(0xE366EF);
            SEAFOAM400 = Color(0x1B959A);
            SEAFOAM500 = Color(0x20A3A8);
            SEAFOAM600 = Color(0x23B2B8);
            SEAFOAM700 = Color(0x26C0C7);
            CHARTREUSE400 = Color(0x85D044);
            CHARTREUSE500 = Color(0x8EDE49);
            CHARTREUSE600 = Color(0x9BEC54);
            CHARTREUSE700 = Color(0xA3F858);
            PURPLE400 = Color(0x9256D9);
            PURPLE500 = Color(0x9D64E1);
            PURPLE600 = Color(0xA873E9);
            PURPLE700 = Color(0xB483F0);
        }

        ImGuiStyle* style = &ImGui::GetStyle();
        style->GrabRounding = 4.0f;

        ImVec4* colors = style->Colors;
        colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(GRAY800); // text on hovered controls is gray900
        colors[ImGuiCol_TextDisabled] = ImGui::ColorConvertU32ToFloat4(GRAY500);
        colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(GRAY100);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(GRAY50); // not sure about this. Note: applies to tooltips too.
        colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(GRAY300);
        colors[ImGuiCol_BorderShadow] = ImGui::ColorConvertU32ToFloat4(EMPTY); // We don't want shadows. Ever.
        colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(GRAY75); // this isnt right, spectrum does not do this, but it's a good fallback
        colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(GRAY50);
        colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(GRAY200);
        colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(GRAY300); // those titlebar values are totally made up, spectrum does not have this.
        colors[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(GRAY200);
        colors[ImGuiCol_TitleBgCollapsed] = ImGui::ColorConvertU32ToFloat4(GRAY400);
        colors[ImGuiCol_MenuBarBg] = ImGui::ColorConvertU32ToFloat4(GRAY100);
        colors[ImGuiCol_ScrollbarBg] = ImGui::ColorConvertU32ToFloat4(GRAY100); // same as regular background
        colors[ImGuiCol_ScrollbarGrab] = ImGui::ColorConvertU32ToFloat4(GRAY400);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImGui::ColorConvertU32ToFloat4(GRAY600);
        colors[ImGuiCol_ScrollbarGrabActive] = ImGui::ColorConvertU32ToFloat4(GRAY700);
        colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(BLUE500);
        colors[ImGuiCol_SliderGrab] = ImGui::ColorConvertU32ToFloat4(GRAY700);
        colors[ImGuiCol_SliderGrabActive] = ImGui::ColorConvertU32ToFloat4(GRAY800);
        
        /* the default btn is way too bad..
        colors[ImGuiCol_Button] = ImGui::ColorConvertU32ToFloat4(GRAY75); // match default button to Spectrum's 'Action Button'.
        colors[ImGuiCol_ButtonHovered] = ImGui::ColorConvertU32ToFloat4(GRAY50);
        colors[ImGuiCol_ButtonActive] = ImGui::ColorConvertU32ToFloat4(GRAY200);
        */
        // change to seafoam 400, 500, 700
        colors[ImGuiCol_Button] = ImGui::ColorConvertU32ToFloat4(SEAFOAM400); // match default button to Spectrum's 'Action Button'.
        colors[ImGuiCol_ButtonHovered] = ImGui::ColorConvertU32ToFloat4(SEAFOAM500);
        colors[ImGuiCol_ButtonActive] = ImGui::ColorConvertU32ToFloat4(SEAFOAM700);
        
        colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(BLUE400);
        colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(BLUE500);
        colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(BLUE600);
        colors[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(GRAY400);
        colors[ImGuiCol_SeparatorHovered] = ImGui::ColorConvertU32ToFloat4(GRAY600);
        colors[ImGuiCol_SeparatorActive] = ImGui::ColorConvertU32ToFloat4(GRAY700);
        colors[ImGuiCol_ResizeGrip] = ImGui::ColorConvertU32ToFloat4(GRAY400);
        colors[ImGuiCol_ResizeGripHovered] = ImGui::ColorConvertU32ToFloat4(GRAY600);
        colors[ImGuiCol_ResizeGripActive] = ImGui::ColorConvertU32ToFloat4(GRAY700);
        colors[ImGuiCol_PlotLines] = ImGui::ColorConvertU32ToFloat4(BLUE400);
        colors[ImGuiCol_PlotLinesHovered] = ImGui::ColorConvertU32ToFloat4(BLUE600);
        colors[ImGuiCol_PlotHistogram] = ImGui::ColorConvertU32ToFloat4(BLUE400);
        colors[ImGuiCol_PlotHistogramHovered] = ImGui::ColorConvertU32ToFloat4(BLUE600);
        colors[ImGuiCol_TextSelectedBg] = ImGui::ColorConvertU32ToFloat4((BLUE400 & 0x00FFFFFF) | 0x33000000);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImGui::ColorConvertU32ToFloat4((GRAY900 & 0x00FFFFFF) | 0x0A000000);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
    }
}