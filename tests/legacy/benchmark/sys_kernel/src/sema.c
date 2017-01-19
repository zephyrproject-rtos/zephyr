/* sema.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "syskernel.h"

struct nano_sem nanoSem1;
struct nano_sem nanoSem2;

/**
 *
 * @brief Initialize semaphores for the test
 *
 * @return N/A
 */
void sema_test_init(void)
{
	nano_sem_init(&nanoSem1);
	nano_sem_init(&nanoSem2);
}


/**
 *
 * @brief Semaphore test fiber
 *
 * @param par1   Ignored parameter.
 * @param par2   Number of test loops.
 *
 * @return N/A
 */
void sema_fiber1(int par1, int par2)
{
	int i;

	ARG_UNUSED(par1);

	for (i = 0; i < par2; i++) {
		nano_fiber_sem_take(&nanoSem1, TICKS_UNLIMITED);
		nano_fiber_sem_give(&nanoSem2);
	}
}


/**
 *
 * @brief Semaphore test fiber
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 *
 * @return N/A
 */
void sema_fiber2(int par1, int par2)
{
	int i;
	int *pcounter = (int *)par1;

	for (i = 0; i < par2; i++) {
		nano_fiber_sem_give(&nanoSem1);
		nano_fiber_sem_take(&nanoSem2, TICKS_UNLIMITED);
		(*pcounter)++;
	}
}

/**
 *
 * @brief Semaphore test fiber
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 *
 * @return N/A
 */
void sema_fiber3(int par1, int par2)
{
	int i;
	int *pcounter = (int *)par1;

	for (i = 0; i < par2; i++) {
		nano_fiber_sem_give(&nanoSem1);
		while (!nano_fiber_sem_take(&nanoSem2, TICKS_NONE)) {
			fiber_yield();
		}
		(*pcounter)++;
	}
}


/**
 *
 * @brief The main test entry
 *
 * @return 1 if success and 0 on failure
 */
int sema_test(void)
{
	uint32_t t;
	int i = 0;
	int return_value = 0;

	fprintf(output_file, sz_test_case_fmt,
			"Semaphore #1");
	fprintf(output_file, sz_description,
			"\n\tnano_sem_init"
			"\n\tnano_fiber_sem_take(TICKS_UNLIMITED)"
			"\n\tnano_fiber_sem_give");
	printf(sz_test_start_fmt);

	sema_test_init();

	t = BENCH_START();

	task_fiber_start(fiber_stack1, STACK_SIZE, sema_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	task_fiber_start(fiber_stack2, STACK_SIZE, sema_fiber2, (int) &i,
					 NUMBER_OF_LOOPS, 3, 0);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	fprintf(output_file, sz_test_case_fmt,
			"Semaphore #2");
	fprintf(output_file, sz_description,
			"\n\tnano_sem_init"
			"\n\tnano_fiber_sem_take(TICKS_NONE)"
			"\n\tfiber_yield"
			"\n\tnano_fiber_sem_give");
	printf(sz_test_start_fmt);

	sema_test_init();
	i = 0;

	t = BENCH_START();

	task_fiber_start(fiber_stack1, STACK_SIZE, sema_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	task_fiber_start(fiber_stack2, STACK_SIZE, sema_fiber3, (int) &i,
					 NUMBER_OF_LOOPS, 3, 0);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	fprintf(output_file, sz_test_case_fmt,
			"Semaphore #3");
	fprintf(output_file, sz_description,
			"\n\tnano_sem_init"
			"\n\tnano_fiber_sem_take(TICKS_UNLIMITED)"
			"\n\tnano_fiber_sem_give"
			"\n\tnano_task_sem_give"
			"\n\tnano_task_sem_take(TICKS_UNLIMITED)");
	printf(sz_test_start_fmt);

	sema_test_init();

	t = BENCH_START();

	task_fiber_start(fiber_stack1, STACK_SIZE, sema_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	for (i = 0; i < NUMBER_OF_LOOPS; i++) {
		nano_task_sem_give(&nanoSem1);
		nano_task_sem_take(&nanoSem2, TICKS_UNLIMITED);
	}

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	return return_value;
}
