//
// Timer.h - Simple timers for Windows and Linux
//
// Author: Louis Bavoil
// Email: lbavoil@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#ifndef _TIMER_H_
#define _TIMER_H_

#ifndef _WIN32

#include <sys/time.h>
#include <unistd.h>

#if 1
inline double currentSeconds()
{
     struct timeval now_time;
     gettimeofday(&now_time, 0);
     return now_time.tv_sec + now_time.tv_usec*1.e-6;
}
#else
inline double currentSeconds()
{
     return (double)clock() / CLOCKS_PER_SEC;
}
#endif

inline unsigned currentMS() 
{
     struct timeval now_time;
     gettimeofday(&now_time, 0);
     return now_time.tv_sec*1000 + now_time.tv_usec/1000;
}

#else

#include <windows.h>

double currentSeconds() {
     return (double)GetTickCount() / 1000;
}

unsigned currentMS() {
     return GetTickCount();
}

#endif

#endif
