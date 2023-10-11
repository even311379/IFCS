#include "IFCS_Types.h"
#include <yaml-cpp/yaml.h>

// // #include "Modals.h"
#include "Setting.h"
#include "Utils.h"
#include "Style.h"



namespace IFCS
{
    void FClipInfo::Update(const int& NewFrameCount, const float& NewFPS, const int& NewWidth, const int& NewHeight)
    {
        FrameCount = NewFrameCount;
        FPS = NewFPS;
        Width = NewWidth;
        Height = NewHeight;
    }

    std::string FClipInfo::GetClipFileName() const
    {
        std::vector<std::string> temp = Utils::Split(ClipPath, '/');
        return temp.back();
    }

    std::string FClipInfo::GetRelativePath() const
    {
        if (!Setting::Get().ProjectIsLoaded) return ClipPath;
        if (ClipPath.empty()) return ClipPath;
        return ClipPath.substr(Setting::Get().ProjectPath.size() + 7);
    }

    void FClipInfo::MakeDetailWidget()
    {
        const float Offset = 140.f;
        ImGui::Text("Frames:");
        ImGui::SameLine(Offset);
        ImGui::Text("%d", FrameCount);
        ImGui::Text("FPS:");
        ImGui::SameLine(Offset);
        ImGui::Text("%.2f", FPS);
        ImGui::Text("Size");
        ImGui::SameLine(Offset);
        ImGui::Text("%d x %d", Width, Height);
    }

    void FImageInfo::Update(const int& NewWidth, const int& NewHeight)
    {
        Width = NewWidth;
        Height = NewHeight;
    }

    std::string FImageInfo::GetFileName() const
    {
        std::vector<std::string> temp = Utils::Split(ImagePath, '/');
        return temp.back();
    }

    std::string FImageInfo::GetRelativePath() const
    {
        if (!Setting::Get().ProjectIsLoaded) return ImagePath;
        if (ImagePath.empty()) return ImagePath;
        return ImagePath.substr(Setting::Get().ProjectPath.size() + 8);
    }

    void FImageInfo::MakeDetailWidget()
    {
        const float Offset = 140.f;
        ImGui::Text("Size");
        ImGui::SameLine(Offset);
        ImGui::Text("%d x %d", Width, Height);
    }

    FCategory::FCategory(std::string NewDisplayName)
    {
        DisplayName = NewDisplayName;
        Color = Utils::RandomPickColor(Setting::Get().Theme);
    }

    FCategory::FCategory(YAML::Node InputNode)
    {
        Deserialize(InputNode);
    }

    YAML::Node FCategory::Serialize() const
    {
        YAML::Node OutNode;
        OutNode["DisplayName"] = DisplayName;
        OutNode["Color"] = ImGui::ColorConvertFloat4ToU32(Color);
        OutNode["ParentID"] = std::to_string(ParentID);
        return OutNode;
    }

    void FCategory::Deserialize(YAML::Node InputNode)
    {
        DisplayName = InputNode["DisplayName"].as<std::string>();
        Color = ImGui::ColorConvertU32ToFloat4(InputNode["Color"].as<unsigned int>());
        ParentID = InputNode["ParentID"].as<uint64_t>();
    }

    FAnnotation::FAnnotation(UUID InCategory, std::array<float, 4> InXYWH)
        : CategoryID(InCategory), XYWH(InXYWH)
    {
    }

    FAnnotation::FAnnotation(YAML::Node InputNode)
    {
        Deserialize(InputNode);
    }

    YAML::Node FAnnotation::Serialize() const
    {
        YAML::Node OutNode;
        OutNode["CategoryID"] = std::to_string(CategoryID);
        std::array<float, 4> ToSave = XYWH;
        ToSave[0] /= WorkArea.x;
        ToSave[1] /= WorkArea.y;
        ToSave[2] /= WorkArea.x;
        ToSave[3] /= WorkArea.y;
        OutNode["XYWH"] = ToSave;
        return OutNode;
    }

    void FAnnotation::Deserialize(YAML::Node InputNode)
    {
        CategoryID = InputNode["CategoryID"].as<uint64_t>();
        XYWH = InputNode["XYWH"].as<std::array<float, 4>>();
        XYWH[0] *= WorkArea.x;
        XYWH[1] *= WorkArea.y;
        XYWH[2] *= WorkArea.x;
        XYWH[3] *= WorkArea.y;
    }

