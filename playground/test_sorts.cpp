#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>   // for sqrtf
#include <string.h> // for memcpy
#define LIB_SIMPLE_PROFILER_IMPLEMENTATION
#include "lib_simple_profiler.h"

#define MAX_ARRAY_SIZE   2*1024*1024*1024ll // 2GB enough
#define NUM_TRIALS 10

typedef int (*compare_op_t)(const void* a, const void* b);
typedef void (*sort_algo_t)(void* base, size_t num_of_elements, size_t size_of_elements, compare_op_t fn_compare_op);

#define pointer_offset(base, i, size) ((char*)base + ((i) * (size)))
#define swap_ptr(ptr_a, ptr_b, size) { void* buf = malloc(size); memcpy(buf, ptr_a, size); memcpy(ptr_a, ptr_b, size); memcpy(ptr_b, buf, size); free(buf); }
#define swap_ptr_buf(ptr_a, ptr_b, buf, size) { memcpy(buf, ptr_a, size); memcpy(ptr_a, ptr_b, size); memcpy(ptr_b, buf, size); }
#define algo(fn) { #fn, fn }
#define array_count(arr) (sizeof(arr) / sizeof(arr[0]))
struct RunResults
{
	uint64_t min;
	uint64_t max;
	uint64_t avg;
	uint64_t std;
};

enum InitType
{
	INIT_TYPE_ORDERED
};

struct SortAlgoData
{
	const char* name;
	sort_algo_t fn_sort;
};

void format_nanosecs(uint64_t time, char* buf)
{
	const char* prefixes[] = { "ns", "is", "ms", "s"};
	uint64_t frac = 0;

	int prefix = 0;
	while (time >= 1000 && prefix < array_count(prefixes))
	{
		frac = time % 1000;
		time /= 1000;
		++prefix;
	}

	sprintf(buf, "%3llu.%03llu%s", time, frac, prefixes[prefix]);
}

int compare_integers(const void* a, const void* b)
{
	return *(const uint64_t*)a - *(const uint64_t*)b;
}

bool sort_check(void* base, size_t num_of_elements, size_t size_of_elements, compare_op_t fn_compare_op)
{
	for (size_t i = 0; i < num_of_elements - 1; ++i)
	{
		void* a = pointer_offset(base, i, size_of_elements);
		void* b = pointer_offset(base, i + 1, size_of_elements);
		if(fn_compare_op(a, b) > 0)
			return false;
	}

	return true;
}

void sort_selection(void* base, size_t num_of_elements, size_t size_of_elements, compare_op_t fn_compare_op)
{
	if(num_of_elements == 0)
		return;
	for (size_t i = 0; i < num_of_elements - 1; ++i)
	{
		void* element_i = pointer_offset(base, i, size_of_elements);
		void* element_min = element_i;
		for(int j = i+1; j < num_of_elements; ++j)
		{
			void* element_j = pointer_offset(base, j, size_of_elements);
			if (fn_compare_op(element_j, element_min) < 0)
				element_min = element_j;
		}

		if (element_min != element_i)
		{
			// swap
			swap_ptr(element_i, element_min, size_of_elements);
		}
	}

	assert(sort_check(base, num_of_elements, size_of_elements, fn_compare_op));
}

void sort_selection_no_alloc(void* base, size_t num_of_elements, size_t size_of_elements, compare_op_t fn_compare_op)
{
	if(num_of_elements == 0)
		return;
	
	void* buf = malloc(size_of_elements);
	for (size_t i = 0; i < num_of_elements - 1; ++i)
	{
		void* element_i = pointer_offset(base, i, size_of_elements);
		void* element_min = element_i;
		for(int j = i+1; j < num_of_elements; ++j)
		{
			void* element_j = pointer_offset(base, j, size_of_elements);
			if (fn_compare_op(element_j, element_min) < 0)
				element_min = element_j;
		}

		if (element_min != element_i)
		{
			// swap
			swap_ptr_buf(element_i, element_min, buf, size_of_elements);
		}
	}
	free(buf);

	assert(sort_check(base, num_of_elements, size_of_elements, fn_compare_op));
}

