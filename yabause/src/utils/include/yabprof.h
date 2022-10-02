#ifndef __YAB_PROF_H
#define __YAB_PROF_H

#define YPROFILER

#ifdef YPROFILER
#include <Windows.h>

//https://stackoverflow.com/questions/1739259/how-to-use-queryperformancecounter
#define YPROF_INIT \
	static unsigned long long _yprof_execs = 0; \
	LARGE_INTEGER _yprof_f, _yprof_t1, _yprof_t2; \
	if (!QueryPerformanceFrequency(&_yprof_f)) \
		printf("QueryPerformanceFrequency failed\n");\
	if (!QueryPerformanceCounter(&_yprof_t1)) \
		printf("QueryPerformanceCounter failed\n");\
	++_yprof_execs;
#define YPROF(function) \
	if (!QueryPerformanceCounter(&_yprof_t2)) \
		printf("Performance Counter failed\n"); \
	printf("%s #%d\t %lld\n", function, _yprof_execs, (_yprof_t2.QuadPart - _yprof_t1.QuadPart)); \
	fflush(stdout);
#else
#define YPROF_INIT
#define YPROF(function)
#endif

#if 0
/*unsigned long long yabprof_execs = 0;
LARGE_INTEGER yabprof_freq;
LARGE_INTEGER yabprof_stamp;

#define YAB_PROF_START(function) QueryPerformanceFrequency(&yabprof_freq); if (!QueryPerformanceCounter(&yabprof_stamp)) { printf("Performance Counter failed\n"); }; printf("%s #%d start\t\t %d\n", function, ++yabprof_execs, yabprof_stamp.QuadPart); fflush(stdout);
#define YAB_PROF_END(function)   if (!QueryPerformanceCounter(&yabprof_stamp)) { printf("Performance Counter failed\n"); }; printf("\t%s #%d done\t\t %d\n", function, ++yabprof_execs, yabprof_stamp.QuadPart); fflush(stdout);
*/

/** Use to init the clock */
#define TIMER_INIT \
    LARGE_INTEGER _yprof_freq; \
    LARGE_INTEGER t1,t2; \
    double elapsedTime; \
    QueryPerformanceFrequency(&_yprof_freq);


/** Use to start the performance timer */
#define TIMER_START QueryPerformanceCounter(&t1);

/** Use to stop the performance timer and output the result to the standard stream. Less verbose than \c TIMER_STOP_VERBOSE */
#define TIMER_STOP \
    QueryPerformanceCounter(&t2); \
    elapsedTime=(float)(t2.QuadPart-t1.QuadPart)/frequency.QuadPart; \
    std::wcout<<elapsedTime<<L" sec"<<endl;
#endif
#endif