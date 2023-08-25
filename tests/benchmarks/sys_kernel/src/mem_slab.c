/* memslab.c */

/*
 * Copyright (c) 2020, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "syskernel.h"

#define MEM_SLAB_BLOCK_SIZE  (8)
#define MEM_SLAB_BLOCK_CNT   (NUMBER_OF_LOOPS)
#define MEM_SLAB_BLOCK_ALIGN (4)

K_MEM_SLAB_DEFINE_STATIC(my_slab,
		  MEM_SLAB_BLOCK_SIZE,
		  MEM_SLAB_BLOCK_CNT,
		  MEM_SLAB_BLOCK_ALIGN);

/* Array contains pointers to allocated regions. */
static void *slab_array[MEM_SLAB_BLOCK_CNT];

/**
 *
 * @brief Memslab allocation test function.
 *		  This test allocates available memory space.
 *
 * @param no_of_loops  Amount of loops to run.
 * @param test_repeats Amount of test repeats per loop.
 *
 * @return NUmber of done loops.
 */
static int mem_slab_alloc_test(int no_of_loops)
{
	int i;

	for (i = 0; i < no_of_loops; i++) {
		if (k_mem_slab_alloc(&my_slab, &slab_array[i], K_NO_WAIT)
		    != 0) {
			return i;
		}
	}

	return i;
}

/**
 *
 * @brief Memslab free test function.
 *		  This test frees memory previously allocated in
 *		  @ref mem_slab_alloc_test.
 *
 * @param no_of_loops  Amount of loops to run.
 * @param test_repeats Amount of test repeats per loop.
 *
 * @return NUmber of done loops.
 */
static int mem_slab_free_test(int no_of_loops)
{
	int i;

	for (i = 0; i < no_of_loops; i++) {
		k_mem_slab_free(&my_slab, slab_array[i]);
	}

	return i;
}

int mem_slab_test(void)
{
	uint32_t t;
	int i = 0;
	int return_value = 0;

	/* Test k_mem_slab_alloc. */
	fprintf(output_file, sz_test_case_fmt,
		"Memslab #1");
	fprintf(output_file, sz_description,
		"\n\tk_mem_slab_alloc");
	printf(sz_test_start_fmt);

	t = BENCH_START();
	i = mem_slab_alloc_test(number_of_loops);
	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	/* Test k_mem_slab_free. */
	fprintf(output_file, sz_test_case_fmt,
		"Memslab #2");
	fprintf(output_file, sz_description,
		"\n\tk_mem_slab_free");
	printf(sz_test_start_fmt);

	t = BENCH_START();
	i = mem_slab_free_test(number_of_loops);
	t = TIME_STAMP_DELTA_GET(t);

	/* Check if all slabs were freed. */
	if (k_mem_slab_num_used_get(&my_slab) != 0) {
		i = 0;
	}

	return_value += check_result(i, t);

	return return_value;
}
