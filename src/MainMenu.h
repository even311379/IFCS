#pragma once
#include "Utils.h"


namespace IFCS
{
    class MainMenu
    {
    public:
        MAKE_SINGLETON(MainMenu)
        ~MainMenu();
        void Render();

        void SetApp(class Application* InApp);
    private:
        class Application* App;
        
    };
}
