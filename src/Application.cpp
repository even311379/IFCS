#include "Application.h"
#include "UUID.h"
#include "Panel.h"
#include "Setting.h"
#include "gl_utils.h"
#include "fa_solid_900.h"
#include "MainMenu.h"
#include "Setting.h"
#include "DataBrowser.h"
#include "Annotation.h"
#include "Train.h"
#include "Deploy.h"
#include "Modals.h"
#include "CategoryManagement.h"
#include "Detection.h"

#include <string>
#include "yaml-cpp/yaml.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "stb_image.h"
#include "IconFontCppHeaders/IconsFontAwesome5.h"
#include "Implot/implot.h"
#include "spdlog/spdlog.h"
#include "imgui_internal.h"

namespace IFCS
{
    Application::Application()
    {

    }

    Application::Application(const char* title, unsigned int width, unsigned int height, bool vsync)
    {    
        m_Data = WindowData{
            title,
            width,
            height,
            vsync,
        };
    }

    Application::~Application()
    {        
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        glfwDestroyWindow(m_Window);
        glfwTerminate();

    }

    void Application::Init()
    {    

        if (!glfwInit())
        {
            fprintf(stderr, "Failed to init GLFW\n");
            return;
        }

        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_Window = glfwCreateWindow(m_Data.width, m_Data.height, m_Data.title, NULL, NULL);
        if (m_Window == NULL) return;
        // InitWindowTransform();

        glfwSetWindowUserPointer(m_Window, &m_Data);

        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow *window, int width, int height) {
        WindowData *data = (WindowData *)glfwGetWindowUserPointer(window);
        data->width = width;
        data->height = height;
        GL(glViewport(0, 0, width, height));

        printf("Resized %d, %d\n 123456", data->width, data->height);
    });


        glfwMakeContextCurrent(m_Window);

        /*
        This is the key!!!! 
         https://stackoverflow.com/questions/68051588/segmentation-fault-with-glviewport-opengl-c-programming-with-glfw-and-glad
        */
        gladLoadGL(); 


        glfwSwapInterval(1); // enable vsync

        // create window toolbar icon (top - left)
        int IconWidth, IconHeight, IconChannels;
        unsigned char* pixels = stbi_load("Resources/IFCS.png", &IconWidth, &IconHeight, &IconChannels, 4);
        GLFWimage Icon[1];
        Icon[0].width = IconWidth;
        Icon[0].height = IconHeight;
        Icon[0].pixels = pixels;
        glfwSetWindowIcon(m_Window, 1, Icon);

        // create imgui/ implot
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        // setup io flags
        ImGuiIO& io = ImGui::GetIO(); //(void)io; // why cast to void??
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = "Config/UserWks.ini";

        // setup platform renderer content should call lastly? after create content?
        ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
    }

    bool show_demo_window = true;
    void Application::Run()
    {
        InitIFCS();

        while (!glfwWindowShouldClose(m_Window))
        {
            glfwPollEvents();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            DS_ID = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
            // Rendering
            RenderIFCS();


    //         ImGui::ShowDemoWindow();
    //         ImPlot::ShowDemoWindow();
    //         static int WS_ID = 0;
    //         static bool update_WS = false;
    // if (ImGui::BeginMainMenuBar())
    // {
    //     if (ImGui::BeginMenu("File"))
    //     {
    //         if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
    //         ImGui::EndMenu();
    //     }
    //     if (ImGui::BeginMenu("ChangeWS"))
    //     {
    //         if (ImGui::MenuItem("WS1", "CTRL+1")) {
    //             WS_ID = 0;
    //             update_WS = true;
    //         }
    //         if (ImGui::MenuItem("WS2", "CTRL+2")) {
    //             WS_ID = 1;
    //             update_WS = true;

    //         }  // Disabled item
    //         ImGui::Separator();
    //         if (ImGui::MenuItem("Cut", "CTRL+X")) {}
    //         if (ImGui::MenuItem("Copy", "CTRL+C")) {}
    //         if (ImGui::MenuItem("Paste", "CTRL+V")) {}
    //         ImGui::EndMenu();
    //     }
    //     ImGui::EndMainMenuBar();
    // }

    // ImGuiID DS_ID = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    // {
    //     ImGui::Begin("Win1");
    //     ImGui::Text("Something");
    //     ImGui::End();
    // }
    // {
    //     ImGui::Begin("Win2");
    //     ImGui::Text("Something");
    //     ImGui::End();
    // }
    // if (update_WS)
    // {            
    //     ImGuiDockNodeFlags Flags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar;
    //     // ImGui::DockBuilderRemoveNode(DS_ID);
    //     // ImGui::DockBuilderAddNode(DS_ID, Flags | ImGuiDockNodeFlags_DockSpace);
    //     // ImGui::DockBuilderSetNodeSize(DS_ID, ImGui::GetMainViewport()->Size);
    //     // static ImGuiID Left = ImGui::GetID("DS_Left");
    //     static ImGuiID Left;
    //     static ImGuiID Right = ImGui::GetID("DS_Right");
    //     static ImGuiID Up = ImGui::GetID("DS_Up");
    //     static ImGuiID Down = ImGui::GetID("DS_Down");
    //     ImGui::DockBuilderRemoveNodeChildNodes(DS_ID);         
    //     if (WS_ID == 0)
    //     {   
    //         ImGui::DockBuilderSplitNode(DS_ID, ImGuiDir_Left, 0.40f, &Left, &Right);
    //         ImGui::DockBuilderDockWindow("Win1", Left);
    //         ImGui::DockBuilderDockWindow("Win2", Right);
    //         ImGui::DockBuilderFinish(DS_ID);
    //         spdlog::info("change to WS 2?");
    //     }
    //     if (WS_ID == 1)
    //     {
    //         ImGui::DockBuilderSplitNode(DS_ID, ImGuiDir_Up, 0.40f, &Up, &Down);
    //         ImGui::DockBuilderDockWindow("Win1", Up);
    //         ImGui::DockBuilderDockWindow("Win2", Down);
    //         ImGui::DockBuilderFinish(DS_ID);
    //         spdlog::info("change to WS 1?");

    //     }
    //     update_WS = false;
    // }



            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(m_Window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(ClearColor.x * ClearColor.w, ClearColor.y * ClearColor.w, ClearColor.z * ClearColor.w,
                         ClearColor.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(m_Window);

            PostRenderIFCS();
        }

        OnIFCS_Closed();
    }

    void Application::InitIFCS()
    {
        MainMenu::Get().SetApp(this);
        // BGPanel::Get().Setup();
        Setting::Get().LoadEditorIni();
        Setting::Get().SetupApp(this);        
        DataBrowser::Get().Setup("Data Browser", true, 0);
        Annotation::Get().Setup("Annotation", true, 0);
        Annotation::Get().SetApp(this);
        Train::Get().Setup("Model Generator", true, 0);
        Detection::Get().Setup("Detection", true, 0);
        Deploy::Get().Setup("Deploy", true, 0);
        BuildFont();
        if (Setting::Get().ProjectIsLoaded)
        {
            // TODO: chinese project path?
            glfwSetWindowTitle(m_Window, (std::string("IFCS (v1.2)   ") + "(" + Setting::Get().ProjectPath + ")").c_str());
            CategoryManagement::Get().Setup("Category Management", Setting::Get().ActiveWorkspace == EWorkspace::Data, 0); // need project path?
        }
        else
        {
            Modals::Get().IsModalOpen_Welcome = true;
        }
    }

    void Application::RenderIFCS()
    {        
        if (Setting::Get().JustSetup)
        {
            glfwSetWindowTitle(m_Window, (std::string("IFCS (v1.2)    ") + "(" + Setting::Get().ProjectPath + ")").c_str());
            CategoryManagement::Get().Setup("Category Management", true, 0); // need project path?
            Setting::Get().JustSetup = false;
        }

        // render all the contents...
        MainMenu::Get().Render();

        // major panels
        if (Setting::Get().ProjectIsLoaded)
        {
            DataBrowser::Get().Render();
            Annotation::Get().Render();
            CategoryManagement::Get().Render();
            Train::Get().Render();
            Detection::Get().Render();
            Deploy::Get().Render();
            // BGPanel::Get().Render();
        }

        // task modals
        Modals::Get().Render();


        if (tick == 1)
        {
            Setting::Get().Tick1();
            spdlog::info("tick 1 is called");
        }

        if (Setting::Get().bEnableAutoSave && tick  > UnSaveFileTick + Setting::Get().AutoSaveInterval*60  && Annotation::Get().NeedSaveFile)
        {
            // spdlog::info("tick: {0}, tick + interval *60: {1}, UnSaveFileTick: {2}", tick, tick + Setting::Get().AutoSaveInterval * 60, UnSaveFileTick);
            Annotation::Get().SaveDataFile();
            UnSaveFileTick = 99999999;
        }

        if (RequestToUpdateWorkSpace)
        {        
            ImGuiDockNodeFlags Flags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar;
            ImGui::DockBuilderRemoveNodeChildNodes(DS_ID);                    
            static ImGuiID Left = ImGui::GetID("LeftDS");
            static ImGuiID Center = ImGui::GetID("CenterDS");
            static ImGuiID Right = ImGui::GetID("RightDS");
            switch (Setting::Get().ActiveWorkspace) {
            case EWorkspace::Data:
            {            
                ImGui::DockBuilderSplitNode(DS_ID, ImGuiDir_Left, 0.15f, &Left, &Center);
                ImGui::DockBuilderSplitNode(Center, ImGuiDir_Right, 0.175f, &Right, nullptr);
                ImGui::DockBuilderDockWindow("Data Browser", Left);
                ImGui::DockBuilderDockWindow("Annotation", Center);
                ImGui::DockBuilderDockWindow("Category Management", Right);
                ImGui::DockBuilderFinish(DS_ID);
                break;
            }
            case EWorkspace::Train:
            {
                ImGui::DockBuilderSplitNode(DS_ID, ImGuiDir_Left, 0.15f, &Left, &Right);
                ImGui::DockBuilderDockWindow("Data Browser", Left);
                ImGui::DockBuilderDockWindow("Training Set Generator", Right);
                ImGui::DockBuilderDockWindow("Model Generator", Right);
                ImGui::DockBuilderFinish(DS_ID);
                break;
            }
            case EWorkspace::Detect:
            {            
                ImGuiID Left;
                ImGuiID Right;
                ImGui::DockBuilderSplitNode(DS_ID, ImGuiDir_Left, 0.15f, &Left, &Right);
                ImGui::DockBuilderDockWindow("Data Browser", Left);
                ImGui::DockBuilderDockWindow("Detection", Right);
                ImGui::DockBuilderFinish(DS_ID);
                break;
            }
            case EWorkspace::Deploy:
            {            
                ImGuiID Left;
                ImGuiID Right;
                ImGui::DockBuilderSplitNode(DS_ID, ImGuiDir_Left, 0.15f, &Left, &Right);
                ImGui::DockBuilderDockWindow("Data Browser", Left);
                ImGui::DockBuilderDockWindow("Deploy", Right);
                ImGui::DockBuilderFinish(DS_ID);
                break;
            }
            }
            RequestToUpdateWorkSpace = false;
        }
    }

    void Application::PostRenderIFCS()
    {            
        if (RequestToChangeTitle)
        {
            // TODO: chinese project path?
            if (Annotation::Get().NeedSaveFile)
                UnSaveFileTick = tick;
            std::string NeedSaveMark = Annotation::Get().NeedSaveFile? "*" : "";
            glfwSetWindowTitle(m_Window, (std::string("IFCS (v1.2)   ") + "(" + Setting::Get().ProjectPath + ") " + NeedSaveMark).c_str());
            RequestToChangeTitle = false;
        }

        if (RequestToQuit)
        {
            glfwDestroyWindow(m_Window);
        }

        tick ++;

    }

    void Application::OnIFCS_Closed()
    {
        Setting::Get().Save();
        if (Annotation::Get().NeedSaveFile)
            Annotation::Get().SaveDataFile();
        DataBrowser::Get().ForceCloseLoading = true;
    }

    void Application::BuildFont()
    {
        ImGuiIO& io = ImGui::GetIO(); //(void)io; // why cast to void??
        io.Fonts->Clear();
        std::string FontFile = Setting::Get().Font.empty()? "Resources/Font/NotoSansCJKtc-Black.otf" : Setting::Get().Font;
        Setting::Get().DefaultFont = io.Fonts->AddFontFromFileTTF(FontFile.c_str(), 18.0f,NULL,io.Fonts->GetGlyphRangesChineseFull());
        ImFontConfig font_cfg;
        font_cfg.FontDataOwnedByAtlas = false;
        font_cfg.MergeMode = true;
        font_cfg.GlyphMaxAdvanceX = 16.0f;
        static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        io.Fonts->AddFontFromMemoryTTF((void*)fa_solid_900, sizeof(fa_solid_900), 18.f, &font_cfg, icon_ranges);
        io.Fonts->Build();

        // // instead of dummy way to load same file twice... use deep copy and set scale 1.5?
        Setting::Get().TitleFont = new ImFont;
        *Setting::Get().TitleFont = *Setting::Get().DefaultFont; // is this so called deep copy?
        Setting::Get().TitleFont->Scale = 1.5;

        // markdown fonts
        Setting::Get().H2 = new ImFont;
        *Setting::Get().H2 = *Setting::Get().DefaultFont;
        Setting::Get().H2->Scale = 1.25;
        Setting::Get().H3 = new ImFont;
        *Setting::Get().H3= *Setting::Get().DefaultFont;
        Setting::Get().H3->Scale = 1.10f;
    }
}
