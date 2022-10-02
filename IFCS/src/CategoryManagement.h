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
    protected:
        void RenderContent() override;

    private:
        void LoadFromCategoryFile();
        std::vector<FCategory> Categories;
        UUID SelectedCatID;
        char NewCatName[64];
        
        
    };
    
}