void sort_selection_ints(void* base, size_t num_of_elements, size_t size_of_elements, compare_op_t fn_compare_op)
{
	if(num_of_elements == 0)
		return;
	
	uint64_t* base_int = (uint64_t*)base;
	for (size_t i = 0; i < num_of_elements - 1; ++i)
	{
		size_t idx_min = i;
		for(int j = i+1; j < num_of_elements; ++j)
		{
			if (base_int[j] < base_int[idx_min])
				idx_min = j;
		}

		// if (idx_min != i) swapping is almost free, no need to bother with the test probably
		{
			// swap
			uint64_t buf;
			buf = base_int[i];
			base_int[i] = base_int[idx_min];
			base_int[idx_min] = buf;
		}
	}

	assert(sort_check(base, num_of_elements, size_of_elements, fn_compare_op));
}

int main(void)
{
	InitType init_type = INIT_TYPE_ORDERED;

	uint64_t sizes[] = {
		0,
		1, 2, 8, 32, 1024,
		//65536,
		// 100000,
		//1000000,
		// 1000000000
	};
	int sizes_count = array_count(sizes);

	SortAlgoData algorithms[] = {
		algo(sort_selection),
		algo(sort_selection_no_alloc),
		algo(sort_selection_ints),
		algo(qsort)
	};
	int algorithms_count = array_count(algorithms);
	uint64_t** arrays = (uint64_t**)calloc(sizes_count, sizeof(uint64_t*));
	RunResults** results = (RunResults**)calloc(algorithms_count, sizeof(RunResults*));
	uint64_t* work_array = (uint64_t*)malloc(MAX_ARRAY_SIZE);

	// initialize

	for (int i = 0; i < sizes_count; ++i)
	{
		for(int j = 0; j < algorithms_count; ++j)
			results[j] = (RunResults*)calloc(sizes_count, sizeof(RunResults));

		size_t size = sizes[i];

		if(size == 0)
			size = 1; // for unix compatibility

		// check we are not using too big array sizes
		assert(size * sizeof(uint64_t*) <= MAX_ARRAY_SIZE);
		arrays[i] = (uint64_t*)malloc(size * sizeof(uint64_t));

		for(uint64_t j = 0; j < size; ++j)
			switch (init_type)
			{
				case INIT_TYPE_ORDERED: arrays[i][j] = j; break;
			}
	}

	// run trials
	for (int i = 0; i < algorithms_count; ++i)
	{
		printf(" === %24s ============\n", algorithms[i].name);
		for (int j = 0; j < sizes_count; ++j)
		{
			printf("\tN: %12llu [", sizes[j]);
			RunResults tmp_results = { 0 };
			tmp_results.min = -1;
			uint64_t run_results[NUM_TRIALS];
			
			for (int k = 0; k < NUM_TRIALS; ++k)
			{
				putc('X', stdout);
				memcpy(work_array, arrays[j], sizeof(uint64_t)*sizes[j]);

				lib_profiler_sample_beg();
				algorithms[i].fn_sort(work_array, sizes[j], sizeof(uint64_t), compare_integers);
				run_results[k] = lib_profiler_sample_end();

				if(run_results[k] < tmp_results.min)
					tmp_results.min = run_results[k];
				if(run_results[k] > tmp_results.max)
					tmp_results.max = run_results[k];
				tmp_results.avg += run_results[k];
			}
			// aggregate data
			{
				tmp_results.avg /= NUM_TRIALS;
				for (int k = 0; k < NUM_TRIALS; ++k)
					tmp_results.std += (run_results[k] - tmp_results.avg)*(run_results[k] - tmp_results.avg);
				tmp_results.std = sqrtf(tmp_results.std / NUM_TRIALS);
				results[i][j] = tmp_results;
			}
			printf("]\n");
		}
	}

	// print results
	{
		printf("\n ==================================================================================================================== \n");

		// header
		printf("%10s", "");
		for (int i = 0; i < algorithms_count; ++i)
			printf("%25s", algorithms[i].name);
		putc('\n', stdout);
		for (int j = 0; j < sizes_count; ++j)
		{
			printf("%10llu", sizes[j]);
			for (int i = 0; i < algorithms_count; ++i)
			{
				char buf_avg[64];
				char buf_std[64];
				format_nanosecs(results[i][j].avg, buf_avg);
				format_nanosecs(results[i][j].std, buf_std);
				printf(
					"%14s(%9s)",
					buf_avg,
					buf_std
				);
			}
				//printf("%16llu(%8llu)", results[i][j].avg, results[i][j].std);
			putc('\n', stdout);
		}
	}


	return 0;
}