#include <clocale>

#include "Application.h"
#include <cstdlib>

int main()
{
    setlocale(LC_ALL, ".UTF8");
    
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
