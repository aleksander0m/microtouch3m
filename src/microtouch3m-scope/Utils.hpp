#ifndef MICROTOUCH3M_SCOPE_UTILS_HPP
#define MICROTOUCH3M_SCOPE_UTILS_HPP

#include <string>

#include <time.h>

namespace Utils {
    inline timespec timespec_diff(const timespec &start, const timespec &stop)
    {
        timespec ret;

        if ((stop.tv_nsec - start.tv_nsec) < 0)
        {
            ret.tv_sec = stop.tv_sec - start.tv_sec - 1;
            ret.tv_nsec = stop.tv_nsec - start.tv_nsec + 1000000000;
        }
        else
        {
            ret.tv_sec = stop.tv_sec - start.tv_sec;
            ret.tv_nsec = stop.tv_nsec - start.tv_nsec;
        }

        return ret;
    }

    std::string ipv4_string();
};

#endif // MICROTOUCH3M_SCOPE_UTILS_HPP
