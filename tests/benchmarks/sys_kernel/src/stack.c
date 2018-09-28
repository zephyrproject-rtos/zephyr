/* stack.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "syskernel.h"

struct k_stack  stack_1;
struct k_stack  stack_2;

u32_t stack1[2];
u32_t stack2[2];

/**
 *
 * @brief Initialize stacks for the test
 *
 * @return N/A
 *
 */
void stack_test_init(void)
{
	k_stack_init(&stack_1, stack1, 2);
	k_stack_init(&stack_2, stack2, 2);
}


/**
 *
 * @brief Stack test thread
 *
 * @param par1   Ignored parameter.
 * @param par2   Number of test loops.
 * @param par3	 Unused
 *
 * @return N/A
 *
 */
void stack_thread1(void *par1, void *par2, void *par3)
{
	int num_loops = ((int) par2 / 2);
	int i;
	u32_t data;

	ARG_UNUSED(par1);
	ARG_UNUSED(par3);

	for (i = 0; i < num_loops; i++) {
		k_stack_pop(&stack_1, &data, K_FOREVER);
		if (data != 2 * i) {
			break;
		}
		data = 2 * i;
		k_stack_push(&stack_2, data);
		k_stack_pop(&stack_1, &data, K_FOREVER);
		if (data != 2 * i + 1) {
			break;
		}
		data = 2 * i + 1;
		k_stack_push(&stack_2, data);
	}
}


/**
 *
 * @brief Stack test thread
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 * @param par3	 Unused
 *
 * @return N/A
 *
 */
void stack_thread2(void *par1, void *par2, void *par3)
{
	int i;
	u32_t data;
	int *pcounter = (int *)par1;
	int num_loops = (int) par2;

	ARG_UNUSED(par3);

	for (i = 0; i < num_loops; i++) {
		data = i;
		k_stack_push(&stack_1, data);
		k_stack_pop(&stack_2, &data, K_FOREVER);
		if (data != i) {
			break;
		}
		(*pcounter)++;
	}
}


/**
 *
 * @brief Stack test thread
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 * @param par3	 Unused
 *
 * @return N/A
 *
 */
void stack_thread3(void *par1, void *par2, void *par3)
{
	int i;
	u32_t data;
	int *pcounter = (int *)par1;
	int num_loops = (int) par2;

	ARG_UNUSED(par3);

	for (i = 0; i < num_loops; i++) {
		data = i;
		k_stack_push(&stack_1, data);
		data = 0xffffffff;

		while (k_stack_pop(&stack_2, &data,
					     K_NO_WAIT) != 0) {
			k_yield();
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
	u32_t t;
	int i = 0;
	int return_value = 0;

	/* test get wait & put stack functions between co-op threads */
	fprintf(output_file, sz_test_case_fmt,
			"Stack #1");
	fprintf(output_file, sz_description,
			"\n\tk_stack_init"
			"\n\tk_stack_pop(K_FOREVER)"
			"\n\tk_stack_push");
	printf(sz_test_start_fmt);

	stack_test_init();

	t = BENCH_START();

	k_thread_create(&thread_data1, thread_stack1, STACK_SIZE, stack_thread1,
			 0, (void *) number_of_loops, NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);
	k_thread_create(&thread_data2, thread_stack2, STACK_SIZE, stack_thread2,
			 (void *) &i, (void *) number_of_loops, NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	/* test get/yield & put stack functions between co-op threads */
	fprintf(output_file, sz_test_case_fmt,
			"Stack #2");
	fprintf(output_file, sz_description,
			"\n\tk_stack_init"
			"\n\tk_stack_pop(K_FOREVER)"
			"\n\tk_stack_pop"
			"\n\tk_stack_push"
			"\n\tk_yield");
	printf(sz_test_start_fmt);

	stack_test_init();

	t = BENCH_START();

	i = 0;
	k_thread_create(&thread_data1, thread_stack1, STACK_SIZE, stack_thread1,
			 0, (void *) number_of_loops, NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);
	k_thread_create(&thread_data2, thread_stack2, STACK_SIZE, stack_thread3,
			 (void *) &i, (void *) number_of_loops, NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	/* test get wait & put stack functions across co-op and premptive
	 * threads
	 */
	fprintf(output_file, sz_test_case_fmt,
			"Stack #3");
	fprintf(output_file, sz_description,
			"\n\tk_stack_init"
			"\n\tk_stack_pop(K_FOREVER)"
			"\n\tk_stack_push"
			"\n\tk_stack_pop(K_FOREVER)"
			"\n\tk_stack_push");
	printf(sz_test_start_fmt);

	stack_test_init();

	t = BENCH_START();

	k_thread_create(&thread_data1, thread_stack1, STACK_SIZE, stack_thread1,
			 0, (void *) number_of_loops, NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);

	for (i = 0; i < number_of_loops / 2; i++) {
		u32_t data;

		data = 2 * i;
		k_stack_push(&stack_1, data);
		data = 2 * i + 1;
		k_stack_push(&stack_1, data);

		k_stack_pop(&stack_2, &data, K_FOREVER);
		if (data != 2 * i + 1) {
			break;
		}
		k_stack_pop(&stack_2, &data, K_FOREVER);
		if (data != 2 * i) {
			break;
		}
	}

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i * 2, t);

	return return_value;
}
