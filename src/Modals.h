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
        bool IsModalOpen_Delete;

    private:
        void RenderWelcome();
        std::string TempProjectName;
        std::string TempProjectLocation;
        std::string TempExistingProjectLocation;
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
        void RenderDoc(const char* DocName);
        void RenderDeleteModal(EAssetType AssetType);
        void HandleFileDialogClose();
        bool NeedToRestartApp = false;
    };
    
}
