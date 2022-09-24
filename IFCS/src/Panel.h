#pragma once
#include <string>

#include "imgui.h"
/*
 * the base class to define a imgui panel
 */

namespace IFCS
{
    class Panel
    {
    public:
        Panel();
        ~Panel();
        void Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags);
        void Render();
        void SetVisibility(bool NewVisibility);
    protected:
        virtual void PreRender(); // setup more panel config
        virtual void RenderContent();
    private:
        const char* Name;
        bool ShouldOpen;
        ImGuiWindowFlags Flags;
        bool IsSetup;
        
    };

    class BGPanel : public Panel
    {
    public:
        void Setup();
    protected:
        void PreRender() override;
        void RenderContent() override;
    };

    // class LogPanel : public Panel
    // {
    //     
    // }
    

    class TestPanel : public Panel
    {
    protected:
        void RenderContent() override;
    private:
        std::string s = "星期一";
        const char* cs = "星期二";

        
    };


    
}
