#include "Application.h"
#include "Utils.h"

#include "imgui_internal.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_glfw.h"
#include "Implot/implot.h"

#include <cstdio>
#include <spdlog/spdlog.h>
#include "yaml-cpp/yaml.h"

#include "Setting.h"
#include "Annotation.h"
#include "CategoryManagement.h"
#include "DataBrowser.h"
#include "Deploy.h"
#include "Panel.h"
#include "Log.h"
#include "MainMenu.h"
#include "Train.h"
#include "Detection.h"
#include "Modals.h"


#include "ImFileDialog/ImFileDialog.h"
#include "fa_solid_900.h"
#include "stb_image.h"
#include "IconFontCppHeaders/IconsFontAwesome5.h"


/*
 * TODO: fix exported format to real yaml... rather than something wierd??
 * and create a helper script to fix previous failures
 */


namespace IFCS
{
    int UnSaveFileTick = 99999999;

    
    Application::Application()
        : Window(nullptr)
    {
    }

    Application::~Application()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        glfwDestroyWindow(Window);
        glfwTerminate();
    }

    static void glfw_error_callback(int error, const char* description)
    {
        fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    }

    void Application::init()
    {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) return;

        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

        // TODO: set window size by config file 
        Window = glfwCreateWindow(1920, 1080, "IFCS", NULL, NULL);
        if (Window == NULL) return;
        InitWindowTransform();
        glfwMakeContextCurrent(Window);
        glfwSwapInterval(1); // enable vsync

        // create window toolbar icon (top - left)
        int IconWidth, IconHeight, IconChannels;
        unsigned char* pixels = stbi_load("Resources/IFCS.png", &IconWidth, &IconHeight, &IconChannels, 4);
        GLFWimage Icon[1];
        Icon[0].width = IconWidth;
        Icon[0].height = IconHeight;
        Icon[0].pixels = pixels;
        glfwSetWindowIcon(Window, 1, Icon);

        // create imgui/ implot
        ImGui::CreateContext();
        ImPlot::CreateContext();
        // setup io flags
        ImGuiIO& io = ImGui::GetIO(); //(void)io; // why cast to void??
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = "Config/UserWks.ini";

        // tweak platform window
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 6.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // setup platform renderer content should call lastly? after create content?
        ImGui_ImplGlfw_InitForOpenGL(Window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        // load font
        // first loaded will become default font!
        Style::ApplyTheme();
        Setting::Get().DefaultFont = io.Fonts->AddFontFromFileTTF("Resources/Font/cjkFonts_allseto_v1.11.ttf", 18.0f,
                                                                  NULL,
                                                                  io.Fonts->GetGlyphRangesChineseFull());
        
        ImFontConfig font_cfg;
        font_cfg.FontDataOwnedByAtlas = false;
        font_cfg.MergeMode = true;
        font_cfg.GlyphMaxAdvanceX = 16.0f;
        static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        io.Fonts->AddFontFromMemoryTTF((void*)fa_solid_900, sizeof(fa_solid_900), 18.f, &font_cfg, icon_ranges);
        io.Fonts->Build();
        // instead of dummy way to load same file twice... use deep copy and set scale 1.5?
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

        // load static images?
    }

    void Application::run()
    {
        // setup third party stuff
        CreateFileDialog();

        // setup this app
        MainMenu::Get().SetApp(this);
        BGPanel::Get().Setup();
        Setting::Get().LoadEditorIni();
        Setting::Get().SetupApp(this);
        LogPanel::Get().Setup("Log", false, 0);
        DataBrowser::Get().Setup("Data Browser", true, 0);
        Annotation::Get().Setup("Annotation", true, 0);
        Annotation::Get().SetApp(this);
        Train::Get().Setup("Model Generator", true, 0);
        Detection::Get().Setup("Detection", true, 0);
        Deploy::Get().Setup("Deploy", true, 0);
        if (Setting::Get().ProjectIsLoaded)
        {
            // TODO: chinese project path?
            // glfwSetWindowTitle(Window, "這是中文");
            glfwSetWindowTitle(Window, (std::string("IFCS (v1.04)   ") + "(" + Setting::Get().ProjectPath + ")").c_str());
            CategoryManagement::Get().Setup("Category Management", Setting::Get().ActiveWorkspace == EWorkspace::Data, 0); // need project path?
        }
        else
        {
            Modals::Get().IsModalOpen_Welcome = true;
        }

        // DEV
        // TestPanel* test = new TestPanel();
        // test->Setup("abstraction", true, 0);
        // UtilPanel::Get().Setup("Util", true, 0);

        int tick = 0;
        while (!glfwWindowShouldClose(Window))
        {
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (Setting::Get().JustSetup)
            {
                glfwSetWindowTitle(Window, (std::string("IFCS (v1.04)    ") + "(" + Setting::Get().ProjectPath + ")").c_str());
                CategoryManagement::Get().Setup("Category Management", true, 0); // need project path?
                Setting::Get().JustSetup = false;
            }

            // render all the contents...
            BGPanel::Get().Render();
            MainMenu::Get().Render();
            LogPanel::Get().Render();

            // major panels
            if (Setting::Get().ProjectIsLoaded)
            {
                DataBrowser::Get().Render();
                Annotation::Get().Render();
                CategoryManagement::Get().Render();
                Train::Get().Render();
                Detection::Get().Render();
                Deploy::Get().Render();
            }

            // task modals
            Modals::Get().Render();

            // Dev panels
            // test->Render();
            // ImGui::ShowDemoWindow();
            // UtilPanel::Get().Render();
            // ImPlot::ShowDemoWindow();

            if (tick == 1)
            {
                Setting::Get().Tick1();
            }

            if (Setting::Get().bEnableAutoSave && tick  > UnSaveFileTick + Setting::Get().AutoSaveInterval*60  && Annotation::Get().NeedSaveFile)
            {
                // spdlog::info("tick: {0}, tick + interval *60: {1}, UnSaveFileTick: {2}", tick, tick + Setting::Get().AutoSaveInterval * 60, UnSaveFileTick);
                Annotation::Get().SaveDataFile();
                UnSaveFileTick = 99999999;
            }

            // end of render content
            // Rendering
            ImGui::Render();
            int DisplayWidth, DisplayHeight;
            glfwGetFramebufferSize(Window, &DisplayWidth, &DisplayHeight);
            glViewport(0, 0, DisplayWidth, DisplayHeight);
            glClearColor(ClearColor.x * ClearColor.w, ClearColor.y * ClearColor.w, ClearColor.z * ClearColor.w,
                         ClearColor.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(Window);
            glfwGetWindowPos(Window, &WindowPosX, &WindowPosY);
            glfwGetWindowSize(Window, &WindowWidth, &WindowHeight);

            if (RequestToChangeTitle)
            {
                // TODO: chinese project path?
                if (Annotation::Get().NeedSaveFile)
                    UnSaveFileTick = tick;
                std::string NeedSaveMark = Annotation::Get().NeedSaveFile? "*" : "";
                glfwSetWindowTitle(Window, (std::string("IFCS (v1.04)   ") + "(" + Setting::Get().ProjectPath + ") " + NeedSaveMark).c_str());
                RequestToChangeTitle = false;
            }

            if (RequestToQuit)
            {
                glfwDestroyWindow(Window);
            }

            tick ++;
        }
        Setting::Get().Save();
        if (Annotation::Get().NeedSaveFile)
            Annotation::Get().SaveDataFile();
        DataBrowser::Get().ForceCloseLoading = true;
    }

    void Application::InitWindowTransform()
    {
        YAML::Node EditorIni = YAML::LoadFile("Config/Editor.ini");
        if (EditorIni["Window"])
        {
            std::vector<int> WT = EditorIni["Window"].as<std::vector<int>>();
            glfwSetWindowPos(Window, WT[0], WT[1]);
            glfwSetWindowSize(Window, WT[2], WT[3]);
            //TODO: make it recognize monitor status ex: fullscreen?
            // way too tedious...
            // int MonitorAmount;
            // GLFWmonitor** Mo = glfwGetMonitors(&MonitorAmount);
            // &Mo->
            // glfwSetWindowMonitor(Window, )
        }
    }

    void Application::CreateFileDialog()
    {
        ifd::FileDialog::Instance().CreateTexture = [](uint8_t* data, int w, int h, char fmt) -> void*
        {
            GLuint tex;

            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            // glGenerateMipmap(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);

            return (void*)(intptr_t)tex;
        };
        ifd::FileDialog::Instance().DeleteTexture = [](void* tex)
        {
            GLuint texID = (GLuint)((uintptr_t)tex);
            glDeleteTextures(1, &texID);
        };
    }


}
