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
        "%{prj.name}/vendor/**.h", 
        "%{prj.name}/vendor/**.cpp",
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
        "Dependencies/glfw/include",
        "Dependencies/opencv/build/include",
        "%(AdditionalIncludeDirectories)"
    }

    libdirs 
    { 
        "Dependencies/glfw/lib-vc2019",
        "Dependencies/opencv/build/x64/vc15/lib",
        "%(AdditionalLibraryDirectories)"
    }
    links
    {
        "glfw3.lib",
        "opengl32.lib",
    }

    filter "configurations:Debug"
        defines { "IFCS_DEBUG" }
        symbols "On"
        buildoptions {"/utf-8"}
        links { "opencv_world460d.lib" }

    filter "configurations:Release"
        defines { "IFCS_RELEASE" }
        optimize "On"
        buildoptions{"/utf-8"}
        links { "opencv_world460.lib" }











