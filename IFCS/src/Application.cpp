#include "Application.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Implot/implot.h"

#include <cstdio>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include <iostream>

#include "Setting.h"
#include "Annotation.h"
#include "CategoryManagement.h"
#include "DataBrowser.h"
#include "FrameExtractor.h"
#include "Panel.h"
#include "Log.h"
#include "MainMenu.h"
#include "ModelGenerator.h"
#include "Prediction.h"
#include "TrainingSetGenerator.h"
#include "Utils.h"


#include "ImFileDialog/ImFileDialog.h"
#include "ImguiNotify/imgui_notify.h"
#include "ImguiNotify/fa_solid_900.h"
#include "Spectrum/imgui_spectrum.h"


namespace IFCS
{
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

        // TODO: what this is for? is it important?
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);


        // TODO: set window size by config file 
        Window = glfwCreateWindow(1920, 1030, "IFCS", NULL, NULL);
        if (Window == NULL) return;
        glfwSetWindowPos(Window, 0, 50);
    	glfwSetWindowAttrib(Window, GLFW_RESIZABLE, 0);
        glfwMakeContextCurrent(Window);
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
        ImGui_ImplGlfw_InitForOpenGL(Window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        // setup theme
        Spectrum::StyleColorsSpectrum();

        // load font
        // first loaded will become default font!
        Setting::Get().DefaultFont = io.Fonts->AddFontFromFileTTF("Resources/Font/cjkFonts_allseto_v1.11.ttf", 18.0f, NULL,
                                     io.Fonts->GetGlyphRangesChineseFull());
        ImFontConfig font_cfg;
        font_cfg.FontDataOwnedByAtlas = false;
        font_cfg.MergeMode = true;
        font_cfg.GlyphMaxAdvanceX = 16.0f;
        static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        io.Fonts->AddFontFromMemoryTTF((void*)fa_solid_900, sizeof(fa_solid_900), 20.f, &font_cfg, icon_ranges);
    	io.Fonts->Build();
		// instead of dummy way to load same file twice... use deep copy and set scale 1.5?
    	Setting::Get().TitleFont = new ImFont;
		*Setting::Get().TitleFont = *Setting::Get().DefaultFont; // is this so called deep copy?
    	Setting::Get().TitleFont->Scale = 1.5;

        // load static images?
    	
    }

    void Application::run()
    {
    	// setup third party stuff
    	CreateFileDialog();

    	// setup this app
    	MainMenu::Get().SetApp(this);
        BGPanel::Get().Setup();
        LogPanel::Get().Setup("Log", false, 0);
    	DataBrowser::Get().Setup("Data Browser", true, 0);
    	FrameExtractor::Get().Setup("Frame Extractor", true, 0);
    	Annotation::Get().Setup("Annotation", true, 0);
    	TrainingSetGenerator::Get().Setup("Training Set Generator", true, 0);
    	ModelGenerator::Get().Setup("Model Generator", true, 0);
    	Prediction::Get().Setup("Prediction", true, 0);
        Setting::Get().LoadEditorIni();
    	CategoryManagement::Get().Setup("Category Management", true, 0); // need project path?
    	if (Setting::Get().ProjectIsLoaded)
			glfwSetWindowTitle(Window, (std::string("IFCS    ") + "(" + Setting::Get().ProjectPath + ")").c_str());


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

            // render all the contents...
            BGPanel::Get().Render();
            MainMenu::Get().Render();
            LogPanel::Get().Render();

            // major panels
            DataBrowser::Get().Render();
        	FrameExtractor::Get().Render();
        	Annotation::Get().Render();
        	CategoryManagement::Get().Render();

        	TrainingSetGenerator::Get().Render();
        	ModelGenerator::Get().Render();

        	Prediction::Get().Render();

            // task modals
        	if (!Setting::Get().ProjectIsLoaded && !WelcomeModal::Get().IsChoosingFolder)
        	{
        		ImGui::OpenPopup("Welcome");
        	}
        	if (Setting::Get().IsModalOpen)
        	{
        		ImGui::OpenPopup("Setting");
        	}
			WelcomeModal::Get().Render();
        	Setting::Get().RenderModal();

            // Dev panels
            // test->Render();
            // ImGui::ShowDemoWindow();
        	// UtilPanel::Get().Render();
        	// ImPlot::ShowDemoWindow();

        	// third party close/end
        	HandleDialogClose();

        	if (tick == 1)
        	{
        		Setting::Get().Tick1();
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

			if (RequestToQuit)
			{
				glfwDestroyWindow(Window);
			}
        	
        	tick ++;
        }
    }

    void Application::CreateFileDialog()
    {
		ifd::FileDialog::Instance().CreateTexture = [](uint8_t* data, int w, int h, char fmt) -> void* {
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
		ifd::FileDialog::Instance().DeleteTexture = [](void* tex) {
			GLuint texID = (GLuint)((uintptr_t)tex);
			glDeleteTextures(1, &texID);
		};
    }

    void Application::HandleDialogClose()
    {
		if (ifd::FileDialog::Instance().IsDone("ChooseProjectLocationDialog")) {
			if (ifd::FileDialog::Instance().HasResult()) {
				std::string RawString = ifd::FileDialog::Instance().GetResult().string();
				std::replace(RawString.begin(), RawString.end(), '\\', '/');
				strcpy(WelcomeModal::Get().TempProjectLocation, RawString.c_str());
				char buff[128];
				snprintf(buff, sizeof(buff), "Choose project location: %s.", RawString.c_str());
				LogPanel::Get().AddLog(ELogLevel::Info, buff);
			}
			ifd::FileDialog::Instance().Close();
			WelcomeModal::Get().IsChoosingFolder = false;
		}
		if (ifd::FileDialog::Instance().IsDone("ChooseCondaFolder")) {
			if (ifd::FileDialog::Instance().HasResult()) {
				std::string RawString = ifd::FileDialog::Instance().GetResult().string();
				std::replace(RawString.begin(), RawString.end(), '\\', '/');
				strcpy(Setting::Get().TempCondaPath, RawString.c_str());
				Setting::Get().CondaPath = RawString;
			}
			ifd::FileDialog::Instance().Close();
		}
		if (ifd::FileDialog::Instance().IsDone("ChooseYoloV7Folder")) {
			if (ifd::FileDialog::Instance().HasResult()) {
				std::string RawString = ifd::FileDialog::Instance().GetResult().string();
				std::replace(RawString.begin(), RawString.end(), '\\', '/');
				strcpy(Setting::Get().TempYoloV7Path, RawString.c_str());
				Setting::Get().YoloV7Path = RawString;
			}
			ifd::FileDialog::Instance().Close();
		}
    }
}
