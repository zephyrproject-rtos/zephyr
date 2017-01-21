/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

/*macro definition*/

#ifdef CONFIG_RISCV32
#define STACK_SIZE 512
#else
#define STACK_SIZE 256
#endif

/*local variables*/
static char __stack tstack[STACK_SIZE];

static void customdata_entry(void *p1, void *p2, void *p3)
{
	uint32_t data = 1;

	assert_is_null(k_thread_custom_data_get(), NULL);
	while (1) {
		k_thread_custom_data_set((void *)data);
		/* relinguish cpu for a while */
		k_sleep(50);
		/** TESTPOINT: custom data comparison */
		assert_equal(data, (uint32_t)k_thread_custom_data_get(), NULL);
		data++;
	}
}

/* test cases */
/**
 * @ingroup t_customdata_api
 * @brief test thread custom data get/set from coop thread
 */
void test_customdata_get_set_coop(void)
{
	k_tid_t tid = k_thread_spawn(tstack, STACK_SIZE,
		customdata_entry, NULL, NULL, NULL,
		K_PRIO_COOP(1), 0, 0);
	k_sleep(500);

	/* cleanup environment */
	k_thread_abort(tid);
}

/**
 * @ingroup t_customdata_api
 * @brief test thread custom data get/set from preempt thread
 */
void test_customdata_get_set_preempt(void)
{
	/** TESTPOINT: custom data of preempt thread */
	k_tid_t tid = k_thread_spawn(tstack, STACK_SIZE,
		customdata_entry, NULL, NULL, NULL,
		K_PRIO_PREEMPT(0), 0, 0);
	k_sleep(500);

	/* cleanup environment */
	k_thread_abort(tid);
}
