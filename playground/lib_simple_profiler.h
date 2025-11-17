#ifndef LIB_SIMPLE_PROFILER_H
#define LIB_SIMPLE_PROFILER_H

#include <stdint.h>

void lib_profiler_sample_beg();		// starts a sample
uint64_t lib_profiler_sample_end();	// ends a sample and returns the number of elapsed NANOSECONDS

#endif // LIB_SIMPLE_PROFILER_H


// win version
#ifdef LIB_SIMPLE_PROFILER_IMPLEMENTATION

#if WIN32
#include <windows.h>

uint64_t get_performance_frequency()
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

	return freq.QuadPart;
}

uint64_t get_performance_counter()
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	return counter.QuadPart;
}

uint64_t performance_frequency = get_performance_frequency();
uint64_t sample_beg;

void lib_profiler_sample_beg()
{
	sample_beg = get_performance_counter();
}

uint64_t lib_profiler_sample_end()
{
	uint64_t elapsed = get_performance_counter() - sample_beg;

	// convert to nansecs
	elapsed *= 1000000000;
	return elapsed / performance_frequency;

}
#endif // WIN32
#endif //LIB_SIMPLE_PROFILER_IMPLEMENTATION