    // TODO: XYWH is not saved as 0 ~ 1 format, but real pixel size now...
    void FAnnotation::Pan(std::array<float, 2> Changed)
    {
        if ((XYWH[0] + Changed[0] - 0.5f * XYWH[2] >= 0.f) && (XYWH[0] + Changed[0] + 0.5f * XYWH[2] <= WorkArea.x))
            XYWH[0] += Changed[0];
        if (((XYWH[1] + Changed[1] - 0.5f * XYWH[3] >= 0.f) && XYWH[1] + Changed[1] + 0.5f * XYWH[3] <= WorkArea.y))
            XYWH[1] += Changed[1];
    }

    void FAnnotation::Resize(EBoxCorner WhichCorner, std::array<float, 2> Changed)
    {
        ImVec2 NewCornerPos;
        switch (WhichCorner)
        {
        case EBoxCorner::TopLeft:
            if (XYWH[2] - Changed[0] < 10) return;
            if (XYWH[3] - Changed[1] < 10) return;
            NewCornerPos.x = XYWH[0] + 0.5f * Changed[0] - 0.5f * (XYWH[2] - Changed[0]);
            NewCornerPos.y = XYWH[1] + 0.5f * Changed[1] - 0.5f * (XYWH[3] - Changed[1]);
            if (NewCornerPos.x < 0.f) return;
            if (NewCornerPos.y < 0.f) return;
            XYWH[2] -= Changed[0];
            XYWH[3] -= Changed[1];
            break;
        case EBoxCorner::BottomLeft:
            if (XYWH[2] - Changed[0] < 10) return;
            if (XYWH[3] + Changed[1] < 10) return;
            NewCornerPos.x = XYWH[0] + 0.5f * Changed[0] - 0.5f * (XYWH[2] - Changed[0]);
            NewCornerPos.y = XYWH[1] + 0.5f * Changed[1] + 0.5f * (XYWH[3] + Changed[1]);
            if (NewCornerPos.x < 0.f) return;
            if (NewCornerPos.y >= WorkArea.y) return;
            XYWH[2] -= Changed[0];
            XYWH[3] += Changed[1];
            break;
        case EBoxCorner::BottomRight:
            if (XYWH[2] + Changed[0] < 10) return;
            if (XYWH[3] + Changed[1] < 10) return;
            NewCornerPos.x = XYWH[0] + 0.5f * Changed[0] + 0.5f * (XYWH[2] + Changed[0]);
            NewCornerPos.y = XYWH[1] + 0.5f * Changed[1] + 0.5f * (XYWH[3] + Changed[1]);
            if (NewCornerPos.x >= WorkArea.x) return;
            if (NewCornerPos.y >= WorkArea.y) return;
            XYWH[2] += Changed[0];
            XYWH[3] += Changed[1];
            break;
        case EBoxCorner::TopRight:
            if (XYWH[2] + Changed[0] < 10) return;
            if (XYWH[3] - Changed[1] < 10) return;
            NewCornerPos.x = XYWH[0] + 0.5f * Changed[0] + 0.5f * (XYWH[2] + Changed[0]);
            NewCornerPos.y = XYWH[1] + 0.5f * Changed[1] - 0.5f * (XYWH[3] - Changed[1]);
            if (NewCornerPos.x > WorkArea.x) return;
            if (NewCornerPos.y < 0.f) return;
            XYWH[2] += Changed[0];
            XYWH[3] -= Changed[1];
            break;
        }
        XYWH[0] += Changed[0] / 2;
        XYWH[1] += Changed[1] / 2;
    }


    std::string FAnnotation::GetExportTxt(size_t CID) const
    {
        std::array<float, 4> Out = XYWH;
        Out[0] /= WorkArea.x;
        Out[1] /= WorkArea.y;
        Out[2] /= WorkArea.x;
        Out[3] /= WorkArea.y;

        std::string OutString = std::to_string(CID);
        for (int i = 0; i < 4; i++)
        {
            OutString += " " + std::to_string(Out[i]);
        }
        return OutString;
    }

    FAnnotationSave::FAnnotationSave(YAML::Node InputNode)
    {
        Deserialize(InputNode);
    }

