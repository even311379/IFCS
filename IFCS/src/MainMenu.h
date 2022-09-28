#pragma once
#include "Common.h"


namespace IFCS
{
    class MainMenu
    {
    public:
        MAKE_SINGLETON(MainMenu)
        ~MainMenu();
        void Render();
        void OpenSetting();
        
    };
}
