/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * This file contains the main testing module that invokes all the tests.
 */

#include <timestamp.h>
#include "utils.h"
#include <tc_util.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACKSIZE)

u32_t tm_off;        /* time necessary to read the time */
int error_count;        /* track number of errors */


extern void thread_switch_yield(void);
extern void int_to_thread(void);
extern void int_to_thread_evt(void);
extern void sema_lock_unlock(void);
extern void mutex_lock_unlock(void);
extern int coop_ctx_switch(void);
void test_thread(void *arg1, void *arg2, void *arg3)
{
	PRINT_BANNER();
	PRINT_TIME_BANNER();

	bench_test_init();

	int_to_thread();
	print_dash_line();

	int_to_thread_evt();
	print_dash_line();

	sema_lock_unlock();
	print_dash_line();

	mutex_lock_unlock();
	print_dash_line();

	thread_switch_yield();
	print_dash_line();

	coop_ctx_switch();
	print_dash_line();

	TC_END_REPORT(error_count);
}

K_THREAD_DEFINE(tt_id, STACK_SIZE,
		test_thread, NULL, NULL, NULL,
		10, 0, K_NO_WAIT);


void main(void)
{
}
