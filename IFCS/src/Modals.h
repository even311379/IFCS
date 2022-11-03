#pragma once
#include "imgui.h"
#include "Utils.h"


/*
 * Every major modals will render here?
 * how to abstract this?
 */
class Modals
{
public:
    MAKE_SINGLETON(Modals)
    void Render();
    bool IsChoosingFolder = false;

private:
    void RenderWelcome();
    void RenderSetting();
    void RenderChangeProject();
    void RenderStartNewProject();
    void HandleFileDialogClose();
};
