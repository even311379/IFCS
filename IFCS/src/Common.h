#pragma once
#include <array>
#include <map>
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
        Deploy = 3,
    };

    enum class EAssetType : uint8_t
    {
        Clip = 0,
        Image = 1,
        TrainingSet = 2,
        Model = 3,
        Detection = 4
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
        void Update(const int& NewFrameCount, const float& NewFPS, const int& NewWidth, const int& NewHeight);
        std::string GetClipFileName() const;
        std::string GetRelativePath() const;
        void MakeDetailWidget();
    };

    struct FImageInfo 
    {
        std::string ImagePath;
        int Width;
        int Height;
        void Update(const int& NewWidth, const int& NewHeight);
        std::string GetFileName() const;
        std::string GetRelativePath() const;
        void MakeDetailWidget();
    };

    struct FCategory
    {
        FCategory()=default;
        FCategory(std::string NewDisplayName); // construct when user click add new
        FCategory(YAML::Node InputNode); // construct when load from yaml

        std::string DisplayName;
        ImVec4 Color;
        UUID ParentID = 0;
        bool Visibility = true;

        // info
        ImU16 TotalUsedCount = 0;
        inline bool HasParent() const { return ParentID != 0; }
        YAML::Node Serialize() const; // DON't serialize ID... they will be the key in data...
        void Deserialize(YAML::Node InputNode);
    };

    struct FCategoryMergeData
    {
        FCategoryMergeData()=default;
        std::string DisplayName;
        std::vector<UUID> SourceCatIdx;
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

    struct FAnnotationToDisplay
    {
        FAnnotationToDisplay()=default;
        FAnnotationToDisplay(const size_t& InCount, const bool& InReady)
            :Count(InCount), IsReady(InReady) {}
        size_t Count;
        bool IsReady;
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
        std::string GetExportTxt(size_t CID) const;
    };

    struct FAnnotationSave
    {
        FAnnotationSave() = default;
        FAnnotationSave(YAML::Node InputNode);
        std::unordered_map<UUID, FAnnotation> AnnotationData;
        std::string UpdateTime;
        bool IsReady;
        YAML::Node Serialize() const;
        void Deserialize(YAML::Node InputNode);
        bool IsEmpty() const {return int(AnnotationData.size()) == 0; } 
    };

    struct FTrainingSetDescription
    {
        FTrainingSetDescription() = default;
        FTrainingSetDescription(const std::string& InName, const YAML::Node& InputNode);
        std::string Name;
        std::vector<std::string> Categories;
        std::string CreationTime;
        std::vector<std::string> IncludeClips;
        std::vector<std::string> IncludeImageFolders;
        std::array<int, 2> Size = {640, 640};
        std::array<float, 3> Split = {0.80f, 0.10f, 0.10f};
        // int NumDuplicates = 0;
        int TotalImagesExported = 1000; // Not include duplicated ones...
        // std::string AppliedAugmentationDescription;
        YAML::Node Serialize();
        void Deserialize(const std::string& InName, const YAML::Node& InputNode);
        void MakeDetailWidget();
    };

    struct FModelDescription
    {
        FModelDescription() = default;
        FModelDescription(const std::string& InName, const YAML::Node& InputNode);
        std::string Name;
        std::vector<std::string> Categories; 
        std::string CreationTime;
        std::string ModelType;
        std::string SourceTrainingSet; // name
        float Precision = 0.f; // take best?
        float Recall = 0.f;
        float mAP5 = 0.f;
        float mAP5_95 = 0.f;
        YAML::Node Serialize() const;
        void Deserialize(const std::string& InName, const YAML::Node& InputNode);
        void MakeDetailWidget();
    };

    struct FDetectionDescription
    {
        FDetectionDescription() = default;
        FDetectionDescription(const std::string& InName, const YAML::Node& InputNode);
        std::string Name;
        std::vector<std::string> Categories; // exported format...
        std::string CreationTime;
        std::string SourceModel; // name
        std::string TargetClip;
        float Confidence = 0.25f;
        float IOU = 0.45f;
        int ImageSize = 640;
        YAML::Node Serialize() const;
        void Deserialize(const std::string& InName, const YAML::Node& InputNode);
        void MakeDetailWidget();
    };

    // used in individual tracking
    struct FLabelData
    {
        int CatID;
        float X, Y, Width, Height, Conf;
        FLabelData() = default;
        FLabelData(const int& InCatID, const float& InX, const float& InY, const float& InWidth, const float& InHeight,
            const float& InConf)
            : CatID(InCatID), X(InX), Y(InY), Width(InWidth), Height(InHeight), Conf(InConf)
        {}
        float Distance(const FLabelData& Other, int Width, int Height) const;
        float GetApproxBodySize(int WPixels, int HPixel) const; // in pixel ^ 2

    };

    enum EIndividualColumnID
    {
        IndividualColumnID_Category,
        IndividualColumnID_IsPassed,
        IndividualColumnID_ApproxSpeed,
        IndividualColumnID_ApproxBodySize,
        IndividualColumnID_EnterFrame,
        IndividualColumnID_LeaveFrame
    };
    
    // used in individual tracking
    struct FIndividualData
    {
        FIndividualData()=default;
        FIndividualData(const int& InFrameNum, const FLabelData& InLabel)
        {
            AddInfo(InFrameNum, InLabel);
        }
        inline static int Width, Height;
        inline static std::vector<std::string> CategoryNames;
        inline static float SpeciesDetermineThreshold = 0.1f;
        
        std::map<int, FLabelData> Info;
        bool IsCompleted = false;
        bool HasPicked = false;
        int GetEnterFrame() const;
        int GetLeaveFrame() const;
        float GetApproxBodySize() const;
        float GetApproxSpeed() const;
        std::string GetName() const;
        void AddInfo(const int& InFrameNum, const FLabelData& InLabel)
        {
            Info[InFrameNum] = InLabel;
        }
        // for sort to work?
        inline static const ImGuiTableSortSpecs* CurrentSortSpecs;
        static int __cdecl CompareWithSortSpecs(const void* lhs, const void* rhs);
    };



    // deploy
    struct FFeasibleZone
    {
        FFeasibleZone() = default;
        FFeasibleZone(const YAML::Node& InputNode);
        void Deserialize(const YAML::Node& InputNode);
        YAML::Node Serialize() const;
        std::array<float, 4> XYWH = {0.f, 0.f, 0.f, 0.f};
        ImVec4 ColorLower, ColorUpper;
    };


    struct FCameraSetup
    {
        FCameraSetup()=default;
        FCameraSetup(const YAML::Node& InputNode);
        YAML::Node Serialize() const;
        void Deserialize(const YAML::Node& InputNode);
        std::string CameraName;
        // feasible data

        // model data
        std::string ModelName;
        std::string ModelFilePath;
        int ImageSize = 640;
        float Confidence = 0.25f;
        float IOU = 0.45f;
        bool ShouldApplyDetectionROI = false;
        std::array<float, 4> DetectionROI = {0.f, 0.f, 1.f, 1.f};
        bool IsPassVertical;
        std::array<float, 2> FishwayStartEnd = {0.f, 1.f};
        bool ShouldEnableSpeedThreshold = true;
        float SpeedThreshold = 0.1f;
        bool ShouldEnableBodySizeThreshold = true;
        float BodySizeThreshold = 0.2f;
        float SpeciesDetermineThreshold = 0.1f;
        int FrameBufferSize = 10;

        // category
        std::vector<std::string> Categories;

    };

    struct FSMSReceiver
    {
        FSMSReceiver() = default;
        FSMSReceiver(const YAML::Node& InputNode);
        void Deserialize(const YAML::Node& InputNode);
        YAML::Node Serialize() const;
        std::string Name = "GoodGuy";
        std::string Phone = "0912345678";
        std::string Group = "Manager";
    };

    struct FSendGroup
    {
        FSendGroup()=default;
        FSendGroup(const YAML::Node& InputNode);
        void Deserialize(const YAML::Node& InputNode);
        YAML::Node Serialize() const;
        bool GroupManager = false;
        bool GroupClient = false;
        bool GroupHelper = false;
    };

    struct FDeploymentData
    {
        FDeploymentData() = default;
        FDeploymentData(const YAML::Node& InputNode);
        YAML::Node Serialize() const;
        void Deserialize(const YAML::Node& InputNode);
        
        std::string TaskInputDir;
        std::vector<FCameraSetup> Cameras = {FCameraSetup()}; // DB name -> camera setup
        bool IsTaskStartNow = false;
        int ScheduledRuntime[2] = {3, 0}; // default time: 3:00AM to start the task...
        bool IsRunSpecifiedDates = false;
        std::vector<tm> RunDates; // run dates bwteen start and end date
        tm StartDate = tm();
        tm EndDate = tm();
        bool ShouldBackupImportantRegions = true;
        
        // Data pipeline
        int DetectionPeriodInSecond = 60;
        int PassCountDurationInMinutes = 5; // minutes
        std::string ServerAddress;
        std::string ServerAccount;
        std::string ServerPassword;
        
        // SMS
        std::string TwilioSID;
        std::string TwilioAuthToken;
        std::string TwilioPhoneNumber;
        std::string ReceiverAreaCode = "886";
        std::vector<FSMSReceiver> SMSReceivers = { FSMSReceiver() };
        FSendGroup DailyReportSendGroup;
        FSendGroup WeeklyReportSendGroup;
        FSendGroup MonthlyReportSendGroup;
        FSendGroup LoseConnectionSendGroup;
        FSendGroup SMSTestSendGroup;
        int SMS_SendTime[2] = {9, 0};
    };
}
