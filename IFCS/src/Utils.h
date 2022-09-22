# pragma once
#include <ctime>
#include <string>


namespace IFCS 
{
    static char* GetCurrentTimeString()
    {
        time_t curr_time;
        char out_string[200];

        time(&curr_time);
        tm* LocalTime = localtime(&curr_time);
        strftime(out_string, 100, "%T %Y/%m/%d", LocalTime);
        return out_string;
    }
    
}
