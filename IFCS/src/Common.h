#pragma once
#include <array>
#include <string>
#include <unordered_map>
#include <yaml-cpp/binary.h>

#include "imgui.h"
#include "UUID.h"

/*
 * defines common enum structs here...
 */

namespace YAML
{
    class Node;
}


namespace IFCS
{
    // declare some global var here? or???
    static const ImVec2 WorkArea = {1280, 720};


    enum class EWorkspace : uint8_t
    {
        Data = 0,
        Train = 1,
        Detect = 2,
    };


    // the value is the column position in csv...
    enum class ESupportedLanguage : uint8_t
    {
        English = 0,
        TraditionalChinese = 1,
    };

    enum class ETheme : uint8_t
    {
        Light = 0,
        Dark = 1,
    };

    enum class ELogLevel : uint8_t
    {
        Info = 0,
        Warning = 1,
        Error = 2,
    };

    struct FLogData
    {
        ELogLevel LogLevel = ELogLevel::Info;
        std::string TimeString;
        std::string Message;
        bool ShouldDisplay;
    };

    struct FClipInfo
    {
        std::string ClipPath;
        int FrameCount;
        int Width;
        int Height;
        float FPS;
        std::string GetClipFileName() const;
    };

    //TODO: when to serialize them to the yaml file?

    struct FCategory
    {
        FCategory()
        {
        }

        FCategory(std::string NewDisplayName); // construct when user click add new
        FCategory(YAML::Node InputNode); // construct when load from yaml

        std::string DisplayName;
        ImVec4 Color;
        UUID ParentID = 0;
        bool Visibility = true;

        // info
        int UsedCountInThisFrame = 0;
        int TotalUsedCount = 0;

        inline bool HasParent() const { return ParentID != 0; }
        YAML::Node Serialize() const; // DON't serialize ID... they will be the key in data...
        void Deserialize(YAML::Node InputNode);
    };

    enum class EAnnotationEditMode : uint8_t
    {
        Add = 0,
        Edit = 1
    };

    enum class EBoxCorner : uint8_t
    {
        TopLeft = 0,
        BottomLeft = 1,
        BottomRight = 2,
        TopRight = 3
    };

    struct FAnnotationShiftData
    {
        int RotationDegree = 0;
        bool IsFlipX = false;
        bool IsFlipY = false;

        FAnnotationShiftData() = default;

        FAnnotationShiftData(const int& InRD, const bool& InFx, const bool& InFy)
            : RotationDegree(InRD), IsFlipX(InFx), IsFlipY(InFy)
        {
        }
    };

    struct FAnnotation
    {
        FAnnotation() = default;
        FAnnotation(UUID InCategory, std::array<float, 4> InXYWH); // contruct when new annotation is created
        FAnnotation(YAML::Node InputNode); // construct from yaml
        UUID CategoryID;
        std::array<float, 4> XYWH = {0.f, 0.f, 0.f, 0.f}; // the four floats for center x , center y, width , height
        YAML::Node Serialize() const;
        void Deserialize(YAML::Node InputNode);
        void Pan(std::array<float, 2> Changed);
        void Resize(EBoxCorner WhichCorner, std::array<float, 2> Changed);
        std::string GetExportTxt(std::unordered_map<UUID, int>& CategoryChecker,
                                 const FAnnotationShiftData& InShiftData) const;
    };

    struct FTrainingSetDescription
    {
        FTrainingSetDescription() = default;
        FTrainingSetDescription(YAML::Node InputNode);
        std::string Name;
        std::string CreationTime;
        std::vector<std::string> IncludeClips;
        std::vector<std::string> IncludeImageFolders;
        std::array<int, 2> Size = {640, 640};
        std::array<float, 3> Split = {0.80f, 0.10f, 0.10f};
        int NumDuplicates = 0;
        int TotalImagesExported = 1000; // Not include duplicated ones...
        std::string AppliedAugmentationDescription;
        YAML::Node Serialize();
        void Deserialize(YAML::Node InputNode);
        void MakeDetailWidget();
    };

    struct FModelDescription
    {
        FModelDescription() = default;
        FModelDescription(YAML::Node InputNode);
        std::string Name;
        std::string CreationTime;
        std::string ModelType;
        std::string HyperParameter;
        std::string SourceTrainingSet; // name
        float TrainingTime = 0; // hours
        std::array<int, 2> ImageSize = {748, 432};
        int BatchSize = 0;
        int NumEpochs = 0;
        float Precision = 0.f; // take best?
        float Recall = 0.f;
        float mAP5 = 0.f;
        float mAP5_95 = 0.f;
        YAML::Node Serialize() const;
        void Deserialize(YAML::Node InputNode);
        void MakeDetailWidget();
    };

    struct FDetectionDescription
    {
        FDetectionDescription() = default;
        FDetectionDescription(YAML::Node InputNode);
        std::string Name;
        std::string CreationTime;
        std::string SourceModel; // name
        std::string TargetClip;
        float Confidence = 0.25f;
        float IOU = 0.45f;
        YAML::Node Serialize() const;
        void Deserialize(YAML::Node InputNode);
        void MakeDetailWidget();
    };
}
