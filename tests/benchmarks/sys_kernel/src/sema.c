/* sema.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "syskernel.h"

struct k_sem sem1;
struct k_sem sem2;

/**
 *
 * @brief Initialize semaphores for the test
 *
 * @return N/A
 */
void sema_test_init(void)
{
	k_sem_init(&sem1, 0, 1);
	k_sem_init(&sem2, 0, 1);
}


/**
 *
 * @brief Semaphore test thread
 *
 * @param par1   Ignored parameter.
 * @param par2   Number of test loops.
 * @param par3   Unused
 *
 * @return N/A
 */
void sema_thread1(void *par1, void *par2, void *par3)
{
	int i;
	int num_loops = (int) par2;

	ARG_UNUSED(par1);
	ARG_UNUSED(par3);

	for (i = 0; i < num_loops; i++) {
		k_sem_take(&sem1, K_FOREVER);
		k_sem_give(&sem2);
	}
}


/**
 *
 * @brief Semaphore test thread
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 * @param par3   Unused
 *
 * @return N/A
 */
void sema_thread2(void *par1, void *par2, void *par3)
{
	int i;
	int *pcounter = (int *)par1;
	int num_loops = (int) par2;

	ARG_UNUSED(par3);

	for (i = 0; i < num_loops; i++) {
		k_sem_give(&sem1);
		k_sem_take(&sem2, K_FOREVER);
		(*pcounter)++;
	}
}

/**
 *
 * @brief Semaphore test thread
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 * @param par3   Unused
 *
 * @return N/A
 */
void sema_thread3(void *par1, void *par2, void *par3)
{
	int i;
	int *pcounter = (int *)par1;
	int num_loops = (int) par2;

	ARG_UNUSED(par3);

	for (i = 0; i < num_loops; i++) {
		k_sem_give(&sem1);
		while (k_sem_take(&sem2, K_NO_WAIT) != 0) {
			k_yield();
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
	u32_t t;
	int i = 0;
	int return_value = 0;

	fprintf(output_file, sz_test_case_fmt,
			"Semaphore #1");
	fprintf(output_file, sz_description,
			"\n\tk_sem_init"
			"\n\tk_sem_take(K_FOREVER)"
			"\n\tk_sem_give");
	printf(sz_test_start_fmt);

	sema_test_init();

	t = BENCH_START();

	k_thread_create(&thread_data1, thread_stack1, STACK_SIZE, sema_thread1,
			 NULL, (void *) number_of_loops, NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);
	k_thread_create(&thread_data2, thread_stack2, STACK_SIZE, sema_thread2,
			 (void *) &i, (void *) number_of_loops, NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	fprintf(output_file, sz_test_case_fmt,
			"Semaphore #2");
	fprintf(output_file, sz_description,
			"\n\tk_sem_init"
			"\n\tk_sem_take(TICKS_NONE)"
			"\n\tk_yield"
			"\n\tk_sem_give");
	printf(sz_test_start_fmt);

	sema_test_init();
	i = 0;

	t = BENCH_START();

	k_thread_create(&thread_data1, thread_stack1, STACK_SIZE, sema_thread1,
			 NULL, (void *) number_of_loops, NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);
	k_thread_create(&thread_data2, thread_stack2, STACK_SIZE, sema_thread3,
			 (void *) &i, (void *) number_of_loops, NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	fprintf(output_file, sz_test_case_fmt,
			"Semaphore #3");
	fprintf(output_file, sz_description,
			"\n\tk_sem_init"
			"\n\tk_sem_take(K_FOREVER)"
			"\n\tk_sem_give"
			"\n\tk_sem_give"
			"\n\tk_sem_take(K_FOREVER)");
	printf(sz_test_start_fmt);

	sema_test_init();

	t = BENCH_START();

	k_thread_create(&thread_data1, thread_stack1, STACK_SIZE, sema_thread1,
			 NULL, (void *) number_of_loops, NULL,
			 K_PRIO_COOP(3), 0, K_NO_WAIT);
	for (i = 0; i < number_of_loops; i++) {
		k_sem_give(&sem1);
		k_sem_take(&sem2, K_FOREVER);
	}

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	return return_value;
}
