#include "Train.h"
#include "Setting.h"

#include <fstream>
#include <future>
#include <chrono>
#include <thread>
#include <iostream>
#include <string>

#include "Annotation.h"
#include "yaml-cpp/yaml.h"
#include "ImFileDialog/ImFileDialog.h"
#include "ImguiNotify/font_awesome_5.h"
#include "imgui_stdlib.h"
#include "Modals.h"
#include "Style.h"

#include "shellapi.h"

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
        if (!Setting::Get().IsEnvSet())
        {
            ImGui::Text("Environment not setup yet!");
            if (ImGui::Button("Open setting to set it now?"))
            {
                Modals::Get().IsModalOpen_Setting = true;
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
            // for (size_t i = 0; i < Data.size(); i++)
            for (YAML::const_iterator it=Data.begin(); it!=Data.end();++it)
            {
                auto Name = it->first.as<std::string>();
                if (ImGui::Selectable(Name.c_str(), Name == SelectedTrainingSet.Name))
                {
                    SelectedTrainingSet = FTrainingSetDescription(it->first.as<std::string>(), it->second);
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
        if (ImGui::InputInt("Num Epoch", &Epochs, 1, 10))
        {
            if (Epochs < 1) Epochs = 1;
            if (Epochs > 300) Epochs = 300;
            UpdateTrainScript();
        }
        // batch size
        if (ImGui::InputInt("Num batch size", &BatchSize, 1, 10))
        {
            if (BatchSize < 1) BatchSize = 1;
            if (BatchSize > 128) BatchSize = 128;
            UpdateTrainScript();
        }
        // Image size
        if (ImGui::InputInt("Image Width", &ImageSize[0], 32, 128))
        {
            if (ImageSize[0] < 64) ImageSize[0] = 64;
            if (ImageSize[0] > 1280) ImageSize[0] = 1280;
            UpdateTrainScript();
        }
        if (ImGui::InputInt("Image Height", &ImageSize[1], 32, 128))
        {
            if (ImageSize[1] < 64) ImageSize[1] = 64;
            if (ImageSize[1] > 1280) ImageSize[1] = 1280;
            UpdateTrainScript();
        }
        // model name
        if (ImGui::InputText("Model name", &ModelName))
        {
            // TODO: check if the name is used...
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
                ImGui::SetClipboardText((SetPathScript + "\n" + TrainScript).c_str());
            }
            ImGui::BeginChildFrame(ImGui::GetID("TrainScript"), ImVec2(0, ImGui::GetTextLineHeight() * 4));
            ImGui::TextWrapped((SetPathScript + "\n" + TrainScript).c_str());
            ImGui::EndChildFrame();
            if (ImGui::Button("Start Training", ImVec2(-1, 0)))
            {
                Training();
            }
            if (!TrainLog.empty())
            {
                ImGui::Text("Training Log:");
                ImGui::BeginChildFrame(ImGui::GetID("TrainLog"), ImVec2(0, ImGui::GetTextLineHeight() * 3));
                ImGui::TextWrapped(TrainLog.c_str());
                ImGui::EndChildFrame();
            }
        }
        ImGui::Text("Check progress with tensorboard:");
        if (ImGui::Button("Host & Open tensorboard", ImVec2(-1, 0)))
        {
            OpenTensorBoard();
        }

        if (IsTraining)
        {
            if (TrainingFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
            {
                TrainLog += Utils::GetCurrentTimeString(true) + " Training complete!\n";
                // TODO: write more log?
                
                // TODO: write model info to yaml?
                // need to calculate best iter and save its metrics
                /*

             fi = fitness(np.array(results).reshape(1, -1))               
def fitness(x):
    # Model fitness as a weighted combination of metrics
    w = [0.0, 0.0, 0.1, 0.9]  # weights for [P, R, mAP@0.5, mAP@0.5:0.95]
    return (x[:, :4] * w).sum(1)
                 */
                IsTraining = false;
            }
        }
    }

    void Train::Training()
    {
        if (IsTraining) return;
        TrainLog = Utils::GetCurrentTimeString(true) + " Start training... Check progross in Console or TensorBoard\n";

        // system can noy handle command quene...
        auto func = [=]()
        {
            system(SetPathScript.c_str());
            if (bApplyTransferLearning)
            {
                std::string PtFile = Setting::Get().YoloV7Path + "/" + ModelOptions[SelectedModelIdx] + "_training.pt";
                if (!std::filesystem::exists(PtFile))
                {
                    std::string AppPath = std::filesystem::current_path().u8string();
                    std::string DownloadWeightCommand = Setting::Get().PythonPath + "/python " + AppPath +
                        "/Scripts/DownloadWeight.py --model " + std::string(ModelOptions[SelectedModelIdx]);
                    std::ofstream ofs;
                    ofs.open("DownloadWeight.bat");
                    ofs << SetPathScript << " &^\n" << DownloadWeightCommand;
                    ofs.close();
                    system("DownloadWeight.bat");
                }
            }
            std::ofstream ofs;
            ofs.open("Train.bat");
            ofs << SetPathScript << " &^\n" << TrainScript;
            ofs.close();
            system("Train.bat");
        };


        IsTraining = true;
        TrainingFuture = std::async(std::launch::async, func);
    }


    void Train::OpenTensorBoard()
    {
        if (!HasHostTensorBoard)
        {
            // make host only once
            const char host[150] =
                " K:/Python/python-3.10.8-embed-amd64/Scripts/tensorboard --logdir L:/IFCS_DEV_PROJECTS/Great/Models";
            auto func = [=]()
            {
                system(host);
            };
            std::thread t(func);
            t.detach();
            HasHostTensorBoard = true;
        }
        char url[100] = "http://localhost:6006/";
        ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    }

    void Train::UpdateTrainScript()
    {
        SetPathScript = "cd " + Setting::Get().YoloV7Path;
        TrainScript = "";
        TrainScript += Setting::Get().PythonPath + "/python train.py";
        TrainScript += " --weights ";
        if (bApplyTransferLearning)
            TrainScript += std::string(ModelOptions[SelectedModelIdx]) + "_training.pt";
        else
            TrainScript += "''";
        TrainScript += " --cfg cfg/training/" + std::string(ModelOptions[SelectedModelIdx]) + ".yaml";
        TrainScript += " --data " + Setting::Get().ProjectPath + "/Data/" + SelectedTrainingSet.Name + "/" + SelectedTrainingSet.Name + ".yaml";
        TrainScript += " --hyp data/hyp.scratch." + std::string(HypOptions[SelectedHypIdx]) + ".yaml";
        TrainScript += " --epochs " + std::to_string(Epochs);
        TrainScript += " --batch-size " + std::to_string(BatchSize);
        TrainScript += " --img-size " + std::to_string(ImageSize[0]) + " " + std::to_string(ImageSize[1]);
        TrainScript += " --workers 1 --device 0";
        TrainScript += " --project " + Setting::Get().ProjectPath + "/Models";
        TrainScript += " --name " + std::string(ModelName);

        TensorBoardHostCommand = "";
        TensorBoardHostCommand += Setting::Get().PythonPath + "/Scripts/tensorboard --logdir " +
            Setting::Get().ProjectPath + "/Models";
    }

}
