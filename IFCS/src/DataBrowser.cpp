#include "DataBrowser.h"
#include "Log.h"
#include "Style.h"
#include "Utils.h"
#include "Setting.h"

#include "IconFontCppHeaders/IconsFontAwesome5.h"
#include <spdlog/spdlog.h>
#include <shellapi.h>
#include <fstream>
#include <regex>
#include <yaml-cpp/yaml.h>
#include "opencv2/opencv.hpp"
#include "backends/imgui_impl_glfw.h"
#include <GLFW/glfw3.h>

#include "Annotation.h"


namespace IFCS
{

    // EAssetType LastSelectedAssetType = EAssetType::Clip;

    void DataBrowser::Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose)
    {
        Panel::Setup(InName, InShouldOpen, InFlags, InCanClose);
        SelectedTrainingSet = "";
        SelectedModel = "";
        SelectedDetection = "";
    }

    std::vector<std::string> DataBrowser::GetAllClips() const
    {
        std::vector<std::string> Out;
        for (const auto& p : std::filesystem::recursive_directory_iterator(
                 Setting::Get().ProjectPath + std::string("/Clips")))
        {
            if (!std::filesystem::is_directory(p))
            {
                bool pass = false;
                for (const std::string& Format : AcceptedClipsFormat)
                {
                    if (Utils::InsensitiveStringCompare(p.path().extension().string(), Format))
                    {
                        pass = true;
                        break;
                    }
                }
                if (pass)
                {
                    std::string full_path = p.path().u8string();
                    std::replace(full_path.begin(), full_path.end(), '\\', '/');
                    Out.push_back(full_path);
                }
            }
        }
        return Out;
    }

    std::vector<std::string> DataBrowser::GetAllImages() const
    {
        std::vector<std::string> Out;
        for (const auto& p : std::filesystem::recursive_directory_iterator(
                 Setting::Get().ProjectPath + std::string("/Images")))
        {
            if (!std::filesystem::is_directory(p))
            {
                bool pass = false;
                for (const std::string& Format : AcceptedImageFormat)
                {
                    if (Utils::InsensitiveStringCompare(p.path().extension().string(), Format))
                    {
                        pass = true;
                        break;
                    }
                }
                if (pass)
                {
                    std::string full_path = p.path().u8string();
                    std::replace(full_path.begin(), full_path.end(), '\\', '/');
                    Out.push_back(full_path);
                }
            }
        }
        return Out;
    }

    std::vector<std::string> DataBrowser::GetImageFolders() const
    {
        std::vector<std::string> AllImageFolders;
        AllImageFolders.emplace_back(Setting::Get().ProjectPath + "/Images/");
        for (const auto& p : std::filesystem::recursive_directory_iterator(
                 Setting::Get().ProjectPath + std::string("/Images")))
        {
            if (std::filesystem::is_directory(p))
            {
                std::string full_path = p.path().u8string();
                std::replace(full_path.begin(), full_path.end(), '\\', '/');
                AllImageFolders.push_back(full_path);
            }
        }
        return AllImageFolders;
    }

    void DataBrowser::DisplayFrame(int NewFrameNum, const std::string& ParticularClip)
    {
        CurrentFrame = NewFrameNum;
        if (!VideoFrames.count(CurrentFrame))
        {
            cv::VideoCapture Cap;
            if (ParticularClip.empty())
                Cap.open(SelectedClipInfo.ClipPath);
            else
            {
                Cap.open(ParticularClip);
            }
            cv::Mat Frame;
            Cap.set(cv::CAP_PROP_POS_FRAMES, CurrentFrame);
            Cap >> Frame;
            cv::resize(Frame, Frame, cv::Size(1280, 720)); // 16 : 9
            cv::cvtColor(Frame, Frame, cv::COLOR_BGR2RGB);
            Cap.release();
            MatToGL(Frame);
        }
        else
        {
            MatToGL(VideoFrames[NewFrameNum]);
        }
    }

    void DataBrowser::DisplayImage()
    {
        IsImageDisplayed = true;
        // release memory for video frames
        VideoFrames.clear();
        
        // imread and update info
        cv::Mat Img = cv::imread(DataBrowser::Get().SelectedImageInfo.ImagePath);

        if (Img.empty())
        {
            // Solution to unicode path in opencv: https://stackoverflow.com/a/43369056
            std::wstring wpath = Utils::ConvertUtf8ToWide(DataBrowser::Get().SelectedImageInfo.ImagePath);
            
            std::ifstream f(wpath, std::iostream::binary);
            // Get its size
            std::filebuf* pbuf = f.rdbuf();
            size_t size = pbuf->pubseekoff(0, f.end, f.in);
            pbuf->pubseekpos(0, f.in);

            // Put it in a vector
            std::vector<uchar> buffer(size);
            pbuf->sgetn((char*)buffer.data(), size);

            // Decode the vector
            Img = cv::imdecode(buffer, cv::IMREAD_COLOR);
        }
        SelectedImageInfo.Update(Img.cols, Img.rows);
        cv::cvtColor(Img, Img, cv::COLOR_BGR2RGB);
        cv::resize(Img, Img, cv::Size((int)WorkArea.x, (int)WorkArea.y));
        MatToGL(Img);
        Annotation::Get().GrabData();
    }

    void DataBrowser::PrepareVideo(bool& InLoadingStatus)
    {
        if (InLoadingStatus)
        {
            ForceCloseLoading = true;
            return;
        }
        IsImageDisplayed = false;
        // show frame 1 and update info
        CurrentFrame = 0;
        VideoStartFrame = 0;
        Annotation::Get().GrabData();
        cv::Mat Frame;
        cv::VideoCapture cap(SelectedClipInfo.ClipPath);
        int Width = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
        int Height = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
        float FPS = (float)cap.get(cv::CAP_PROP_FPS);
        int FrameCount = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
        VideoEndFrame = FrameCount - 1;
        SelectedClipInfo.Update(FrameCount, FPS, Width, Height);
        cap >> Frame;
        cv::resize(Frame, Frame, cv::Size(1280, 720)); // 16 : 9
        cv::cvtColor(Frame, Frame, cv::COLOR_BGR2RGB);
        cap.release();
        MatToGL(Frame);
        VideoFrames.clear();
        LoadingVideoBlock(InLoadingStatus, 0);
    }

    void DataBrowser::LoadingVideoBlock(bool& InLoadingStatus, int CurrentFrameNum, const std::string& ParticularClip)
    {
        // check which block to load and which to erase
        if (InLoadingStatus) return;
        if (VideoFrames.count(CurrentFrameNum)) return;
        InLoadingStatus = true;
        VideoFrames.clear();
        const int NewBlockStartIdx = CurrentFrameNum / Setting::Get().MaxCachedFramesSize;
        // async load frame with multi core?
        auto LoadVideo = [=](int Ith)
        {
            cv::VideoCapture Cap;
            if (ParticularClip.empty())
            {
                Cap.open(SelectedClipInfo.ClipPath);
            }
            else
            {
                Cap.open(ParticularClip);
            }
            int NumFramesToLoad = std::min(Setting::Get().MaxCachedFramesSize, (int)Cap.get(cv::CAP_PROP_FRAME_COUNT));
            int PerCoreSize = NumFramesToLoad / Setting::Get().CoresToUse;
            int Start = NewBlockStartIdx * NumFramesToLoad + Ith * PerCoreSize;
            int End =  Ith == Setting::Get().CoresToUse - 1?  (NewBlockStartIdx + 1) * NumFramesToLoad: Start + PerCoreSize + 1;
            Cap.set(cv::CAP_PROP_POS_FRAMES, Start);
            while (1)
            {
                if (ForceCloseLoading)
                {
                    spdlog::info("forced stop loading...");
                    break;
                }
                if (Start > End)
                {
                    break;
                }
                cv::Mat Frame;
                Cap.read(Frame);
                if (Frame.empty())
                {
                    break;
                }
                cv::resize(Frame, Frame, cv::Size((int)WorkArea.x, (int)WorkArea.y));
                cv::cvtColor(Frame, Frame, cv::COLOR_BGR2RGB);
                VideoFrames[Start] = Frame;
                Start++;
                // spdlog::info("Loading in thread {}", Ith);
            }
            Cap.release();
        };
        LoadingVideoTasks.clear();
        for (int T = 0; T < Setting::Get().CoresToUse; T++)
        {
            LoadingVideoTasks.emplace_back(std::async(std::launch::async, LoadVideo, T));
        }
    }

    static int Tick = 0;

    void DataBrowser::RenderContent()
    {
        if (!Setting::Get().ProjectIsLoaded) return;
        // Inside child window to have independent scroll
        const std::string ClipFolderPath = Setting::Get().ProjectPath + "/Clips";
        const std::string ImageFolderPath = Setting::Get().ProjectPath + "/Images";
        ImGui::PushFont(Setting::Get().TitleFont);
        ImGui::Text("Data Browser");
        ImGui::PopFont();
        ImVec2 ChildWindowSize = ImGui::GetContentRegionAvail();
        if (ShouldViewDetail)
            ChildWindowSize.y *= 0.70f;
        else
            ChildWindowSize.y *= 0.97f;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("DataBrowserWindow", ChildWindowSize, false, window_flags);
        {
            if (ImGui::TreeNode((std::string(ICON_FA_VIDEO) + " Clips").c_str()))
            {
                RecursiveClipTreeNodeGenerator(ClipFolderPath, 0);
                if (!HasAnyClip)
                {
                    if (ImGui::Button("Open Folder", ImVec2(-1, 0)))
                    {
                        ShellExecuteA(NULL, "open", (Setting::Get().ProjectPath + "/Clips").c_str(), NULL, NULL,
                                      SW_SHOWDEFAULT);
                    }
                }
                ImGui::Checkbox("Show not ready only?", &NeedReviewedOnly);
                ImGui::TreePop();
            }

            if (ImGui::TreeNode((std::string(ICON_FA_IMAGE) + " Images").c_str()))
            {
                RecursiveImageTreeNodeGenerator(ImageFolderPath, 0);
                if (!HasAnyImage)
                {
                    if (ImGui::Button("Open Folder", ImVec2(-1, 0)))
                    {
                        ShellExecuteA(NULL, "open", (Setting::Get().ProjectPath + "/Images").c_str(), NULL, NULL,
                                      SW_SHOWDEFAULT);
                    }
                }
                ImGui::Checkbox("Show not ready only?", &NeedReviewedOnly);
                ImGui::TreePop();
            }
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                char Send[128];
                strcpy(Send, (Setting::Get().ProjectPath + "/Images/").c_str());
                ImGui::SetDragDropPayload("ImageFolder", &Send, sizeof(Send));
                ImGui::Text("/ (Image Folder)");
                ImGui::EndDragDropSource();
            }
            if (ImGui::TreeNode((std::string(ICON_FA_TH) + " Training Sets").c_str()))
            {
                CreateSeletable_TrainingSets();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode((std::string(ICON_FA_MICROCHIP) + " Models").c_str()))
            {
                CreateSeletable_Models();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode((std::string(ICON_FA_WAVE_SQUARE) + " Detections").c_str()))
            {
                CreateSeletable_Detections();
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();
        if (ShouldViewDetail)
        {
            RenderDetailWidget();
        }
        else
        {
            if (ImGui::Button(LOCTEXT("Common.OpenProjectFolder"), ImVec2(-1, 0)))
            {
                ShellExecuteA(NULL, "open", Setting::Get().ProjectPath.c_str(), NULL, NULL,
                              SW_SHOWDEFAULT);
            }
        }

        if (Tick > 30)
        {
            Tick = 0;
            ImgAnnotationsToDisplay.clear();
            for (auto [FilePath, FrameSave] : Annotation::Get().Data)
            {
                if (FilePath.empty()) continue;
                for (const auto& Format : AcceptedImageFormat)
                {
                    if (Utils::InsensitiveStringCompare(FilePath.substr(FilePath.size() - 4), Format))
                    {
                        FAnnotationSave Save = FrameSave[0];
                        ImgAnnotationsToDisplay[FilePath] = FAnnotationToDisplay(Save.AnnotationData.size(), Save.IsReady);
                    }
                }
            }
        }
        Tick++;
    }


    // Copy and modify from https://discourse.dearimgui.org/t/how-to-mix-imgui-treenode-and-filesystem-to-print-the-current-directory-recursively/37
    void DataBrowser::RecursiveClipTreeNodeGenerator(const std::filesystem::path& Path, unsigned int Depth)
    {
        for (const auto& entry : std::filesystem::directory_iterator(Path))
        {
            if (std::filesystem::is_directory(entry.path()))
            {
                using namespace std::string_literals;
                std::string s = ICON_FA_FOLDER + " "s + entry.path().filename().u8string().c_str();
                if (ImGui::TreeNodeEx(s.c_str(), 0))
                {
                    RecursiveClipTreeNodeGenerator(entry, Depth + 1);
                    ImGui::TreePop();
                }
            }
            else
            {
                bool pass = false;
                for (const std::string& Format : AcceptedClipsFormat)
                {
                    if (Utils::InsensitiveStringCompare(entry.path().extension().string(), Format))
                    {
                        pass = true;
                        break;
                    }
                }
                if (!pass) return;
                HasAnyClip = true;
                if (Depth > 0)
                {
                    ImGui::Indent();
                }
                std::string ClipName = entry.path().filename().u8string();
                std::string FullClipPath = entry.path().u8string();
                std::replace(FullClipPath.begin(), FullClipPath.end(), '\\', '/');
                if (ImGui::Selectable(ClipName.c_str(), SelectedClipInfo.ClipPath == FullClipPath,
                                      ImGuiSelectableFlags_AllowDoubleClick))
                {
                    SelectedImageInfo.ImagePath = ""; // Deselect image...
                    SelectedClipInfo.ClipPath = FullClipPath;

                    ShouldViewDetail = true;
                    LastSelectedAssetType = EAssetType::Clip;
                    if (Setting::Get().ActiveWorkspace == EWorkspace::Data)
                    {
                        PrepareVideo(Annotation::Get().IsLoadingVideo);
                    }
                    else // only update info
                    {
                        cv::VideoCapture cap(SelectedClipInfo.ClipPath);
                        int Width = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
                        int Height = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
                        float FPS = (float)cap.get(cv::CAP_PROP_FPS);
                        int FrameCount = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
                        SelectedClipInfo.Update(FrameCount, FPS, Width, Height);
                        cap.release();
                    }
                }
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    char Send[128];
                    strcpy(Send, FullClipPath.c_str());
                    ImGui::SetDragDropPayload("Clip", &Send, sizeof(Send));
                    // for test now
                    ImGui::Text(Send);
                    ImGui::EndDragDropSource();
                }
                if (Setting::Get().ActiveWorkspace == EWorkspace::Data && SelectedClipInfo.ClipPath == FullClipPath)
                {
                    ImGui::Indent();
                    ImGui::Indent();
                    for (const auto& [k, v] : Annotation::Get().GetAnnotationToDisplay())
                    {
                        if (NeedReviewedOnly && v.IsReady) continue;
                        char buff[128];
                        const char* Icon = v.IsReady ? ICON_FA_CHECK : "";
                        sprintf(buff, "%d ...... (%d) %s", k, int(v.Count), Icon);
                        if (ImGui::Selectable(buff, k == CurrentFrame))
                        {
                            MoveFrame(k);
                        }
                    }
                    ImGui::Unindent();
                    ImGui::Unindent();
                }
                if (Depth > 0)
                {
                    ImGui::Unindent();
                }
            }
        }
    }

    // TODO: long long list of files will make it lag... need to fix? ... very very hard to fix... ignore it!
    void DataBrowser::RecursiveImageTreeNodeGenerator(const std::filesystem::path& Path, unsigned Depth)
    {
        for (const auto& entry : std::filesystem::directory_iterator(Path))
        {
            if (std::filesystem::is_directory(entry.path()))
            {
                using namespace std::string_literals;
                std::string s = ICON_FA_FOLDER + " "s + entry.path().filename().u8string().c_str();
                if (ImGui::TreeNodeEx(s.c_str(), 0))
                {
                    RecursiveImageTreeNodeGenerator(entry, Depth + 1);
                    ImGui::TreePop();
                }
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    std::string F = entry.path().u8string();
                    std::replace(F.begin(), F.end(), '\\', '/');
                    char Send[128];
                    strcpy(Send, F.c_str());
                    ImGui::SetDragDropPayload("ImageFolder", &Send, sizeof(Send));
                    const size_t RelativeImageFolderNameOffset = Setting::Get().ProjectPath.size() + 8;
                    // for test now
                    ImGui::Text("%s (Image folder)", F.substr(RelativeImageFolderNameOffset).c_str());
                    ImGui::EndDragDropSource();
                }
            }
            else
            {
                bool pass = false;
                for (const std::string& Format : AcceptedImageFormat)
                {
                    if (Utils::InsensitiveStringCompare(entry.path().extension().string(), Format))
                    {
                        pass = true;
                        break;
                    }
                }
                if (!pass) return;
                std::string FullImagePath = entry.path().u8string();
                std::string ImageName = entry.path().filename().u8string();
                std::replace(FullImagePath.begin(), FullImagePath.end(), '\\', '/');
                std::string RelPath = FullImagePath.substr(Setting::Get().ProjectPath.length());
                std::string LabelName;
                HasAnyImage = true;
                if (Depth > 0)
                {
                    ImGui::Indent();
                }
                if (ImgAnnotationsToDisplay.count(RelPath))
                {
                    const FAnnotationToDisplay AD = ImgAnnotationsToDisplay[RelPath];
                    if (NeedReviewedOnly && AD.IsReady) continue;
                    char buff[128];
                    const char* Icon = AD.IsReady ? ICON_FA_CHECK : "";
                    sprintf(buff, "%s ...... (%d) %s", ImageName.c_str(), int(AD.Count), Icon);
                    LabelName = buff;
                }
                else
                {
                    LabelName = ImageName;
                }
                if (!NeedReviewedOnly || ImgAnnotationsToDisplay.count(RelPath))
                {
                    if (ImGui::Selectable(LabelName.c_str(), SelectedImageInfo.ImagePath == FullImagePath,
                                          ImGuiSelectableFlags_AllowDoubleClick))
                    {
                        SelectedClipInfo.ClipPath = "";
                        SelectedImageInfo.ImagePath = FullImagePath;
                        ShouldViewDetail = true;
                        LastSelectedAssetType = EAssetType::Image;
                        if (Setting::Get().ActiveWorkspace == EWorkspace::Data)
                        {
                            DisplayImage();
                        }
                        else // only update info...
                        {
                            cv::Mat Img = cv::imread(SelectedImageInfo.ImagePath);
                            SelectedImageInfo.Update(Img.cols, Img.rows);
                        }
                    }
                    
                }

                if (Depth > 0)
                {
                    ImGui::Unindent();
                }
            }
        }
    }

    void DataBrowser::CreateSeletable_TrainingSets()
    {
        YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Data/TrainingSets.yaml");
        for (YAML::const_iterator it=Data.begin();it!=Data.end();++it)
        {
            std::string Name = it->first.as<std::string>();
            if (ImGui::Selectable(Name.c_str(), Name == SelectedTrainingSet))
            {
                SelectedTrainingSet = Name;
                TrainingSetDescription = FTrainingSetDescription(Name, it->second);
                ShouldViewDetail = true;
                LastSelectedAssetType = EAssetType::TrainingSet;
            }
            // TODO: block the drag source for now...
            // if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            // {
            //     char Send[128];
            //     strcpy(Send, Name.c_str());
            //     ImGui::SetDragDropPayload("TrainingSet", &Send, sizeof(Send));
            //     ImGui::Text(Send);
            //     ImGui::EndDragDropSource();
            // }
        }
    }

    void DataBrowser::CreateSeletable_Models()
    {
        YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Models/Models.yaml");
        for (YAML::const_iterator it = Data.begin(); it != Data.end(); ++it)
        {
            std::string Name = it->first.as<std::string>();
            if (ImGui::Selectable(Name.c_str(), Name == SelectedModel))
            {
                SelectedModel = Name;
                ModelDescription = FModelDescription(Name, it->second);
                ShouldViewDetail = true;
                LastSelectedAssetType = EAssetType::Model;
            }
            // TODO: drag source??
        }
    }

    void DataBrowser::CreateSeletable_Detections()
    {
        YAML::Node Data = YAML::LoadFile(Setting::Get().ProjectPath + "/Detections/Detections.yaml");
        for (YAML::const_iterator it = Data.begin(); it != Data.end(); ++it)
        {
            std::string Name = it->first.as<std::string>();
            if (ImGui::Selectable(Name.c_str(), Name == SelectedDetection))
            {
                SelectedDetection = Name;
                DetectionDescription = FDetectionDescription(Name, it->second);
                ShouldViewDetail = true;
                LastSelectedAssetType = EAssetType::Detection;
            }
            // TODO: drag source??
        }
    }

    void DataBrowser::RenderDetailWidget()
    {
        ImGui::BeginChild("AssetDetail");
        {
            float ContentWidth = ImGui::GetContentRegionAvail().x;
            ImVec2 Pos = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(Pos, Pos + ImVec2(ContentWidth, 5.f),
                                                      ImGui::ColorConvertFloat4ToU32(
                                                          Style::BLUE(400, Setting::Get().Theme)), 12.f);
            ImGui::Dummy(ImVec2(0, 2.f));
            ImGui::PushFont(Setting::Get().TitleFont);
            switch (LastSelectedAssetType)
            {
            case EAssetType::Clip:
                ImGui::Text("%s", SelectedClipInfo.GetClipFileName().c_str());
                break;
            case EAssetType::Image:
                ImGui::Text("%s", SelectedImageInfo.GetFileName().c_str());
                break;
            case EAssetType::TrainingSet:
                ImGui::Text("%s", TrainingSetDescription.Name.c_str());
                break;
            case EAssetType::Model:
                ImGui::Text("%s", ModelDescription.Name.c_str());
                break;
            case EAssetType::Detection:
                ImGui::Text("%s", DetectionDescription.Name.c_str());
                break;
            }
            ImGui::PopFont();
            ImGui::SameLine(ContentWidth - 32);
            if (ImGui::Button(ICON_FA_TIMES, ImVec2(32, 0)))
            {
                ShouldViewDetail = false;
            }
            // content
            switch (LastSelectedAssetType)
            {
            case EAssetType::Clip:
                SelectedClipInfo.MakeDetailWidget();
                break;
            case EAssetType::Image:
                SelectedImageInfo.MakeDetailWidget();
                break;
            case EAssetType::TrainingSet:
                TrainingSetDescription.MakeDetailWidget();
                break;
            case EAssetType::Model:
                ModelDescription.MakeDetailWidget();
                break;
            case EAssetType::Detection:
                DetectionDescription.MakeDetailWidget();
                break;
            }
        }
        ImGui::EndChild();
    }

    void DataBrowser::MatToGL(const cv::Mat& Frame)
    {
        glGenTextures(1, &LoadedFramePtr);
        glBindTexture(GL_TEXTURE_2D, LoadedFramePtr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); // Same
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Frame.cols, Frame.rows, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, Frame.data);
    }

    void DataBrowser::MoveFrame(int NewFrame)
    {
        if (!IsSelectAnyClipOrImg()) return;
        CurrentFrame = NewFrame;
        if (CurrentFrame < VideoStartFrame)  DataBrowser::Get().CurrentFrame = VideoStartFrame;
        if (CurrentFrame > VideoEndFrame) DataBrowser::Get().CurrentFrame = VideoEndFrame;
        DisplayFrame(DataBrowser::Get().CurrentFrame);
    }
}
