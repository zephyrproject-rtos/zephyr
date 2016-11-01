/* fifo.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "syskernel.h"

struct nano_fifo nanoFifo1;
struct nano_fifo nanoFifo2;

static struct nano_fifo nanoFifo_sync; /* for synchronization */


/**
 *
 * @brief Initialize FIFOs for the test
 *
 * @return N/A
 */
void fifo_test_init(void)
{
	nano_fifo_init(&nanoFifo1);
	nano_fifo_init(&nanoFifo2);
}


/**
 *
 * @brief Fifo test fiber
 *
 * @param par1   Ignored parameter.
 * @param par2   Number of test loops.
 *
 * @return N/A
 */
void fifo_fiber1(int par1, int par2)
{
	int i;
	int element[2];
	int *pelement;

	ARG_UNUSED(par1);
	for (i = 0; i < par2; i++) {
		pelement = (int *)nano_fiber_fifo_get(&nanoFifo1,
						      TICKS_UNLIMITED);
		if (pelement[1] != i) {
			break;
		}
		element[1] = i;
		nano_fiber_fifo_put(&nanoFifo2, element);
	}
	/* wait till it is safe to end: */
	nano_fiber_fifo_get(&nanoFifo_sync, TICKS_UNLIMITED);
}


/**
 *
 * @brief Fifo test fiber
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 *
 * @return N/A
 */
void fifo_fiber2(int par1, int par2)
{
	int i;
	int element[2];
	int *pelement;
	int *pcounter = (int *) par1;

	for (i = 0; i < par2; i++) {
		element[1] = i;
		nano_fiber_fifo_put(&nanoFifo1, element);
		pelement = (int *)nano_fiber_fifo_get(&nanoFifo2,
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
 * @brief Fifo test fiber
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 *
 * @return N/A
 */
void fifo_fiber3(int par1, int par2)
{
	int i;
	int element[2];
	int *pelement;
	int *pcounter = (int *)par1;

	for (i = 0; i < par2; i++) {
		element[1] = i;
		nano_fiber_fifo_put(&nanoFifo1, element);
		while ((pelement = nano_fiber_fifo_get(&nanoFifo2,
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
int fifo_test(void)
{
	uint32_t t;
	int i = 0;
	int return_value = 0;
	int element[2];
	int j;

	nano_fifo_init(&nanoFifo_sync);

	/* test get wait & put fiber functions */
	fprintf(output_file, sz_test_case_fmt,
			"FIFO #1");
	fprintf(output_file, sz_description,
			"\n\tnano_fifo_init"
			"\n\tnano_fiber_fifo_get(TICKS_UNLIMITED)"
			"\n\tnano_fiber_fifo_put");
	printf(sz_test_start_fmt);

	fifo_test_init();

	t = BENCH_START();

	task_fiber_start(fiber_stack1, STACK_SIZE, fifo_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	task_fiber_start(fiber_stack2, STACK_SIZE, fifo_fiber2, (int) &i,
					 NUMBER_OF_LOOPS, 3, 0);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	/* fibers have done their job, they can stop now safely: */
	for (j = 0; j < 2; j++) {
		nano_task_fifo_put(&nanoFifo_sync, (void *) element);
	}

	/* test get/yield & put fiber functions */
	fprintf(output_file, sz_test_case_fmt,
			"FIFO #2");
	fprintf(output_file, sz_description,
			"\n\tnano_fifo_init"
			"\n\tnano_fiber_fifo_get(TICKS_UNLIMITED)"
			"\n\tnano_fiber_fifo_get(TICKS_NONE)"
			"\n\tnano_fiber_fifo_put"
			"\n\tfiber_yield");
	printf(sz_test_start_fmt);

	fifo_test_init();

	t = BENCH_START();

	i = 0;
	task_fiber_start(fiber_stack1, STACK_SIZE, fifo_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	task_fiber_start(fiber_stack2, STACK_SIZE, fifo_fiber3, (int) &i,
					 NUMBER_OF_LOOPS, 3, 0);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	/* fibers have done their job, they can stop now safely: */
	for (j = 0; j < 2; j++) {
		nano_task_fifo_put(&nanoFifo_sync, (void *) element);
	}

	/* test get wait & put fiber/task functions */
	fprintf(output_file, sz_test_case_fmt,
			"FIFO #3");
	fprintf(output_file, sz_description,
			"\n\tnano_fifo_init"
			"\n\tnano_fiber_fifo_get(TICKS_UNLIMITED)"
			"\n\tnano_fiber_fifo_put"
			"\n\tnano_task_fifo_get(TICKS_UNLIMITED)"
			"\n\tnano_task_fifo_put");
	printf(sz_test_start_fmt);

	fifo_test_init();

	t = BENCH_START();

	task_fiber_start(fiber_stack1, STACK_SIZE, fifo_fiber1, 0,
					 NUMBER_OF_LOOPS / 2, 3, 0);
	task_fiber_start(fiber_stack2, STACK_SIZE, fifo_fiber1, 0,
					 NUMBER_OF_LOOPS / 2, 3, 0);
	for (i = 0; i < NUMBER_OF_LOOPS / 2; i++) {
		int element[2];
		int *pelement;

		element[1] = i;
		nano_task_fifo_put(&nanoFifo1, element);
		element[1] = i;
		nano_task_fifo_put(&nanoFifo1, element);

		pelement = (int *)nano_task_fifo_get(&nanoFifo2,
						     TICKS_UNLIMITED);
		if (pelement[1] != i) {
			break;
		}
		pelement = (int *)nano_task_fifo_get(&nanoFifo2,
						     TICKS_UNLIMITED);
		if (pelement[1] != i) {
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
