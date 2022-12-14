#pragma once
#include "imgui.h"
#include "Utils.h"


/*
 * Every major modals will render here?
 * how to abstract this?
 */
namespace IFCS
{
    class Modals
    {
    public:
        MAKE_SINGLETON(Modals)
        void Render();
        bool IsChoosingFolder = false;
        void Sync();
        bool IsModalOpen_Welcome;
        bool IsModalOpen_NewProject;
        bool IsModalOpen_LoadProject;
        bool IsModalOpen_ImportData;
        bool IsModalOpen_Setting;
        bool IsModalOpen_About;
        bool IsModalOpen_Tutorial;
        bool IsModalOpen_Contact;
        bool IsModalOpen_License;

    private:
        void RenderWelcome();
        char TempProjectName[128];
        char TempProjectLocation[128];
        char TempExistingProjectLocation[128];
        bool CheckValidProjectName();
        bool CheckValidExistingProject();
        void RenderNewProject();
        void RenderLoadProject();
        void RenderImportData();
        void RenderSetting();
        int ThemeToUse = 0;
        int LanguageToUse = 0;
        int AppSizeToUse = 0;
        int CustomWidth = 1280;
        int CustomHeight = 720;
        float WidgetResizeScale = 1.f;
        float GlobalFontScaling = 1.f;
        char TempPythonPath[128];
        char TempYoloV7Path[128];
        char TempPythonEnv[128];
        void RenderDoc(const char* DocName);
        void HandleFileDialogClose();
    };
    
}
