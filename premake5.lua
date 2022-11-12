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
        "%{prj.name}/vendor/imgui/*.h",
        "%{prj.name}/vendor/imgui/*.cpp",
        "%{prj.name}/vendor/imgui/misc/cpp/imgui_stdlib.h",
        "%{prj.name}/vendor/imgui/misc/cpp/imgui_stdlib.cpp",
        "%{prj.name}/vendor/imgui/backends/imgui_impl_glfw.h",
        "%{prj.name}/vendor/imgui/backends/imgui_impl_glfw.cpp",
        "%{prj.name}/vendor/imgui/backends/imgui_impl_opengl3.h",
        "%{prj.name}/vendor/imgui/backends/imgui_impl_opengl3.cpp",
        "%{prj.name}/vendor/imgui/backends/imgui_impl_opengl3_loader.h",
        "%{prj.name}/vendor/imgui_extensions/ImFileDialog/*.h",
        "%{prj.name}/vendor/imgui_extensions/ImFileDialog/*.cpp",
        "%{prj.name}/vendor/imgui_extensions/Implot/*.h",
        "%{prj.name}/vendor/imgui_extensions/Implot/*.cpp",
        "%{prj.name}/vendor/stb/**.h", 
        "%{prj.name}/vendor/stb/**.cpp",
--         "%{prj.name}/vendor/imgui_extensions/Imspinner/*.h", 
--         "%{prj.name}/vendor/imgui_extensions/IconFontCppHeaders/IconsFontAwesome5.h", 
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
        "Dependencies/Python/include",
        "%(AdditionalIncludeDirectories)"
    }

    libdirs 
    { 
        "Dependencies/glfw/lib-vc2019",
        "Dependencies/opencv/build/x64/vc15/lib",
        "Dependencies/yaml-cpp/lib",
        "Dependencies/Python/libs",
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
            "yaml-cppd.lib",
            "python310_d.lib"
        }

    filter "configurations:Release"
        defines { "IFCS_RELEASE", "NDEBUG" }
        optimize "On"
        buildoptions{"/utf-8"}
        links { 
            "opencv_world460.lib",
            "yaml-cpp.lib",
            "python310.lib"
        }











