#ifdef IFCS_RELEASE
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif
#include "Application.h"


int main()
{
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