    void FAnnotationSave::Deserialize(YAML::Node InputNode)
    {
        AnnotationData.clear();
        for (YAML::const_iterator it=InputNode.begin(); it!=InputNode.end(); ++it)
        {
            if (it->first.as<std::string>() == "UpdateTime")
            {
                UpdateTime = it->second.as<std::string>();
            }
            else if (it->first.as<std::string>() == "IsReady")
            {
                IsReady = it->second.as<bool>();
            }
            else
            {
                AnnotationData[it->first.as<uint64_t>()] = FAnnotation(it->second);
            }
        }
    }

    YAML::Node FAnnotationSave::Serialize() const
    {
        YAML::Node Out;
        Out["UpdateTime"] = UpdateTime;
        Out["IsReady"] = IsReady;
        for (auto [UID, Ann] : AnnotationData)
            Out[uint64_t(UID)] = Ann.Serialize();
        return Out;
    }

    FTrainingSetDescription::FTrainingSetDescription(const std::string& InName, const YAML::Node& InputNode)
    {
        Deserialize(InName, InputNode);
    }


    YAML::Node FTrainingSetDescription::Serialize()
    {
        YAML::Node OutNode;
        OutNode["Categories"] = Categories;
        OutNode["CreationTime"] = CreationTime;
        OutNode["IncludeClips"] = IncludeClips;
        OutNode["IncludeImageFolders"] = IncludeImageFolders;
        OutNode["Size"] = Size;
        OutNode["Split"] = Split;
        OutNode["TotalImagesExported"] = TotalImagesExported;
        return OutNode;
    }

    void FTrainingSetDescription::Deserialize(const std::string& InName, const YAML::Node& InputNode)
    {
        Name = InName;
        Categories = InputNode["Categories"].as<std::vector<std::string>>();
        CreationTime = InputNode["CreationTime"].as<std::string>();
        IncludeClips = InputNode["IncludeClips"].as<std::vector<std::string>>();
        IncludeImageFolders = InputNode["IncludeImageFolders"].as<std::vector<std::string>>();
        Size = InputNode["Size"].as<std::array<int, 2>>();
        Split = InputNode["Split"].as<std::array<float, 3>>();
        TotalImagesExported = InputNode["TotalImagesExported"].as<int>();
    }

