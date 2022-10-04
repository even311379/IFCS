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
    protected:
        void RenderContent() override;

    private:
        std::vector<FCategory> Categories;
        UUID SelectedCatID;
        char NewCatName[64];
        void Save();
        void LoadCategoriesFromFile();
    };
    
}
