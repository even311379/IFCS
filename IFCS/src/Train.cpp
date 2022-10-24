#include "Train.h"
#include "Setting.h"

#include <fstream>
#include <future>
#include <chrono>
#include <thread>
#include <iostream>
#include <string>

#include "yaml-cpp/yaml.h"
#include "ImFileDialog/ImFileDialog.h"
#include "ImguiNotify/font_awesome_5.h"
#include "imgui_stdlib.h"
#include "Style.h"
#include "Implot/implot.h"

// TODO: model comparison!!!

namespace IFCS
{
    static std::string RunResult;

    static const char* ModelOptions[] = {
        "yolov7", "yolov7-d6", "yolov7-e6", "yolov7-e6e", "yolov7-tiny", "yolov7-w6", "yolov7x"
    };

    static const char* HypOptions[] = {
        "p5", "p6", "tiny", "custom"
    };

    void Train::Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose)
    {
        Panel::Setup(InName, InShouldOpen, InFlags, InCanClose);
        UpdateTrainScript();
    }

    void Train::RenderContent()
    {
        // TODO: migrate these env setup to settings?

        if (!Setting::Get().IsEnvSet())
        {
            ImGui::Text("Environment not setup yet!");
            if (ImGui::Button("Open setting to set it now?"))
            {
                Setting::Get().IsModalOpen = true;
            }
            return;
        }
        // model select
        if (ImGui::Combo("Choose Model", &SelectedModelIdx, ModelOptions, IM_ARRAYSIZE(ModelOptions)))
        {
            UpdateTrainScript();
            if (SelectedModelIdx == 4)
                bApplyTransferLearning = false;
        }

        if (ImGui::BeginCombo("Choose Training Set", SelectedTrainingSet.Name.c_str()))
        {
            YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/TrainingSets.yaml");
            for (size_t i = 0; i < Data.size(); i++)
            {
                std::string Name = Data[i]["Name"].as<std::string>();
                if (ImGui::Selectable(Name.c_str(), Name == SelectedTrainingSet.Name))
                {
                    SelectedTrainingSet = FTrainingSetDescription(Data[i]);
                    UpdateTrainScript();
                }
            }
            ImGui::EndCombo();
        }

        // apply transfer learning?
        if (ImGui::Checkbox("Apply Transfer learning?", &bApplyTransferLearning))
        {
            UpdateTrainScript();
        }
        ImGui::SameLine();
        ImGui::Text("(Tiny not supported)");
        // TODO: allow to selection custom pre-trained weight?

        // hyp options
        if (ImGui::Combo("Hyper parameter", &SelectedHypIdx, HypOptions, IM_ARRAYSIZE(HypOptions)))
        {
            UpdateTrainScript();
        }

        // epoch
        if (ImGui::DragInt("Num Epoch", &Epochs, 1, 1, 300))
        {
            UpdateTrainScript();
        }
        // batch size
        if (ImGui::DragInt("Num batch size", &BatchSize, 1, 2, 128))
        {
            UpdateTrainScript();
        }
        // Image size
        if (ImGui::DragInt2("Image Size", ImageSize, 16, 64, 1280))
        {
            UpdateTrainScript();
        }
        // model name
        if (ImGui::InputText("Model name", &ModelName))
        {
            UpdateTrainScript();
        }
        if (SelectedTrainingSet.Name.empty() || ModelName.empty())
        {
            ImGui::TextColored(Style::RED(400, Setting::Get().Theme), "Training set or model name not set yet!");
        }
        else
        {
            ImGui::Text("About to run: ");
            ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 32);
            if (ImGui::Button(ICON_FA_COPY))
            {
                ImGui::SetClipboardText((SetPathCommand + TrainScript).c_str());
            }
            ImGui::BeginChildFrame(ImGui::GetID("CDScript"), ImVec2(0, ImGui::GetTextLineHeight() * 1));
            ImGui::TextWrapped(SetPathCommand.c_str());
            ImGui::EndChildFrame();
            ImGui::BeginChildFrame(ImGui::GetID("TrainScript"), ImVec2(0, ImGui::GetTextLineHeight() * 3));
            ImGui::TextWrapped(TrainScript.c_str());
            ImGui::EndChildFrame();
            if (ImGui::Button("Start Training", ImVec2(-1, 0)))
            {
                Training();
            }
        }

        // TODO: no need to reinvent the wheel!! use tensor board is already good enough!

        
        // ImGui::Separator();
        // ImGui::Text("Training Log:");
        // ImGui::BeginChildFrame(ImGui::GetID("TrainingLog"), ImVec2(0, ImGui::GetTextLineHeight() * 6));
        // // TODO: I can grab results.txt during traning... as long as one batch is done... that file get updated...
        // // TODO: use that info to draw table + learning curve
        // ImGui::TextWrapped("%s", RunResult.c_str());
        // ImGui::EndChildFrame();
        ImGui::Text("Training Progress:");
        ImGui::BeginChildFrame(ImGui::GetID("TrainingProgress"), ImVec2(0, 0));
        ImGui::Text("%s", StartTime);
        ImGui::Text("%s", RunStatus);
        ImGui::Text("%s", EndTime);
        // TODO:: change table header color... too bad...
        if (ImGui::BeginTable("##RunProgress", 15))
        {
            ImGui::TableSetupColumn("Epoch");
            ImGui::TableSetupColumn("gpu_mem");
            ImGui::TableSetupColumn("box");
            ImGui::TableSetupColumn("obj");
            ImGui::TableSetupColumn("cls");
            ImGui::TableSetupColumn("total");
            ImGui::TableSetupColumn("labels");
            ImGui::TableSetupColumn("img_size");
            ImGui::TableSetupColumn("Class");
            ImGui::TableSetupColumn("Images");
            ImGui::TableSetupColumn("Labels");
            ImGui::TableSetupColumn("P");
            ImGui::TableSetupColumn("R");
            ImGui::TableSetupColumn("mAP@.5");
            ImGui::TableSetupColumn("mAP@.5:.95");
            ImGui::TableHeadersRow();

            for (int R = 0 ; R < ImGui::ProgressBar().size; R++)
            {
                ImGui::TableNextRow();
                for (int C=0; C<15; C++)
                {
                    ImGui::TableSetColumnIndex(C);
                    ImGui::Text();
                }
            }
        }
        if (ImPlot::BeginPlot())
        {
            ImPlot::EndPlot();
        }
        // ImGui::TextWrapped("%s", RunProgress.c_str());
        // display a table and a few graphs...
        ImGui::EndChildFrame();

        if (ShouldTrackRunProgress)
        {
            TrackRunProgress();
            // ShouldTra
            Tick++;
        }

        if (ShouldCheckFuture)
        {
            // if (TestFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
            // {
            //     spdlog::info("{}", TestFuture.get());
            //     ShouldCheckFuture = false;
            // }
        }
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
        if (!CheckWeightHasDownloaded())
        {
            // run script to download particular weight
        }
        // create run bash script?
        std::thread t1(Exec, "Train.bat", true);
        t1.detach();

        /*spdlog::info("async sent...");
        ShouldCheckFuture = true;
        TestFuture = std::async(std::launch::async, &Train::TestAsyncFun, 3);*/
    }

    void Train::TrackRunProgress()
    {
    }

    void Train::UpdateTrainScript()
    {
        SetPathCommand = "cd " + Setting::Get().YoloV7Path + "\n";
        TrainScript = "";
        TrainScript += Setting::Get().PythonPath + "/python train.py";
        TrainScript += " --weights ";
        if (bApplyTransferLearning)
            TrainScript += std::string(ModelOptions[SelectedModelIdx]) + "_training.pt";
        else
            TrainScript += "''";
        TrainScript += " --cfg cfg/training/" + std::string(ModelOptions[SelectedModelIdx]) + ".yaml";
        TrainScript += " --data " + Setting::Get().ProjectPath + "/Data/" + SelectedTrainingSet.Name + ".yaml";
        TrainScript += " --hyp data/hpy.scratch." + std::string(HypOptions[SelectedHypIdx]) + ".yaml";
        TrainScript += " --epochs " + std::to_string(Epochs);
        TrainScript += " --batch-size " + std::to_string(BatchSize);
        TrainScript += " --img-size " + std::to_string(ImageSize[0]) + " " + std::to_string(ImageSize[1]);
        TrainScript += " --workers 1 --device 0";
        TrainScript += " --name " + std::string(ModelName);
    }

    /*std::string Train::TestAsyncFun(int time)
    {
        std::this_thread::sleep_for(std::chrono::seconds(time));
        return std::string("done?");
    }*/
}
