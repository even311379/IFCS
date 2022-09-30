workspace "IFCS"
    platforms { "x64" }
    configurations { "Debug", "Release"}
    flags { "MultiProcessorCompile"}

project "IFCS"
    location "IFCS"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"

    targetdir "build/%{cfg.buildcfg}"
    objdir "build/%{cfg.buildcfg}"


    files
    { 
        "%{prj.name}/src/**.h", 
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/vendor/stb/**.h", 
        "%{prj.name}/vendor/stb/**.cpp",
        "%{prj.name}/vendor/imgui/**.h", 
        "%{prj.name}/vendor/imgui/**.cpp",
        "%{prj.name}/vendor/imgui_extensions/**.h", 
        "%{prj.name}/vendor/imgui_extensions/**.cpp",
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS"
    }

    includedirs
    {
        "%{prj.name}/vendor/imgui",
        "%{prj.name}/vendor/imgui_extensions",
        "%{prj.name}/vendor/stb",
        "%{prj.name}/vendor/spdlog/include",
        "Dependencies/glfw/include",
        "Dependencies/opencv/build/include",
        "Dependencies/yaml-cpp/include",
        "Dependencies/pybind11/include",
        "%(AdditionalIncludeDirectories)"
    }

    libdirs 
    { 
        "Dependencies/glfw/lib-vc2019",
        "Dependencies/opencv/build/x64/vc15/lib",
        "Dependencies/yaml-cpp/lib",
        "%(AdditionalLibraryDirectories)"
    }
    
    links
    {
        "glfw3.lib",
        "opengl32.lib",
        "gdi32.lib" -- for ??? to work
    }
    
    defines {"YAML_CPP_STATIC_DEFINE"}
    
--     copy all resource folder to build folder /s: include subdir, /i: auto create folder, /y override existing
    postbuildcommands {
        "xcopy %{wks.location}%{prj.name}\\Resources %{wks.location}build\\%{cfg.buildcfg}\\Resources /s /i /y",
        "xcopy %{wks.location}%{prj.name}\\Config %{wks.location}build\\%{cfg.buildcfg}\\Config /s /i /y"
    }
    filter "configurations:Debug"
        defines { "IFCS_DEBUG" }
        symbols "On"
        buildoptions {"/utf-8"}
        links { 
            "opencv_world460d.lib",
            "yaml-cppd.lib"
        }

    filter "configurations:Release"
        defines { "IFCS_RELEASE", "NDEBUG" }
        optimize "On"
        buildoptions{"/utf-8"}
        links { 
            "opencv_world460.lib",
            "yaml-cpp.lib"
        }











