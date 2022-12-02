#include "Application.h"
#include <cstdlib>

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
