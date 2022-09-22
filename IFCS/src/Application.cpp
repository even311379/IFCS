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
        YAML::Emitter out;
        out << "Hello, World!";
        std::cout << "Here's the output YAML:\n" << out.c_str(); // prints "Hello, World!"
        YAML::Node primes = YAML::Load("[2, 3, 5, 7, 11]");
        for (std::size_t i=0;i<primes.size();i++) {
          std::cout << primes[i].as<int>() << "\n";
        }
        // or:
        for (YAML::const_iterator it=primes.begin();it!=primes.end();++it) {
          std::cout << it->as<int>() << "\n";
        }

        // YAML::Node test_node = YAML::Load("{name: David, mood: Sad}");
        YAML::Node test_node = YAML::LoadFile("../Resources/test.yaml");
        
        spdlog::warn("load yaml file has passed");
        if (test_node["name"])
        {
            spdlog::warn(test_node["name"].as<std::string>());
            spdlog::error(test_node["mood"].as<std::string>());
        }
        spdlog::warn("Good?");
        // std::cout<< config["test"].as<std::string>() << std::endl;

        
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

        // setup styles
        Spectrum::StyleColorsSpectrum(); // light

        // load static images?

        // setup platform renderer content should call lastly? after create content?
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
    }

    void Application::run()
    {
        BG* bg = new BG();
        bg->Setup();
        TestPanel* test = new TestPanel();
        test->Setup("abstraction", true, 0);
        LogPanel* MyLog = new LogPanel();
        MyLog->Setup("Log", true, 0);
        // bool open_log = true;
        static char MM[128] = "";
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            // render all the contents...
            bg->Render();
            test->Render();
            // ShowLogPanel(&open_log);
            ImGui::Begin("TEst123");
                ImGui::Text("Hello");
                ImGui::InputText("MM", MM, IM_ARRAYSIZE(MM));
                if (ImGui::Button("Add Log"))
                    MyLog->AddLog(ELogLevel::Info, MM);
                if (ImGui::Button("Add Warning Log"))
                    MyLog->AddLog(ELogLevel::Warning, MM);
                if (ImGui::Button("Add Error Log"))
                    MyLog->AddLog(ELogLevel::Error, MM);
            ImGui::End();
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
        delete bg;
        delete test;
    }
}
