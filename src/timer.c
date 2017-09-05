#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include "messaging.h"

void startTimer(Timespec* timer)
{
    if(clock_gettime(CLOCK_MONOTONIC, timer))
    {
        Error("Error getting current time");
    }
}

unsigned long elapsedMillis(Timespec* timer)
{
    Timespec current;
    if(clock_gettime(CLOCK_MONOTONIC, &current))
    {
        Error("Error getting current time");
    }

    unsigned long ret;

    ret = (current.tv_sec - timer->tv_sec) * 1000;
    ret += (current.tv_nsec - timer->tv_nsec) / 1000000;

    return ret;
}