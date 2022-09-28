#ifdef IFCS_RELEASE
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif
#include <filesystem>

#include "Application.h"
#include "Utils.h"


int main()
{
    system("chcp 65001"); // make encoding utf8?
    // auto p = std::filesystem::path("L:/IFCS_DEV_PROJECTS/Great/traning_clips");
    // spdlog::info(p.string());
    //     std::string video_top_path = "L:/IFCS_DEV_PROJECTS/Great/traning_clips";
    //     // std::replace(video_path.begin(), video_path.end(), '/', '\\');
    //     // spdlog::info(video_path);
    //     for (const auto& entry : std::filesystem::recursive_directory_iterator(video_top_path))
    //     {
    //         std::cout << entry.path() << std::endl;
    //         std::cout << entry.path().extension() << std::endl;
    //     }
    
    // create app
    const auto app = new IFCS::Application();
    
    // start app
    app->init();
    
    // loop app
    app->run();
    
    // end app
    delete app;


    return 0;
}
