 
workspace "IFCS"
    configurations { "Debug", "Release"}
    -- system {"Windows", "Unix"}
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

    links {
        "GLFW",
        "GLAD",
        "ImGui",
        "spdlog",
    }


    filter "system:windows"
        defines { 
            "_WINDOWS",
            "_CRT_SECURE_NO_WARNINGS",
            "YAML_CPP_STATIC_DEFINE"
        }            
        buildoptions {"/utf-8"}
        includedirs {
            "Dependencies/opencv/build/include",        
        }
        libdirs {
            "Dependencies/opencv/build/x64/vc15/lib",
            "Dependencies/window_build",
        }
        postbuildcommands {
            "xcopy Resources %{wks.location}bin\\%{cfg.buildcfg}\\Resources /s /i /y",
            "xcopy Scripts %{wks.location}bin\\%{cfg.buildcfg}\\Scripts /s /i /y",
            "xcopy Config_Template %{wks.location}bin\\%{cfg.buildcfg}\\Config /s /i /y",
            "xcopy Config_Template Config /s /i /y",
        }
        filter "configurations:Debug"
            links {
                "opencv_world460d",
                "yaml-cppd",
            }
        filter "configurations:Release"
            links {
                "opencv_world460",
                "yaml-cpp",            
            }


include "libs/glfw.lua"
include "libs/glad.lua"
include "libs/imgui.lua"