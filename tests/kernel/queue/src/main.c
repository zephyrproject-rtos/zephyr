/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "test_queue.h"

#ifndef CONFIG_USERSPACE
#define dummy_test(_name)		\
	static void _name(void)		\
	{				\
		ztest_test_skip();	\
	}

dummy_test(test_queue_supv_to_user);
dummy_test(test_queue_alloc_prepend_user);
dummy_test(test_queue_alloc_append_user);
dummy_test(test_auto_free);
dummy_test(test_queue_init_null);
dummy_test(test_queue_alloc_append_null);
dummy_test(test_queue_alloc_prepend_null);
dummy_test(test_queue_get_null);
dummy_test(test_queue_is_empty_null);
dummy_test(test_queue_peek_head_null);
dummy_test(test_queue_peek_tail_null);
dummy_test(test_queue_cancel_wait_error);
#endif

#ifdef CONFIG_64BIT
#define MAX_SZ	128
#else
#define MAX_SZ	96
#endif
K_HEAP_DEFINE(test_pool, MAX_SZ * 4 + 128);

/*test case main entry*/
void test_main(void)
{
	k_thread_heap_assign(k_current_get(), &test_pool);

	ztest_test_suite(queue_api,
			 ztest_1cpu_unit_test(test_queue_supv_to_user),
			 ztest_user_unit_test(test_queue_alloc_prepend_user),
			 ztest_user_unit_test(test_queue_alloc_append_user),
			 ztest_unit_test(test_auto_free),
			 ztest_1cpu_unit_test(test_queue_thread2thread),
			 ztest_unit_test(test_queue_thread2isr),
			 ztest_unit_test(test_queue_isr2thread),
			 ztest_1cpu_unit_test(test_queue_get_2threads),
			 ztest_1cpu_unit_test(test_queue_get_fail),
			 ztest_1cpu_unit_test(test_queue_loop),
			 ztest_unit_test(test_queue_alloc),
			 ztest_1cpu_unit_test(test_queue_poll_race),
			 ztest_unit_test(test_multiple_queues),
			 ztest_unit_test(test_access_kernel_obj_with_priv_data),
			 ztest_unit_test(test_queue_append_list_error),
			 ztest_unit_test(test_queue_merge_list_error),
			 ztest_user_unit_test(test_queue_init_null),
			 ztest_user_unit_test(test_queue_alloc_append_null),
			 ztest_user_unit_test(test_queue_alloc_prepend_null),
			 ztest_user_unit_test(test_queue_get_null),
			 ztest_user_unit_test(test_queue_is_empty_null),
			 ztest_user_unit_test(test_queue_peek_head_null),
			 ztest_user_unit_test(test_queue_peek_tail_null),
			 ztest_user_unit_test(test_queue_cancel_wait_error)
			 );
	ztest_run_test_suite(queue_api);
}