    void FTrainingSetDescription::MakeDetailWidget()
    {
        const float Width = ImGui::GetContentRegionAvail().x;
        ImGui::TextWrapped(CreationTime.c_str());
        const size_t RelOffset = Setting::Get().ProjectPath.size() + 7;
        if (!IncludeClips.empty())
        {
            ImGui::Text("Included Clips:");
            ImGui::BeginChildFrame(ImGui::GetID("DCs"), ImVec2(Width, ImGui::GetTextLineHeightWithSpacing() * 2));
            for (const std::string& C : IncludeClips)
            {
                std::string NoPath = C.substr(RelOffset);
                ImGui::Text(NoPath.c_str());
            }
            ImGui::EndChildFrame();
        }
        if (!IncludeImageFolders.empty())
        {
            ImGui::Text("Included Image folders:");
            ImGui::BeginChildFrame(ImGui::GetID("DIFs"), ImVec2(Width, ImGui::GetTextLineHeightWithSpacing() * 2));
            for (const std::string& C : IncludeImageFolders)
            {
                std::string NoPath = C.substr(RelOffset + 1);
                if (NoPath.empty())
                    NoPath = "(Root)"; 
                ImGui::Text(NoPath.c_str());
            }
            ImGui::EndChildFrame();
        }
        // Cats
        std::string OutCatTxt;
        for (const std::string& Cat : Categories)
        {
            OutCatTxt += Cat;
            if (Cat != Categories.back())
                OutCatTxt += ", ";
        }
        ImGui::TextWrapped("Categories:\n  [%s]", OutCatTxt.c_str());

        // Split and total export
        ImGui::TextWrapped("Train: %d, Valid: %d, Test: %d", int(Split[0] * TotalImagesExported),
                           int(Split[1] * TotalImagesExported), int(Split[2] * TotalImagesExported));

        // Delete button
        ImGui::PushStyleColor(ImGuiCol_Button, Style::RED(400, Setting::Get().Theme));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Style::RED(600, Setting::Get().Theme));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Style::RED(700, Setting::Get().Theme));
        if (ImGui::Button("Delete", {128, 0}))
        {
            // TODO: modal linux
            // Modals::Get().IsModalOpen_Delete = true;
        }
        ImGui::PopStyleColor(3);
    }

    FModelDescription::FModelDescription(const std::string& InName, const YAML::Node& InputNode)
    {
        Deserialize(InName, InputNode);
    }

    YAML::Node FModelDescription::Serialize() const
    {
        YAML::Node OutNode;
        OutNode["Categories"] = Categories;
        OutNode["CreationTime"] = CreationTime;
        OutNode["ModelType"] = ModelType;
        OutNode["SourceTrainingSet"] = SourceTrainingSet;
        OutNode["Precision"] = Precision;
        OutNode["Recall"] = Recall;
        OutNode["mAP5"] = mAP5;
        OutNode["mAP5_95"] = mAP5_95;
        return OutNode;
    }

    void FModelDescription::Deserialize(const std::string& InName, const YAML::Node& InputNode)
    {
        Name = InName;
        Categories = InputNode["Categories"].as<std::vector<std::string>>();
        CreationTime = InputNode["CreationTime"].as<std::string>();
        ModelType = InputNode["ModelType"].as<std::string>();
        SourceTrainingSet = InputNode["SourceTrainingSet"].as<std::string>();
        Precision = InputNode["Precision"].as<float>();
        Recall = InputNode["Recall"].as<float>();
        mAP5 = InputNode["mAP5"].as<float>();
        mAP5_95 = InputNode["mAP5_95"].as<float>();
    }

    void FModelDescription::MakeDetailWidget()
    {
        ImGui::TextWrapped(CreationTime.c_str());
        // Cats
        std::string OutCatTxt;
        for (const std::string& Cat : Categories)
        {
            OutCatTxt += Cat;
            if (Cat != Categories.back())
                OutCatTxt += ", ";
        }
        const float Offset = 140.f;
        ImGui::TextWrapped("Categories:\n  [%s]", OutCatTxt.c_str());
        ImGui::Text("Model Type:");
        ImGui::SameLine(Offset);
        ImGui::Text(ModelType.c_str());
        ImGui::Text("Data Set:");
        ImGui::SameLine(Offset);
        ImGui::Text(SourceTrainingSet.c_str());
        ImGui::Text("Precision:");
        ImGui::SameLine(Offset);
        ImGui::Text("%.3f", Precision);
        ImGui::Text("Recall:");
        ImGui::SameLine(Offset);
        ImGui::Text("%.3f", Recall);
        ImGui::Text("mAP@.5:");
        ImGui::SameLine(Offset);
        ImGui::Text("%.3f", mAP5);
        ImGui::Text("mAP@.5:.95:");
        ImGui::SameLine(Offset);
        ImGui::Text("%.3f", mAP5_95);
        
        // Delete button
        ImGui::PushStyleColor(ImGuiCol_Button, Style::RED(400, Setting::Get().Theme));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Style::RED(600, Setting::Get().Theme));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Style::RED(700, Setting::Get().Theme));
        if (ImGui::Button("Delete", {128, 0}))
        {
            // TODO: modal linux
            // Modals::Get().IsModalOpen_Delete = true;
        }
        ImGui::PopStyleColor(3);
    }

    FDetectionDescription::FDetectionDescription(const std::string& InName, const YAML::Node& InputNode)
    {
        Deserialize(InName, InputNode);
    }


    YAML::Node FDetectionDescription::Serialize() const
    {
        YAML::Node OutNode;
        OutNode["Categories"] = Categories;
        OutNode["CreationTime"] = CreationTime;
        OutNode["SourceModel"] = SourceModel;
        OutNode["TargetClip"] = TargetClip;
        OutNode["Confidence"] = Confidence;
        OutNode["IOU"] = IOU;
        OutNode["ImageSize"] = ImageSize;
        return OutNode;
    }

    void FDetectionDescription::Deserialize(const std::string& InName, const YAML::Node& InputNode)
    {
        Name = InName;
        Categories = InputNode["Categories"].as<std::vector<std::string>>();
        CreationTime = InputNode["CreationTime"].as<std::string>();
        SourceModel = InputNode["SourceModel"].as<std::string>();
        TargetClip = InputNode["TargetClip"].as<std::string>();
        Confidence = InputNode["Confidence"].as<float>();
        IOU = InputNode["IOU"].as<float>();
        ImageSize = InputNode["ImageSize"].as<int>();
    }


    void FDetectionDescription::MakeDetailWidget()
    {
        ImGui::TextWrapped(CreationTime.c_str());
        // Cats
        std::string OutCatTxt;
        for (const std::string& Cat : Categories)
        {
            OutCatTxt += Cat;
            if (Cat != Categories.back())
                OutCatTxt += ", ";
        }
        ImGui::TextWrapped("Categories: \n  [%s]", OutCatTxt.c_str());
        const float Offset = 140.f;
        ImGui::Text("Source Model:");
        ImGui::SameLine(Offset);
        ImGui::Text("%s", SourceModel.c_str());
        ImGui::Text("Target Clip:");
        ImGui::SameLine(Offset);
        ImGui::Text("%s", TargetClip.c_str());
        ImGui::Text("Confidence:");
        ImGui::SameLine(Offset);
        ImGui::Text("%.2f", Confidence);
        ImGui::Text("IOU:");
        ImGui::SameLine(Offset);
        ImGui::Text("%.2f", IOU);
        ImGui::Text("Image Size:");
        ImGui::SameLine(Offset);
        ImGui::Text("%d", ImageSize);
        
        // Delete button
        ImGui::PushStyleColor(ImGuiCol_Button, Style::RED(400, Setting::Get().Theme));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Style::RED(600, Setting::Get().Theme));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Style::RED(700, Setting::Get().Theme));
        if (ImGui::Button("Delete", {128, 0}))
        {
            // TODO: modal linux
            // Modals::Get().IsModalOpen_Delete = true;
        }
        ImGui::PopStyleColor(3);
    }

    float FLabelData::Distance(const FLabelData& Other, const int Width, const int Height) const
    {
        return Utils::Distance(X * Width, Y * Height, Other.X * Width, Other.Y * Height);
    }

    float FLabelData::GetApproxBodySize(int WPixels, int HPixels) const
    {
        return WPixels * Width * HPixels * Height;
    }

    int FIndividualData::GetEnterFrame() const
    {
        return Info.begin()->first;
    }

    int FIndividualData::GetLeaveFrame() const
    {
        return std::prev(Info.end())->first;
    }

    float FIndividualData::GetApproxBodySize() const
    {
        std::vector<float> S;
        for (const auto& [k, v] : Info)
            S.push_back(v.GetApproxBodySize(Width, Height));
        return Utils::Mean(S);
    }

    float FIndividualData::GetApproxSpeed() const
    {
        if (Info.size() == 1) return 0.f;
        std::vector<float> S;
        int i = 0;
        int LastF;
        FLabelData LastLabel;
        for (const auto& [F, L] : Info)
        {
            if (i != 0)
            {
                S.push_back(Utils::Distance(L.X * Width, L.Y * Height, LastLabel.X * Width, LastLabel.Y * Height)
                    / (float)(F - LastF));
            }
            LastF = F;
            LastLabel = L;
            i++;
        }
        return Utils::Mean(S);
    }

    std::string FIndividualData::GetName() const
    {
        float MaxConf = 0.f;
        std::string Out = OtherName;
        for (auto& [F, L] : Info)
        {
            if (L.Conf < MaxConf) continue;
            if (CategoryNames[L.CatID] != OtherName)
            {
                MaxConf = L.Conf;
                Out = CategoryNames[L.CatID];
            }
        }
        return Out;
    }

    int FIndividualData::CompareWithSortSpecs(const void* lhs, const void* rhs)
    {
        const FIndividualData* a = (const FIndividualData*)lhs;
        const FIndividualData* b = (const FIndividualData*)rhs;
        for (int n = 0; n < CurrentSortSpecs->SpecsCount; n++)
        {
            const ImGuiTableColumnSortSpecs* SortSpecs = &CurrentSortSpecs->Specs[n];
            int delta = 0;
            switch (SortSpecs->ColumnUserID)
            {
            case IndividualColumnID_Category:
                delta = a->GetName().compare(b->GetName());
                break;
            case IndividualColumnID_IsPassed:
                delta = a->IsCompleted && b->IsCompleted;
                break;
            case IndividualColumnID_ApproxSpeed:
                delta = (int)a->GetApproxSpeed() - (int)b->GetApproxSpeed();
                break;
            case IndividualColumnID_ApproxBodySize:
                delta = (int)a->GetApproxBodySize() - (int)b->GetApproxBodySize();
                break;
            case IndividualColumnID_EnterFrame:
                delta = a->GetEnterFrame() - b->GetEnterFrame();
                break;
            case IndividualColumnID_LeaveFrame:
                delta = a->GetLeaveFrame() - b->GetLeaveFrame();
                break;
            default: break;
            }
            if (delta > 0)
                return SortSpecs->SortDirection == ImGuiSortDirection_Ascending ? +1 : -1;
            if (delta < 0)
                return SortSpecs->SortDirection == ImGuiSortDirection_Ascending ? -1 : +1;
        }
        return a->GetName().compare(b->GetName());
    }

    FCameraSetup::FCameraSetup(const YAML::Node& InputNode)
    {
        Deserialize(InputNode);
    }

    YAML::Node FCameraSetup::Serialize() const
    {
        YAML::Node Out;
        Out["CameraName"] = CameraName;
        Out["ModelName"] = ModelName;
        Out["ModelFilePath"] = ModelFilePath;
        Out["ImageSize"] = ImageSize;
        Out["Confidence"] = Confidence;
        Out["IOU"] = IOU;
        Out["ShouldApplyDetectionROI"] = ShouldApplyDetectionROI;
        std::array<float, 4> Arr{};
        for (int i =0; i < 4; i++)
            Arr[i] = DetectionROI[i];
        Out["DetectionROI"] = Arr;
        Out["IsPassVertical"] = IsPassVertical;
        std::array<float, 2> Arr2{};
        for (int i = 0; i < 2; i++)
            Arr2[i] = FishwayStartEnd[i];
        Out["FishwayStartEnd"] = Arr2;
        Out["UseFishwayMiddle"] = UseFishwayMiddle;
        Out["FishwayMiddle"] = FishwayMiddle;
        Out["ShouldEnableSpeedThreshold"] = ShouldEnableSpeedThreshold;
        Out["SpeedThreshold"] = SpeedThreshold;
        Out["ShouldEnableBodySizeThreshold"] = ShouldEnableBodySizeThreshold;
        Out["BodySizeThreshold"] = BodySizeThreshold;
        Out["FrameBufferSize"] = FrameBufferSize;
        Out["OtherCategoryName"] = OtherCategoryName;
        return Out;
    }

    void FCameraSetup::Deserialize(const YAML::Node& InputNode)
    {
        CameraName = InputNode["CameraName"].as<std::string>();
        ModelName = InputNode["ModelName"].as<std::string>();
        ModelFilePath = InputNode["ModelFilePath"].as<std::string>();
        ImageSize = InputNode["ImageSize"].as<int>();
        Confidence = InputNode["Confidence"].as<float>();
        IOU = InputNode["IOU"].as<float>();
        ShouldApplyDetectionROI = InputNode["ShouldApplyDetectionROI"].as<bool>();
        const auto arr = InputNode["DetectionROI"].as<std::array<float, 4>>();
        for (int i = 0; i < 4; i++)
        {
            DetectionROI[i] = arr[i];
        }
        IsPassVertical = InputNode["IsPassVertical"].as<bool>();
        const auto arr2 = InputNode["FishwayStartEnd"].as<std::array<float, 2>>();
        for (int i = 0; i < 2; i++)
        {
            FishwayStartEnd[i] = arr2[i];
        }
        UseFishwayMiddle = InputNode["UseFishwayMiddle"].as<bool>();
        FishwayMiddle = InputNode["FishwayMiddle"].as<float>();
        ShouldEnableSpeedThreshold = InputNode["ShouldEnableSpeedThreshold"].as<bool>();
        SpeedThreshold = InputNode["SpeedThreshold"].as<float>();
        ShouldEnableBodySizeThreshold = InputNode["ShouldEnableBodySizeThreshold"].as<bool>();
        BodySizeThreshold = InputNode["BodySizeThreshold"].as<float>();
        OtherCategoryName = InputNode["OtherCategoryName"].as<std::string>();
        FrameBufferSize = InputNode["FrameBufferSize"].as<int>();
    }


    FDeploymentData::FDeploymentData(const YAML::Node& InputNode)
    {
        Deserialize(InputNode);
    }

    YAML::Node FDeploymentData::Serialize() const
    {
        YAML::Node Out;
        Out["TaskInputDir"] = TaskInputDir;
        Out["TaskOutputDir"] = TaskOutputDir;
        Out["UseSameCameraSetup"] = UseSameCameraSetup;
        for (const auto& Camera : Cameras)
        {
            Out["Cameras"].push_back(Camera.Serialize());
        }
        Out["IsTaskStartNow"] = IsTaskStartNow;
        std::array<int, 2> Arr{};
        for (int i = 0; i < 2; i++)
            Arr[i] = ScheduledRuntime[i];
        Out["ScheduledRuntime"] = Arr;
        Out["IsRunSpecifiedDates"] = IsRunSpecifiedDates;
        for (const auto& Date : RunDates)
        {
            YAML::Node DateNode;
            DateNode["Year"] = Date.tm_year;
            DateNode["Month"] = Date.tm_mon;
            DateNode["Day"] = Date.tm_mday;
            Out["RunDates"].push_back(DateNode);
        }
        Out["StartDate"]["Year"] = StartDate.tm_year;
        Out["StartDate"]["Month"] = StartDate.tm_mon;
        Out["StartDate"]["Day"] = StartDate.tm_mday;
        Out["EndDate"]["Year"] = EndDate.tm_year;
        Out["EndDate"]["Month"] = EndDate.tm_mon;
        Out["EndDate"]["Day"] = EndDate.tm_mday;
        Out["ShouldCollectHighlight"] = ShouldCollectHighlight;

        Out["DetectionFrequency"] = DetectionPeriodInSecond;
        Out["PassCountDuration"] = PassCountDurationInMinutes;
        Out["Server"] = ServerAddress;
        Out["ServerAccount"] = ServerAccount;
        Out["ServerPassword"] = ServerPassword;
        Out["ShouldAutoSendServer"] = ShouldAutoSendServer;
        
        return Out;
    }

    void FDeploymentData::Deserialize(const YAML::Node& InputNode)
    {
        // deserialize all
        TaskInputDir = InputNode["TaskInputDir"].as<std::string>();
        TaskOutputDir = InputNode["TaskOutputDir"].as<std::string>();
        UseSameCameraSetup = InputNode["UseSameCameraSetup"].as<bool>();
        Cameras.clear();
        for (YAML::const_iterator it = InputNode["Cameras"].begin(); it != InputNode["Cameras"].end(); ++it)
        {
            Cameras.emplace_back(it->as<YAML::Node>());
        }
        IsTaskStartNow = InputNode["IsTaskStartNow"].as<bool>();
        const auto temp = InputNode["ScheduledRuntime"].as<std::array<int, 2>>();
        ScheduledRuntime[0] = temp[0];
        ScheduledRuntime[1] = temp[1];
        IsRunSpecifiedDates = InputNode["IsRunSpecifiedDates"].as<bool>();
        RunDates.clear();
        for (YAML::const_iterator it = InputNode["RunDates"].begin(); it != InputNode["RunDates"].end(); ++it)
        {
            tm Date;
            Date.tm_year = it->as<YAML::Node>()["Year"].as<int>();
            Date.tm_mon = it->as<YAML::Node>()["Month"].as<int>();
            Date.tm_mday = it->as<YAML::Node>()["Day"].as<int>();
            RunDates.push_back(Date);
        }
        StartDate = tm();
        StartDate.tm_year = InputNode["StartDate"]["Year"].as<int>();
        StartDate.tm_mon = InputNode["StartDate"]["Month"].as<int>();
        StartDate.tm_mday = InputNode["StartDate"]["Day"].as<int>();
        EndDate = tm();
        EndDate.tm_year = InputNode["EndDate"]["Year"].as<int>();
        EndDate.tm_mon = InputNode["EndDate"]["Month"].as<int>();
        EndDate.tm_mday = InputNode["EndDate"]["Day"].as<int>();
        ShouldCollectHighlight = InputNode["ShouldCollectHighlight"].as<bool>();
        DetectionPeriodInSecond = InputNode["DetectionFrequency"].as<int>();
        PassCountDurationInMinutes = InputNode["PassCountDuration"].as<int>();
        ServerAddress = InputNode["Server"].as<std::string>();
        ServerAccount = InputNode["ServerAccount"].as<std::string>();
        ServerPassword = InputNode["ServerPassword"].as<std::string>();
        ShouldAutoSendServer = InputNode["ShouldAutoSendServer"].as<bool>();
        
     }
}
