#include "Common.h"
#include <yaml-cpp/yaml.h>

#include "Setting.h"
#include "Utils.h"

namespace IFCS
{
    std::string FClipInfo::GetClipFileName() const
    {
        std::vector<std::string> temp = Utils::Split(ClipPath, '/');
        return temp.back();
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


    std::string FAnnotation::GetExportTxt(std::unordered_map<UUID, int>& CategoryChecker,
                                          const FAnnotationShiftData& InShiftData) const
    {
        // TODO: ignore shift data for now... MUST add later...
        std::array<float, 4> ToShift = XYWH;
        ToShift[0] /= WorkArea.x;
        ToShift[1] /= WorkArea.y;
        ToShift[2] /= WorkArea.x;
        ToShift[3] /= WorkArea.y;

        std::string OutString = std::to_string(CategoryChecker[CategoryID]);
        for (int i = 0; i < 4; i++)
        {
            OutString += " " + std::to_string(ToShift[i]);
        }
        return OutString;
    }


    FTrainingSetDescription::FTrainingSetDescription(YAML::Node InputNode)
    {
        Deserialize(InputNode);
    }

    YAML::Node FTrainingSetDescription::Serialize()
    {
        YAML::Node OutNode;
        OutNode["Name"] = Name;
        OutNode["CreationTime"] = CreationTime;
        OutNode["IncludeClips"] = IncludeClips;
        OutNode["IncludeImageFolders"] = IncludeImageFolders;
        OutNode["Size"] = Size;
        OutNode["Split"] = Split;
        OutNode["NumDuplicates"] = NumDuplicates;
        OutNode["TotalImagesExported"] = TotalImagesExported;
        OutNode["AppliedAugmentationDescription"] = AppliedAugmentationDescription;
        return OutNode;
    }

    void FTrainingSetDescription::Deserialize(YAML::Node InputNode)
    {
        Name = InputNode["Name"].as<std::string>();
        CreationTime = InputNode["CreationTime"].as<std::string>();
        IncludeClips = InputNode["IncludeClips"].as<std::vector<std::string>>();
        IncludeImageFolders = InputNode["IncludeImageFolders"].as<std::vector<std::string>>();
        Size = InputNode["Size"].as<std::array<int, 2>>();
        Split = InputNode["Split"].as<std::array<float, 3>>();
        NumDuplicates = InputNode["NumDuplicates"].as<int>();
        TotalImagesExported = InputNode["TotalImagesExported"].as<int>();
        AppliedAugmentationDescription = InputNode["AppliedAugmentationDescription"].as<std::string>();
    }

    void FTrainingSetDescription::MakeDetailWidget()
    {
        float Width = ImGui::GetContentRegionAvail().x;
        ImGui::TextWrapped(CreationTime.c_str());
        if (!IncludeClips.empty())
        {
            ImGui::Text("Included Clips:");
            ImGui::BeginChildFrame(ImGui::GetID("DCs"), ImVec2(Width, ImGui::GetTextLineHeight() * 3));
            for (const std::string& C : IncludeClips)
            {
                std::string NoPath = C.substr(Setting::Get().ProjectPath.size() + 15);
                ImGui::Text(NoPath.c_str());
            }
            ImGui::EndChildFrame();
        }
        if (!IncludeImageFolders.empty())
        {
            ImGui::Text("Included Image folders:");
            ImGui::BeginChildFrame(ImGui::GetID("DIFs"), ImVec2(Width, ImGui::GetTextLineHeight() * 3));
            ImGui::Text("PASS");
            ImGui::EndChildFrame();
        }
        // Utils::AddTitle("%s", SelectedTrainingSet.c_str());
        // ImGui::BulletText("Creation time:");
        // ImGui::Indent();
        // ImGui::TextWrapped(TrainingSetDescription.CreationTime.c_str());
        // ImGui::Unindent();
        //
        //
        // TrainingSetDescription.CreationTime.c_str());
    }

    FModelDescription::FModelDescription(YAML::Node InputNode)
    {
        Deserialize(InputNode);
    }

    YAML::Node FModelDescription::Serialize() const
    {
        YAML::Node OutNode;
        OutNode["Name"] = Name;
        OutNode["CreationTime"] = CreationTime;
        OutNode["ModelType"] = ModelType;
        OutNode["SourceTrainingSet"] = (uint64_t)SourceTrainingSetID;
        OutNode["TraningTime"] = TrainingTime;
        OutNode["BatchSize"] = BatchSize;
        OutNode["NumEpochs"] = NumEpochs;
        OutNode["Progresses"] = Progresses;
        OutNode["Recall"] = Recall;
        OutNode["Precision"] = Precision;
        OutNode["mAP5"] = mAP5;
        OutNode["mAP5_95"] = mAP5_95;
        return OutNode;
    }

    void FModelDescription::Deserialize(YAML::Node InputNode)
    {
        Name = InputNode["Name"].as<std::string>();
        CreationTime = InputNode["CreationTime"].as<std::string>();
        ModelType = InputNode["ModelType"].as<std::string>();
        SourceTrainingSetID = UUID(InputNode["SourceTrainingSet"].as<uint64_t>());
        TrainingTime = InputNode["TraningTime"].as<int>();
        BatchSize = InputNode["BatchSize"].as<int>();
        NumEpochs = InputNode["NumEpochs"].as<int>();
        Progresses = InputNode["Progresses"].as<std::vector<std::string>>();
        Recall = InputNode["Recall"].as<float>();
        Precision = InputNode["Precision"].as<float>();
        mAP5 = InputNode["mAP5"].as<float>();
        mAP5_95 = InputNode["mAP5_95"].as<float>();
    }

    void FModelDescription::MakeDetailWidget()
    {
    }

    FPredictionDescription::FPredictionDescription(YAML::Node InputNode)
    {
        Deserialize(InputNode);
    }

    YAML::Node FPredictionDescription::Serialize() const
    {
        YAML::Node OutNode;
        OutNode["Name"] = Name;
        OutNode["CreationTime"] = CreationTime;
        OutNode["ModelID"] = (uint64_t)ModelID;
        OutNode["TargetClip"] = TargetClip;
        OutNode["Confidence"] = Confidence;
        OutNode["IOU"] = IOU;
        return OutNode;
    }

    void FPredictionDescription::Deserialize(YAML::Node InputNode)
    {
        Name = InputNode["Name"].as<std::string>();
        CreationTime = InputNode["CreationTime"].as<std::string>();
        ModelID = UUID(InputNode["ModelID"].as<uint64_t>());
        TargetClip = InputNode["TargetClip"].as<std::string>();
        Confidence = InputNode["Confidence"].as<float>();
        IOU = InputNode["IOU"].as<float>();
    }

    void FPredictionDescription::MakeDetailWidget()
    {
    }
}
