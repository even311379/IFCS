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

        bool RequestToChangeTitle = false;
        
        bool RequestToQuit = false;

        void InitWindowTransform();

        int WindowPosX, WindowPosY, WindowWidth, WindowHeight;


    private:
        GLFWwindow* Window;
        ImVec4 ClearColor = ImVec4(0.45f,0.55f, 0.60f, 1.00f);
        void CreateFileDialog();
        
    };
    
}

