#pragma once
#include "Panel.h"
#include "IFCS_Types.h"

namespace IFCS
{
    class CategoryManagement : public Panel
    {
    public:
        MAKE_SINGLETON(CategoryManagement)
        void Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose = true) override;
        void UpdateCategoryStatics();
        void LoadCategoriesFromFile();
        std::unordered_map<UUID, FCategory> Data;
        std::unordered_map<UUID, bool> GeneratorCheckData;
        FCategory* GetSelectedCategory();
        UUID SelectedCatID = 0;
        void AddCount();
        std::vector<UUID> GetRegisteredCIDs() const;
        bool NeedUpdate = false;
    protected:
        bool CheckValidNewCatName();
        void RenderContent() override;

    private:
        char NewCatName[64];
        void Save();
        bool ShowBar;
        bool OpenChangeName = false;
        void DrawSummaryBar();

    };
}
