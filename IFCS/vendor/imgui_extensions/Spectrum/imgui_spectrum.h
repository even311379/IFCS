#pragma once

/*
Color definitions in ImGui are a good starting point, 
but do not cover all the intricacies of Spectrum's possible colors
in controls and widgets. 

One big difference is that ImGui communicates widget activity 
(hover, pressed) with their background, while spectrum uses a mix
of background and border, with border being the most common choice.

Because of this, we reference extra colors in spectrum from 
imgui.cpp and imgui_widgets.cpp directly, and to make that work, 
we need to have them defined at here at compile time. 
*/

    namespace Spectrum {
        // a list of changes introduced to change the look of the widgets. 
        // Collected here as const rather than being magic numbers spread 
        // around imgui.cpp and imgui_widgets.cpp.
        const float CHECKBOX_BORDER_SIZE = 2.0f;
        const float CHECKBOX_ROUNDING = 2.0f;

        // Sets the ImGui style to Spectrum
        void StyleColorsSpectrum(bool is_light = true);

        inline unsigned int Color(unsigned int c) {
            // add alpha.
            // also swap red and blue channel for some reason.
            // todo: figure out why, and fix it.
            const short a = 0xFF;
            const short r = (c >> 16) & 0xFF;
            const short g = (c >> 8) & 0xFF;
            const short b = (c >> 0) & 0xFF;
            return(a << 24)
                | (r << 0)
                | (g << 8)
                | (b << 16);
        }
        // all colors are from http://spectrum.corp.adobe.com/color.html

        inline unsigned int color_alpha(unsigned int alpha, unsigned int c) {
            return ((alpha & 0xFF) << 24) | (c & 0x00FFFFFF);
        }


    }
