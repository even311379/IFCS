#pragma once
// #include "Utils.h"
#include "imgui.h"

/*
 * Meyers Singleton
 * Ref from: https://shengyu7697.github.io/cpp-singleton-pattern/
 */
#define MAKE_SINGLETON(class_name) \
    class_name()=default; \
    static class_name& Get() \
    { \
        static class_name singleton; \
        return singleton; \
    }\
    class_name(class_name const&) = delete;\
    void operator=(class_name const&) = delete;


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

// TODO: weird.. this += is wrong? everywhere?
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

inline ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs)
{
    return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y);
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

    // class UtilPanel : public Panel
    // {
    // public:
    //     MAKE_SINGLETON(UtilPanel)
    // protected:
    //     void RenderContent() override;
    // private:
    //     int SelectedWksToSave;
    // };
    

}
