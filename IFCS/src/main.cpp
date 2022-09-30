#ifdef IFCS_RELEASE
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif
#include "Application.h"
#include <cstdlib>

#define TESTCV 0

#if TESTCV
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#endif

int main()
{
    
#if TESTCV 
// test read vidoe in opencv c++

    std::string filename = "sample.mp4";
    cv::VideoCapture capture(filename);
    capture.set(cv::CAP_PROP_POS_FRAMES, 100);
    cv::Mat frame;

    if( !capture.isOpened() )
        throw "Error when reading steam_mp4";

    cv::namedWindow( "w", 1);
        capture >> frame;
        imshow("w", frame);
        cv::waitKey(20); // waits to display frame
    cv::waitKey(0); // key press to close window
#endif
    
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
