#pragma once
#include <vector>

#include "Panel.h"
#include "Common.h"

namespace IFCS
{
    class CategoryManagement : public Panel
    {
    public:
        MAKE_SINGLETON(CategoryManagement)
        void Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose = true) override;
        void UpdateCategoryStatics();
        ImVec4 GetColorFrameDisplayName(const std::string& InName);
        std::unordered_map<UUID, FCategory> Data;
        FCategory* GetSelectedCategory();
        UUID SelectedCatID = 0;
        void AddCount();
        std::vector<UUID> GetRegisteredCIDs() const;
    protected:
        void RenderContent() override;

    private:
        char NewCatName[64];
        void Save();
        void LoadCategoriesFromFile();
        bool ShowBar;
        bool OpenChangeName = false;
        void DrawSummaryBar();

    };
}
