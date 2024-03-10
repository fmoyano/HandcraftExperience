#ifndef TIMING_H
#define TIMING_H

#include "profileapi.h"

static long long get_OS_timer_frequency()
{
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return frequency.QuadPart;
}

static long long get_OS_timer()
{
	LARGE_INTEGER value;
	QueryPerformanceCounter(&value);
	return value.QuadPart;
}

static unsigned long long get_CPU_Frequency()
{
	long long freq = get_OS_timer_frequency();
	long long timer_start = get_OS_timer();
	long long timer_end = 0;
	long long timer_elapsed = 0;

	unsigned long long CPU_timer_start = __rdtsc();
	while (timer_elapsed < freq)
	{
		timer_end = get_OS_timer();
		timer_elapsed = timer_end - timer_start;
	}
	unsigned long long CPU_timer_end = __rdtsc();
	unsigned long long CPU_timer_elapsed = CPU_timer_end - CPU_timer_start;

	return CPU_timer_elapsed;
}

#endif