#include <clocale>

#include "Application.h"
#include <cstdlib>

int main()
{
    setlocale(LC_ALL, ".UTF8");
    
    // create app
    const auto App = new IFCS::Application("IFCS_DEV", 1920, 1080, false);
    
    // start app
    App->Init();
    
    // loop app
    App->Run();
    
    // end app
    delete App;

    return 0;
}

