/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup kernel_message_queue_tests Message Queue
 * @ingroup all_tests
 * @{
 * @}
 */

#include <ztest.h>

#include "test_msgq.h"

extern void test_msgq_thread(void);
extern void test_msgq_thread_overflow(void);
extern void test_msgq_isr(void);
extern void test_msgq_put_fail(void);
extern void test_msgq_get_fail(void);
extern void test_msgq_purge_when_put(void);
extern void test_msgq_attrs_get(void);
extern void test_msgq_alloc(void);
extern void test_msgq_pend_thread(void);
extern void test_msgq_empty(void);
extern void test_msgq_full(void);
#ifdef CONFIG_USERSPACE
extern void test_msgq_user_thread(void);
extern void test_msgq_user_thread_overflow(void);
extern void test_msgq_user_put_fail(void);
extern void test_msgq_user_get_fail(void);
extern void test_msgq_user_attrs_get(void);
extern void test_msgq_user_purge_when_put(void);
#else
#define dummy_test(_name) \
	static void _name(void) \
	{ \
		ztest_test_skip(); \
	}

dummy_test(test_msgq_user_thread);
dummy_test(test_msgq_user_thread_overflow);
dummy_test(test_msgq_user_put_fail);
dummy_test(test_msgq_user_get_fail);
dummy_test(test_msgq_user_attrs_get);
dummy_test(test_msgq_user_purge_when_put);
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_64BIT
#define MAX_SZ	256
#else
#define MAX_SZ	128
#endif

K_HEAP_DEFINE(test_pool, MAX_SZ * 2);

extern struct k_msgq kmsgq;
extern struct k_msgq msgq;
extern struct k_sem end_sema;
extern struct k_thread tdata;
K_THREAD_STACK_DECLARE(tstack, STACK_SIZE);

/*test case main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(), &kmsgq, &msgq, &end_sema,
			      &tdata, &tstack);

	k_thread_heap_assign(k_current_get(), &test_pool);

	ztest_test_suite(msgq_api,
			 ztest_1cpu_unit_test(test_msgq_thread),
			 ztest_unit_test(test_msgq_thread_overflow),
			 ztest_user_unit_test(test_msgq_user_thread),
			 ztest_user_unit_test(test_msgq_user_thread_overflow),
			 ztest_unit_test(test_msgq_isr),
			 ztest_1cpu_unit_test(test_msgq_put_fail),
			 ztest_1cpu_unit_test(test_msgq_get_fail),
			 ztest_user_unit_test(test_msgq_user_put_fail),
			 ztest_user_unit_test(test_msgq_user_get_fail),
			 ztest_unit_test(test_msgq_attrs_get),
			 ztest_user_unit_test(test_msgq_user_attrs_get),
			 ztest_1cpu_unit_test(test_msgq_purge_when_put),
			 ztest_user_unit_test(test_msgq_user_purge_when_put),
			 ztest_1cpu_unit_test(test_msgq_pend_thread),
			 ztest_1cpu_unit_test(test_msgq_empty),
			 ztest_1cpu_unit_test(test_msgq_full),
			 ztest_unit_test(test_msgq_alloc));
	ztest_run_test_suite(msgq_api);
}
