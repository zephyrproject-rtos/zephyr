/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_pipe
 * @{
 * @defgroup t_pipe_api test_pipe_api
 * @}
 */

#include <ztest.h>
extern void test_pipe_thread2thread(void);

extern void test_pipe_put_fail(void);
extern void test_pipe_get_fail(void);
extern void test_pipe_block_put(void);
extern void test_pipe_block_put_sema(void);
extern void test_pipe_get_put(void);
#ifdef CONFIG_USERSPACE
extern void test_pipe_user_thread2thread(void);
extern void test_pipe_user_put_fail(void);
extern void test_pipe_user_get_fail(void);
extern void test_resource_pool_auto_free(void);
#endif

/* k objects */
extern struct k_pipe pipe, kpipe, put_get_pipe;
extern struct k_sem end_sema;
extern struct k_stack tstack;
extern struct k_thread tdata;
extern struct k_mem_pool test_pool;

#ifndef CONFIG_USERSPACE
#define dummy_test(_name) \
	static void _name(void) \
	{ \
		ztest_test_skip(); \
	}

dummy_test(test_pipe_user_thread2thread);
dummy_test(test_pipe_user_put_fail);
dummy_test(test_pipe_user_get_fail);
dummy_test(test_resource_pool_auto_free);
#endif /* !CONFIG_USERSPACE */

/*test case main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(), &pipe,
			      &kpipe, &end_sema, &tdata, &tstack,
			      &put_get_pipe, NULL);

	k_thread_resource_pool_assign(k_current_get(), &test_pool);

	ztest_test_suite(pipe_api,
			 ztest_unit_test(test_pipe_thread2thread),
			 ztest_user_unit_test(test_pipe_user_thread2thread),
			 ztest_user_unit_test(test_pipe_user_put_fail),
			 ztest_user_unit_test(test_pipe_user_get_fail),
			 ztest_unit_test(test_resource_pool_auto_free),
			 ztest_unit_test(test_pipe_put_fail),
			 ztest_unit_test(test_pipe_get_fail),
			 ztest_unit_test(test_pipe_block_put),
			 ztest_unit_test(test_pipe_block_put_sema),
			 ztest_unit_test(test_pipe_get_put));
	ztest_run_test_suite(pipe_api);
}
