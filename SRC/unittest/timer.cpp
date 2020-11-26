#include "timer.h"

#ifdef __GNUC__

#include <cstdio>
#include <sys/time.h>

struct timeval start, stop;
void start_timer(void)
{
    gettimeofday(&start, NULL);
}

long long stop_timer_us(void)
{
    gettimeofday(&stop, NULL);
    long long rv;
    rv = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
    return rv;
}

#else
#include <Windows.h>
#include <profileapi.h>

LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
LARGE_INTEGER Frequency;

void start_timer(void)
{
    QueryPerformanceCounter(&StartingTime);
}

long long stop_timer_us(void)
{
    LARGE_INTEGER EndingTime;
    QueryPerformanceCounter(&EndingTime);

    LARGE_INTEGER ElapsedMicroseconds;
    ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;

    long long rv = (long long)ElapsedMicroseconds.QuadPart;
    return rv;
}

#endif	// WIN32

