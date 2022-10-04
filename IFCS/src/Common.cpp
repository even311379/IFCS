#include "Common.h"
#include <yaml-cpp/yaml.h>

#include "Setting.h"
#include "Utils.h"

namespace IFCS
{
    FCategory::FCategory(std::string NewDisplayName)
    {
        ID = UUID();
        DisplayName = NewDisplayName;
        Color = Utils::RandomPickColor(Setting::Get().Theme==ETheme::Light);
    }

    FCategory::FCategory(UUID InID, YAML::Node InputNode)
    {
        ID = InID;
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
        ID = UUID();
    }

    FAnnotation::FAnnotation(UUID InID, YAML::Node InputNode)
    {
        ID = InID;
        Deserialize(InputNode);
    }

    YAML::Node FAnnotation::Serialize() const
    {
        YAML::Node OutNode;
        OutNode["CategoryID"] = std::to_string(CategoryID);
        OutNode["XYWH"] = XYWH;
        return OutNode;
    }

    void FAnnotation::Deserialize(YAML::Node InputNode)
    {
        CategoryID = InputNode["CategoryID"].as<uint64_t>();
        // can serialize array?
        XYWH = InputNode["XYWH"].as<std::array<float, 4>>();
    }

    void FAnnotation::Pan(std::array<float, 2> Changed)
    {
    }

    void FAnnotation::Resize(EBoxCorner WhichCorner, std::array<float, 2> Changed)
    {
    }

}
