/* stack.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "syskernel.h"

struct nano_stack nano_stack_1;
struct nano_stack nano_stack_2;

uint32_t stack1[2];
uint32_t stack2[2];

/**
 *
 * @brief Initialize stacks for the test
 *
 * @return N/A
 *
 */
void stack_test_init(void)
{
	nano_stack_init(&nano_stack_1, stack1);
	nano_stack_init(&nano_stack_2, stack2);
}


/**
 *
 * @brief Stack test fiber
 *
 * @param par1   Ignored parameter.
 * @param par2   Number of test loops.
 *
 * @return N/A
 *
 */
void stack_fiber1(int par1, int par2)
{
	int i;
	uint32_t data;

	ARG_UNUSED(par1);

	for (i = 0; i < par2 / 2; i++) {
		nano_fiber_stack_pop(&nano_stack_1, &data, TICKS_UNLIMITED);
		if (data != 2 * i) {
			break;
		}
		data = 2 * i;
		nano_fiber_stack_push(&nano_stack_2, data);
		nano_fiber_stack_pop(&nano_stack_1, &data, TICKS_UNLIMITED);
		if (data != 2 * i + 1) {
			break;
		}
		data = 2 * i + 1;
		nano_fiber_stack_push(&nano_stack_2, data);
	}
}


/**
 *
 * @brief Stack test fiber
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 *
 * @return N/A
 *
 */
void stack_fiber2(int par1, int par2)
{
	int i;
	uint32_t data;
	int *pcounter = (int *)par1;

	for (i = 0; i < par2; i++) {
		data = i;
		nano_fiber_stack_push(&nano_stack_1, data);
		nano_fiber_stack_pop(&nano_stack_2, &data, TICKS_UNLIMITED);
		if (data != i) {
			break;
		}
		(*pcounter)++;
	}
}


/**
 *
 * @brief Stack test fiber
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 *
 * @return N/A
 *
 */
void stack_fiber3(int par1, int par2)
{
	int i;
	uint32_t data;
	int *pcounter = (int *)par1;

	for (i = 0; i < par2; i++) {
		data = i;
		nano_fiber_stack_push(&nano_stack_1, data);
		data = 0xffffffff;
		while (!nano_fiber_stack_pop(&nano_stack_2, &data,
					     TICKS_NONE)) {
			fiber_yield();
		}
		if (data != i) {
			break;
		}
		(*pcounter)++;
	}
}


/**
 *
 * @brief The main test entry
 *
 * @return 1 if success and 0 on failure
 *
 */
int stack_test(void)
{
	uint32_t t;
	int i = 0;
	int return_value = 0;

	/* test get wait & put fiber functions */
	fprintf(output_file, sz_test_case_fmt,
			"Stack #1");
	fprintf(output_file, sz_description,
			"\n\tnano_stack_init"
			"\n\tnano_fiber_stack_pop(TICKS_UNLIMITED)"
			"\n\tnano_fiber_stack_push");
	printf(sz_test_start_fmt);

	stack_test_init();

	t = BENCH_START();

	task_fiber_start(fiber_stack1, STACK_SIZE, stack_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	task_fiber_start(fiber_stack2, STACK_SIZE, stack_fiber2, (int) &i,
					 NUMBER_OF_LOOPS, 3, 0);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	/* test get/yield & put fiber functions */
	fprintf(output_file, sz_test_case_fmt,
			"Stack #2");
	fprintf(output_file, sz_description,
			"\n\tnano_stack_init"
			"\n\tnano_fiber_stack_pop(TICKS_UNLIMITED)"
			"\n\tnano_fiber_stack_pop"
			"\n\tnano_fiber_stack_push"
			"\n\tfiber_yield");
	printf(sz_test_start_fmt);

	stack_test_init();

	t = BENCH_START();

	i = 0;
	task_fiber_start(fiber_stack1, STACK_SIZE, stack_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	task_fiber_start(fiber_stack2, STACK_SIZE, stack_fiber3, (int) &i,
					 NUMBER_OF_LOOPS, 3, 0);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	/* test get wait & put fiber/task functions */
	fprintf(output_file, sz_test_case_fmt,
			"Stack #3");
	fprintf(output_file, sz_description,
			"\n\tnano_stack_init"
			"\n\tnano_fiber_stack_pop(TICKS_UNLIMITED)"
			"\n\tnano_fiber_stack_push"
			"\n\tnano_task_stack_pop(TICKS_UNLIMITED)"
			"\n\tnano_task_stack_push");
	printf(sz_test_start_fmt);

	stack_test_init();

	t = BENCH_START();

	task_fiber_start(fiber_stack1, STACK_SIZE, stack_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	for (i = 0; i < NUMBER_OF_LOOPS / 2; i++) {
		uint32_t data;

		data = 2 * i;
		nano_task_stack_push(&nano_stack_1, data);
		data = 2 * i + 1;
		nano_task_stack_push(&nano_stack_1, data);

		nano_task_stack_pop(&nano_stack_2, &data, TICKS_UNLIMITED);
		if (data != 2 * i + 1) {
			break;
		}
		nano_task_stack_pop(&nano_stack_2, &data, TICKS_UNLIMITED);
		if (data != 2 * i) {
			break;
		}
	}

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i * 2, t);

	return return_value;
}
