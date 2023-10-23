 
workspace "IFCS"
    configurations { "Debug", "Release"}
    -- system {"Windows", "Unix"}
    startproject "IFCS"

    flags { "MultiProcessorCompile"}

    filter "configurations:Debug"
        defines { "IFCS_DEBUG" }
        symbols "On"
if (system == windows) then
        links {
            "opencv_world460d",
            "yaml-cppd",
        }
end

    filter "configurations:Release"
        defines { "IFCS_RELEASE", "NDEBUG" }
        optimize "Speed"
        flags { "LinkTimeOptimization" }
if (system == windows) then
        links {
            "opencv_world460",
            "yaml-cpp",            
        }
end


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

    
    filter "system:linux"
        includedirs {"/usr/include/opencv4", "/usr/include"}
        libdirs {"/usr/lib"}
        links {
            "dl",
            "pthread",
            "yaml-cpp",
            "opencv_core",
            "opencv_imgproc",
            "opencv_imgcodecs",
            "opencv_video",
            "opencv_videoio",
        }
        defines { "_X11", "D_GLIBCXX_USE_CXX11_ABI" }

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


include "libs/glfw.lua"
include "libs/glad.lua"
include "libs/imgui.lua"
if (system == linux) then
    include "libs/yaml-cpp.lua"
    include "libs/spdlog.lua"
end
