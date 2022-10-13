#include "Common.h"
#include <yaml-cpp/yaml.h>

#include "Setting.h"
#include "Utils.h"

namespace IFCS
{
    std::string FClipInfo::GetClipFileName() const
    {
        std::vector<std::string> temp = Utils::Split(ClipPath, '\\');
        return temp.back();
    }

    FCategory::FCategory(std::string NewDisplayName)
    {
        DisplayName = NewDisplayName;
        Color = Utils::RandomPickColor(Setting::Get().Theme == ETheme::Light);
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
}
