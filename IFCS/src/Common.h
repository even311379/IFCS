#pragma once
#include <array>
#include <string>

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
    enum class EWorkspace : uint8_t
    {
        Data = 0,
        Train = 1,
        Predict = 2,
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

    //TODO: when to serialize them to the yaml file?
    
    struct FCategory
    {
        FCategory(){}
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
        Edit = 1,
        Reassign = 2,
        Remove = 3
    };
    
    enum class EBoxCorner : uint8_t
    {
        TopLeft = 0,
        BottomLeft = 1,
        BottomRight = 2,
        TopRight = 3
    };
    
    struct FAnnotation
    {
        FAnnotation(){}
        FAnnotation(UUID InCategory, std::array<float, 4> InXYWH); // contruct when new annotation is created
        FAnnotation(YAML::Node InputNode); // construct from yaml
        
        UUID CategoryID;
        std::array<float, 4> XYWH = {0.f, 0.f, 0.f, 0.f}; // the four floats for center x , center y, width , height
        
        YAML::Node Serialize() const;
        void Deserialize(YAML::Node InputNode);
        void Pan(std::array<float, 2> Changed);
        void Resize(EBoxCorner WhichCorner, std::array<float, 2> Changed);
    };
}
