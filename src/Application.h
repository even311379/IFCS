#pragma once

#include "imgui.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace IFCS 
{

    struct WindowData
    {
        const char* title;
        unsigned int width, height;
        bool vsync;
    };

    class Application
    {
    public:

        Application();
        Application(const char* title, unsigned int width, unsigned int height, bool vsync);
        ~Application();

        void Init();

        void Run();

        bool RequestToUpdateFont = false;

        bool RequestToChangeTitle = false;
        
        bool RequestToQuit = false;

        bool RequestToUpdateWorkSpace = false;

        // void InitWindowTransform();

        int WindowPosX, WindowPosY, WindowWidth, WindowHeight;


    private:

        void InitIFCS();

        void RenderIFCS();

        void PostRenderIFCS();

        void OnIFCS_Closed();

        void BuildFont();
        
        GLFWwindow* m_Window;
        WindowData m_Data;
        ImGuiID DS_ID;

        ImVec4 ClearColor = ImVec4(0.45f,0.55f, 0.60f, 1.00f);

        int tick = 0;

        int UnSaveFileTick = 9999999;                

        
    };
    
}

