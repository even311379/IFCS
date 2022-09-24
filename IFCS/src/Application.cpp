#include "Application.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Implot/implot.h"

#include <cstdio>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include <iostream>

#include "Panel.h"
#include "Log.h"
#include "MainMenu.h"
#include "Utils.h"
#include "ImguiNotify/imgui_notify.h"
#include "ImguiNotify/tahoma.h"
#include "ImguiNotify/fa_solid_900.h"
#include "Spectrum/imgui_spectrum.h"

namespace IFCS
{
    Application::Application()
        : window(nullptr)
    {
    }

    Application::~Application()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    static void glfw_error_callback(int error, const char* description)
    {
        fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    }

    void Application::init()
    {
        
        /*YAML::Emitter out;
        out << "Hello, World!";
        std::cout << "Here's the output YAML:\n" << out.c_str(); // prints "Hello, World!"
        YAML::Node primes = YAML::Load("[2, 3, 5, 7, 11]");
        for (std::size_t i = 0; i < primes.size(); i++)
        {
            std::cout << primes[i].as<int>() << "\n";
        }
        // or:
        for (YAML::const_iterator it = primes.begin(); it != primes.end(); ++it)
        {
            std::cout << it->as<int>() << "\n";
        }

        // YAML::Node test_node = YAML::Load("{name: David, mood: Sad}");
        YAML::Node test_node = YAML::LoadFile("Resources/test.yaml");

        spdlog::warn("load yaml file has passed");
        if (test_node["name"])
        {
            spdlog::warn(test_node["name"].as<std::string>());
            spdlog::error(test_node["mood"].as<std::string>());
        }
        spdlog::warn("Good?");*/


        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) return;

        // TODO: what this is for? is it important?
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);


        // TODO: set window size by config file 
        window = glfwCreateWindow(1920, 1030, "IFCS", NULL, NULL);
        if (window == NULL) return;
        glfwSetWindowPos(window, 0, 50);
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // enable vsync

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
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        // setup theme
        Spectrum::StyleColorsSpectrum();

        // load font
        // first loaded will become default font!
        io.Fonts->AddFontFromFileTTF("Resources/Font/cjkFonts_allseto_v1.11.ttf", 18.0f, NULL,
                                     io.Fonts->GetGlyphRangesChineseFull());
        ImFontConfig font_cfg;
        font_cfg.FontDataOwnedByAtlas = false;
        font_cfg.MergeMode = true;
        font_cfg.GlyphMaxAdvanceX = 16.0f;
        static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        io.Fonts->AddFontFromMemoryTTF((void*)fa_solid_900, sizeof(fa_solid_900), 20.f, &font_cfg, icon_ranges);


        // Setting::Get().RegisterFont("default", default_font);
        // Setting::Get().RegisterFont("FA", fa_font);
        // io.Fonts->AddFontFromMemoryTTF((void*)tahoma, sizeof(tahoma), 17.f, &font_cfg);
        // ImGui::MergeIconsWithLatestFont(16.f, false);

        // load static images?
    }

    void Application::run()
    {
        MainMenu* main_menu = new MainMenu();
        BGPanel* bg = new BGPanel();
        bg->Setup();
        TestPanel* test = new TestPanel();
        test->Setup("abstraction", true, 0);
        LogPanel* MyLog = new LogPanel();
        MyLog->Setup("Log", false, 0);

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::ShowDemoWindow();

            // render all the contents...
            bg->Render();
            main_menu->Render();
            
            test->Render();
            // ShowLogPanel(&open_log);

            MyLog->Render();


            // end of render content
            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                         clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }
    }
}
