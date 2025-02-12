/* fifo.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "syskernel.h"

struct k_fifo fifo1;
struct k_fifo fifo2;

static struct k_fifo sync_fifo; /* for synchronization */


/**
 *
 * @brief Initialize FIFOs for the test
 *
 */
void fifo_test_init(void)
{
	k_fifo_init(&fifo1);
	k_fifo_init(&fifo2);
}


/**
 *
 * @brief Fifo test thread
 *
 * @param par1   Ignored parameter.
 * @param par2   Number of test loops.
 * @param par3	 unused
 *
 */
void fifo_thread1(void *par1, void *par2, void *par3)
{
	int i;
	intptr_t element[2];
	intptr_t *pelement;
	int num_loops = POINTER_TO_INT(par2);

	ARG_UNUSED(par1);
	ARG_UNUSED(par3);
	for (i = 0; i < num_loops; i++) {
		pelement = k_fifo_get(&fifo1, K_FOREVER);
		if (pelement[1] != i) {
			break;
		}
		element[1] = i;
		k_fifo_put(&fifo2, element);
	}
	/* wait till it is safe to end: */
	k_fifo_get(&sync_fifo, K_FOREVER);
}


/**
 *
 * @brief Fifo test thread
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 * @param par3   unused
 *
 */
void fifo_thread2(void *par1, void *par2, void *par3)
{
	int i;
	intptr_t element[2];
	intptr_t *pelement;
	int *pcounter = par1;
	int num_loops = POINTER_TO_INT(par2);

	ARG_UNUSED(par3);

	for (i = 0; i < num_loops; i++) {
		element[1] = i;
		k_fifo_put(&fifo1, element);
		pelement = k_fifo_get(&fifo2, K_FOREVER);
		if (pelement[1] != i) {
			break;
		}
		(*pcounter)++;
	}
	/* wait till it is safe to end: */
	k_fifo_get(&sync_fifo, K_FOREVER);
}


/**
 *
 * @brief Fifo test thread
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 * @param par3   unused
 *
 */
void fifo_thread3(void *par1, void *par2, void *par3)
{
	int i;
	intptr_t element[2];
	intptr_t *pelement;
	int *pcounter = par1;
	int num_loops = POINTER_TO_INT(par2);

	ARG_UNUSED(par3);

	for (i = 0; i < num_loops; i++) {
		element[1] = i;
		k_fifo_put(&fifo1, element);
		while ((pelement = k_fifo_get(&fifo2, K_NO_WAIT)) == NULL) {
			k_yield();
		}
		if (pelement[1] != i) {
			break;
		}
		(*pcounter)++;
	}
	/* wait till it is safe to end: */
	k_fifo_get(&sync_fifo, K_FOREVER);
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
	intptr_t element[2];
	int j;

	k_fifo_init(&sync_fifo);

	/* test get wait & put thread functions between co-op threads */
	fprintf(output_file, sz_test_case_fmt,
			"FIFO #1");
	fprintf(output_file, sz_description,
			"\n\tk_fifo_init"
			"\n\tk_fifo_get(K_FOREVER)"
			"\n\tk_fifo_put");
	printf(sz_test_start_fmt);

	fifo_test_init();

	t = BENCH_START();

	k_thread_create(&thread_data1, thread_stack1, STACK_SIZE, fifo_thread1,
			 NULL, INT_TO_POINTER(number_of_loops), NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);
	k_thread_create(&thread_data2, thread_stack2, STACK_SIZE, fifo_thread2,
			 &i, INT_TO_POINTER(number_of_loops), NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	/* threads have done their job, they can stop now safely: */
	for (j = 0; j < 2; j++) {
		k_fifo_put(&sync_fifo, element);
	}

	/* test get/yield & put thread functions between co-op threads */
	fprintf(output_file, sz_test_case_fmt,
			"FIFO #2");
	fprintf(output_file, sz_description,
			"\n\tk_fifo_init"
			"\n\tk_fifo_get(K_FOREVER)"
			"\n\tk_fifo_get(K_NO_WAIT)"
			"\n\tk_fifo_put"
			"\n\tk_yield");
	printf(sz_test_start_fmt);

	fifo_test_init();

	t = BENCH_START();

	i = 0;
	k_thread_create(&thread_data1, thread_stack1, STACK_SIZE, fifo_thread1,
			 NULL, INT_TO_POINTER(number_of_loops), NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);
	k_thread_create(&thread_data2, thread_stack2, STACK_SIZE, fifo_thread3,
			 &i, INT_TO_POINTER(number_of_loops), NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	/* threads have done their job, they can stop now safely: */
	for (j = 0; j < 2; j++) {
		k_fifo_put(&sync_fifo, element);
	}

	/* test get wait & put functions between co-op and preemptive threads */
	fprintf(output_file, sz_test_case_fmt,
			"FIFO #3");
	fprintf(output_file, sz_description,
			"\n\tk_fifo_init"
			"\n\tk_fifo_get(K_FOREVER)"
			"\n\tk_fifo_put"
			"\n\tk_fifo_get(K_FOREVER)"
			"\n\tk_fifo_put");
	printf(sz_test_start_fmt);

	fifo_test_init();

	t = BENCH_START();

	k_thread_create(&thread_data1, thread_stack1, STACK_SIZE, fifo_thread1,
			 NULL, INT_TO_POINTER(number_of_loops / 2U), NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);
	k_thread_create(&thread_data2, thread_stack2, STACK_SIZE, fifo_thread1,
			 NULL, INT_TO_POINTER(number_of_loops / 2U), NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);
	for (i = 0; i < number_of_loops / 2U; i++) {
		intptr_t more_element[2];
		intptr_t *pelement;

		more_element[1] = i;
		k_fifo_put(&fifo1, more_element);
		more_element[1] = i;
		k_fifo_put(&fifo1, more_element);

		pelement = k_fifo_get(&fifo2, K_FOREVER);
		if (pelement[1] != i) {
			break;
		}
		pelement = k_fifo_get(&fifo2, K_FOREVER);
		if (pelement[1] != i) {
			break;
		}
	}
	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i * 2, t);

	/* threads have done their job, they can stop now safely: */
	for (j = 0; j < 2; j++) {
		k_fifo_put(&sync_fifo, element);
	}

	return return_value;
}
