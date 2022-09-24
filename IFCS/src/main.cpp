#ifdef IFCS_RELEASE
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif
#include "Application.h"
#include "Utils.h"


int main()
{
    system("chcp 65001"); // make encoding utf8?

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
