#pragma once
#include <GLFW/glfw3.h>

#include "imgui.h"


namespace IFCS 
{
    class Application
    {
    public:

        Application();
        ~Application();
        
        void init();

        void run();

    private:
        GLFWwindow* window;
        ImVec4 clear_color = ImVec4(0.45f,0.55f, 0.60f, 1.00f);
        
    };
    
}

