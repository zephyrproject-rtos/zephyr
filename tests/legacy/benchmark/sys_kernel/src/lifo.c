/* lifo.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "syskernel.h"

struct nano_lifo nanoLifo1;
struct nano_lifo nanoLifo2;

static struct nano_fifo nanoFifo_sync; /* for synchronization */

/**
 *
 * @brief Initialize LIFOs for the test
 *
 * @return N/A
 */
void lifo_test_init(void)
{
	nano_lifo_init(&nanoLifo1);
	nano_lifo_init(&nanoLifo2);
}


/**
 *
 * @brief Lifo test fiber
 *
 * @param par1   Ignored parameter.
 * @param par2   Number of test loops.
 *
 * @return N/A
 */
void lifo_fiber1(int par1, int par2)
{
	int i;
	int element_a[2];
	int element_b[2];
	int *pelement;

	ARG_UNUSED(par1);

	for (i = 0; i < par2 / 2; i++) {
		pelement = (int *)nano_fiber_lifo_get(&nanoLifo1,
						      TICKS_UNLIMITED);
		if (pelement[1] != 2 * i) {
			break;
		}
		element_a[1] = 2 * i;
		nano_fiber_lifo_put(&nanoLifo2, element_a);
		pelement = (int *)nano_fiber_lifo_get(&nanoLifo1,
						      TICKS_UNLIMITED);
		if (pelement[1] != 2 * i + 1) {
			break;
		}
		element_b[1] = 2 * i + 1;
		nano_fiber_lifo_put(&nanoLifo2, element_b);
	}
	/* wait till it is safe to end: */
	nano_fiber_fifo_get(&nanoFifo_sync, TICKS_UNLIMITED);
}


/**
 *
 * @brief Lifo test fiber
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 *
 * @return N/A
 */
void lifo_fiber2(int par1, int par2)
{
	int i;
	int element[2];
	int *pelement;
	int *pcounter = (int *)par1;

	for (i = 0; i < par2; i++) {
		element[1] = i;
		nano_fiber_lifo_put(&nanoLifo1, element);
		pelement = (int *)nano_fiber_lifo_get(&nanoLifo2,
						      TICKS_UNLIMITED);
		if (pelement[1] != i) {
			break;
		}
		(*pcounter)++;
	}
	/* wait till it is safe to end: */
	nano_fiber_fifo_get(&nanoFifo_sync, TICKS_UNLIMITED);
}

/**
 *
 * @brief Lifo test fiber
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test loops.
 *
 * @return N/A
 */
void lifo_fiber3(int par1, int par2)
{
	int i;
	int element[2];
	int *pelement;
	int *pcounter = (int *)par1;

	for (i = 0; i < par2; i++) {
		element[1] = i;
		nano_fiber_lifo_put(&nanoLifo1, element);
		while ((pelement = nano_fiber_lifo_get(&nanoLifo2,
							TICKS_NONE)) == NULL) {
			fiber_yield();
		}
		if (pelement[1] != i) {
			break;
		}
		(*pcounter)++;
	}
	/* wait till it is safe to end: */
	nano_fiber_fifo_get(&nanoFifo_sync, TICKS_UNLIMITED);
}

/**
 *
 * @brief The main test entry
 *
 * @return 1 if success and 0 on failure
 */
int lifo_test(void)
{
	uint32_t t;
	int i = 0;
	int return_value = 0;
	int element[2];
	int j;

	nano_fifo_init(&nanoFifo_sync);

	/* test get wait & put fiber functions */
	fprintf(output_file, sz_test_case_fmt,
			"LIFO #1");
	fprintf(output_file, sz_description,
			"\n\tnano_lifo_init"
			"\n\tnano_fiber_lifo_get(TICKS_UNLIMITED)"
			"\n\tnano_fiber_lifo_put");
	printf(sz_test_start_fmt);

	lifo_test_init();

	t = BENCH_START();

	task_fiber_start(fiber_stack1, STACK_SIZE, lifo_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	task_fiber_start(fiber_stack2, STACK_SIZE, lifo_fiber2, (int) &i,
					 NUMBER_OF_LOOPS, 3, 0);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	/* fibers have done their job, they can stop now safely: */
	for (j = 0; j < 2; j++) {
		nano_task_fifo_put(&nanoFifo_sync, (void *) element);
	}

	/* test get/yield & put fiber functions */
	fprintf(output_file, sz_test_case_fmt,
			"LIFO #2");
	fprintf(output_file, sz_description,
			"\n\tnano_lifo_init"
			"\n\tnano_fiber_lifo_get(TICKS_UNLIMITED)"
			"\n\tnano_fiber_lifo_get(TICKS_NONE)"
			"\n\tnano_fiber_lifo_put"
			"\n\tfiber_yield");
	printf(sz_test_start_fmt);

	lifo_test_init();

	t = BENCH_START();

	i = 0;
	task_fiber_start(fiber_stack1, STACK_SIZE, lifo_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	task_fiber_start(fiber_stack2, STACK_SIZE, lifo_fiber3, (int) &i,
					 NUMBER_OF_LOOPS, 3, 0);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	/* fibers have done their job, they can stop now safely: */
	for (j = 0; j < 2; j++) {
		nano_task_fifo_put(&nanoFifo_sync, (void *) element);
	}

	/* test get wait & put fiber/task functions */
	fprintf(output_file, sz_test_case_fmt,
			"LIFO #3");
	fprintf(output_file, sz_description,
			"\n\tnano_lifo_init"
			"\n\tnano_fiber_lifo_get(TICKS_UNLIMITED)"
			"\n\tnano_fiber_lifo_put"
			"\n\tnano_task_lifo_get(TICKS_UNLIMITED)"
			"\n\tnano_task_lifo_put");
	printf(sz_test_start_fmt);

	lifo_test_init();

	t = BENCH_START();

	task_fiber_start(fiber_stack1, STACK_SIZE, lifo_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	for (i = 0; i < NUMBER_OF_LOOPS / 2; i++) {
		int element[2];
		int *pelement;

		element[1] = 2 * i;
		nano_task_lifo_put(&nanoLifo1, element);
		element[1] = 2 * i + 1;
		nano_task_lifo_put(&nanoLifo1, element);

		pelement = (int *)nano_task_lifo_get(&nanoLifo2,
						     TICKS_UNLIMITED);
		if (pelement[1] != 2 * i + 1) {
			break;
		}
		pelement = (int *)nano_task_lifo_get(&nanoLifo2,
						     TICKS_UNLIMITED);
		if (pelement[1] != 2 * i) {
			break;
		}
	}

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i * 2, t);

	/* fibers have done their job, they can stop now safely: */
	for (j = 0; j < 2; j++) {
		nano_task_fifo_put(&nanoFifo_sync, (void *) element);
	}

	return return_value;
}
