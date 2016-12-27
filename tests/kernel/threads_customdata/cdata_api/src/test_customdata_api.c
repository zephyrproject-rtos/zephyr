/*
 * Copyright (c) 2016 Intel Corporation
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

#include <ztest.h>

/*macro definition*/
#define STACK_SIZE 256

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
