 
workspace "IFCS"
    configurations { "Debug", "Release"}
    startproject "IFCS"

    flags { "MultiProcessorCompile"}

    filter "configurations:Debug"
        defines { "IFCS_DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "IFCS_RELEASE", "NDEBUG" }
        optimize "Speed"
        flags { "LinkTimeOptimization" }


project "IFCS"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
	architecture "x86_64"
        
    targetdir "bin/%{cfg.buildcfg}"
    objdir "obj/%{cfg.buildcfg}"

    includedirs { 
        "libs/glad/include/", 
        "libs/glfw/include/", 
        "libs/imgui/", 
        "libs/imgui/examples",
        "libs/yaml-cpp/include",
        "libs/spdlog/include",        
        "vendor/stb", 
        "vendor/imgui_extensions/", 
    }

    files {
        "src/*.cpp",
        "vendor/stb/*.cpp",
        "vendor/imgui_extensions/addons/*.cpp",
        "vendor/imgui_extensions/ImGuiFileDialog/*.cpp",
        "vendor/imgui_extensions/Implot/*.cpp",
    }

    links {"GLFW", "GLAD", "ImGui", "yaml-cpp", "spdlog"}

    
    filter "system:linux"
        includedirs {"/usr/include/opencv4", "/usr/include"}
        libdirs {"/usr/lib"}
        links {
            "dl",
            "pthread",
            "opencv_core",
            "opencv_imgproc",
            -- "opencv_highgui", 
            "opencv_imgcodecs",
            "opencv_video",
            "opencv_videoio",
        }
        defines { "_X11", "D_GLIBCXX_USE_CXX11_ABI" }

    filter "system:windows"
        defines { "_WINDOWS" }    
        postbuildcommands {
            "xcopy Resources %{wks.location}build\\%{cfg.buildcfg}\\Resources /s /i /y",
            "xcopy Scripts %{wks.location}build\\%{cfg.buildcfg}\\Scripts /s /i /y",
            "xcopy %{wks.location}%{prj.name}\\Config %{wks.location}build\\%{cfg.buildcfg}\\Config /s /i /y"
        }

include "libs/glfw.lua"
include "libs/glad.lua"
include "libs/imgui.lua"
include "libs/yaml-cpp.lua"
include "libs/spdlog.lua"

