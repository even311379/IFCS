#include "Train.h"

#include <fstream>
#include <future>
#include <thread>
#include <iostream>
#include <string>

#include "Setting.h"
#include "ImFileDialog/ImFileDialog.h"

// TODO: model comparison!!!

namespace IFCS
{
    static std::string RunResult;

    void Train::RenderContent()
    {
        // TODO: migrate these env setup to settings?

        if (Setting::Get().IsEnvSet())
        {
            static const char* ModelOptions[] = {"yolov7", "yolov7-d6", "yolov7-e6", "yolov7-e6e", "yolov7-tiny", "yolov7-w6", "yolov7x"};
            // model select
            ImGui::Combo("Choose Model", &SelectedModelIndex, ModelOptions, IM_ARRAYSIZE(ModelOptions));
            // if (!CheckWeightHasDownloaded())
            // {
            //     // TODO: need to test it! ... test failure
            //     const char* T;
            //     Utils::Items_SingleStringGetter(ModelOptions, SelectedModelIndex, T);
            //     ImGui::Text("%s is missing!", T);
            //     ImGui::Button("Download Weight");
            // }
            // data select

            // TODO: will crash here since avail training set is empty...
            // if (ImGui::BeginCombo("Choose Training Set", AvailTrainingSets[SelectedSetIdx].c_str()))
            // {
            //     for (size_t i = 0; i < AvailTrainingSets.size(); i++)
            //     {
            //         if (ImGui::Selectable(AvailTrainingSets[i].c_str(), i == SelectedSetIdx))
            //         {
            //             SelectedSetIdx = i;
            //         }
            //     }
            //     ImGui::EndCombo();
            // }

            // epoch
            ImGui::DragInt("Num Epoch", &Epoch, 1, 1, 300);
            // batch size
            ImGui::DragInt("Num batch size", &BatchSize, 1, 2, 128);
            // Image size
            ImGui::DragInt2("Image Width", ImageSize, 16, 64, 1280);
            ImGui::Text("About to run: ");
            // ImGui::InputTextMultiline();
            if (ImGui::Button("Start Training", ImVec2(-1, 0)))
            {
                Training();
            }
        }
        else
        {
            ImGui::Text("Yolo V7 Environment not setup yet!");
        }
        ImGui::Separator();
        ImGui::Text("Training Log:");
        ImGui::BeginChildFrame(ImGui::GetID("TrainingLog"), ImVec2(0, 0));
        // TODO: I can grab results.txt during traning... as long as one batch is done... that file get updated...
        // TODO: use that info to draw table + learning curve
        ImGui::TextWrapped("%s", RunResult.c_str());
        ImGui::EndChildFrame();
    }


    bool Train::CheckWeightHasDownloaded()
    {
        return false;
    }

    static void Exec(const char* cmd, bool ShouldClearResult)
    {
        std::shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
        char buffer[1024];
        if (ShouldClearResult) RunResult = "";
        while (!feof(pipe.get()))
        {
            if (fgets(buffer, 1024, pipe.get()) != NULL)
                RunResult.append(buffer);
        }
    }


    void Train::Training()
    {
        std::thread t1(Exec, "SubTest.bat", true);
        t1.detach();
    }
}
