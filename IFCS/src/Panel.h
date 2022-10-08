#pragma once
#include "Utils.h"
#include "imgui.h"


// override the basic imgui types 
inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)
{
    return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

inline ImVec2 operator+(const ImVec2& lhs, const float& rhs)
{
    return ImVec2(lhs.x + rhs, lhs.y + rhs);
}

inline ImVec2 operator-(const ImVec2& lhs, const float& rhs)
{
    return ImVec2(lhs.x - rhs, lhs.y - rhs);
}

inline ImVec2 operator+=(const ImVec2& lhs, const ImVec2& rhs)
{
    return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs)
{
    return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

inline ImVec2 operator-=(const ImVec2& lhs, const ImVec2& rhs)
{
    return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

inline ImVec2 operator*(const int& lhs, const ImVec2& rhs)
{
    return ImVec2(lhs * rhs.x, lhs * rhs.y);
}

inline ImVec2 operator*(const float& lhs, const ImVec2& rhs)
{
    return ImVec2(lhs * rhs.x, lhs * rhs.y);
}

inline ImVec2 operator/(const ImVec2& lhs, const float& rhs)
{
    return ImVec2(lhs.x / rhs, lhs.y / rhs);
}

inline ImVec2 operator*(const ImVec2& lhs, const float& rhs)
{
    return ImVec2(lhs.x * rhs, lhs.y * rhs);
}

/*
 * the base class to define a imgui panel
 */
namespace IFCS
{

    
    class Panel
    {
    public:
        Panel();
        virtual ~Panel();
        virtual void Setup(const char* InName, bool InShouldOpen, ImGuiWindowFlags InFlags, bool InCanClose = true);
        void Render();
        void SetVisibility(bool NewVisibility);
        bool GetVisibility() { return ShouldOpen; }
        void ToggleVisibility() {ShouldOpen = ! ShouldOpen;}
    protected:
        virtual void PreRender(); // setup more panel config
        virtual void RenderContent();
        virtual void PostRender(); // setup more panel config
        bool bAlwaysRender = false;
    private:
        const char* Name;
        bool ShouldOpen;
        bool CanClose;
        ImGuiWindowFlags Flags;
        bool IsSetup;
        
    };

    class BGPanel : public Panel
    {
    public:
        MAKE_SINGLETON(BGPanel)
        void Setup();
    protected:
        void PreRender() override;
        void RenderContent() override;
        void PostRender() override;
    };

    // class LogPanel : public Panel
    // {
    //     
    // }
    

    class TestPanel : public Panel
    {
    protected:
        void RenderContent() override;
        
    public:    
        char MM[128];
    };
    
   /*
    * TODO: returned dir path encode error?
    * TODO: ... this shoul be Modal!!!! not a normal panel...
    */ 
    class WelcomeModal
    {
    public:
        MAKE_SINGLETON(WelcomeModal)
        void Render();
        bool IsChoosingFolder = false;
        char TempProjectLocation[256];
    private:
        // ImVec2 pos;
        // ImVec2 size;
        char TempProjectName[128];
        bool CheckValidInputs();
    };

    
}
