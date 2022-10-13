#include "ModelGenerator.h"
#include <iostream>
#include <stdio.h>
#include <string>

#include "Spectrum/imgui_spectrum.h"


namespace IFCS
{
    void ModelGenerator::RenderContent()
    {
        // test get console output...
        ImGuiInputTextFlags Flags = ImGuiInputTextFlags_AllowTabInput;
        Flags |= ImGuiInputTextFlags_CtrlEnterForNewLine;
        ImGui::InputTextMultiline("Script", Script, IM_ARRAYSIZE(Script), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight()*8), Flags);
        if (ImGui::Button("Test Exec"))
        {
            RunResult = Exec(Script);
            RunSomething = true;
        }
        if (RunSomething)
        {
            ImGui::TextColored(Spectrum::YELLOW(), RunResult.c_str());
        }
    }

    std::string ModelGenerator::Exec(const char* cmd)
    {
        std::shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
        if (!pipe) return "ERROR";
        char buffer[128];
        std::string result = "";
        while (!feof(pipe.get()))
        {
            if (fgets(buffer, 128, pipe.get()) != NULL)
                result += buffer;
        }
        return result;
    }
}
