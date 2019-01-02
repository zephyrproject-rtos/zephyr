/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_pipe_on_single_elements(void);
extern void test_pipe_on_multiple_elements(void);
extern void test_pipe_forever_wait(void);
extern void test_pipe_timeout(void);
extern void test_pipe_get_on_empty_pipe(void);
extern void test_pipe_forever_timeout(void);
extern void test_pipe_get_timeout(void);
extern void test_pipe_get_invalid_size(void);

extern struct k_pipe test_pipe;
extern struct k_sem put_sem, get_sem, sync_sem, multiple_send_sem;
extern struct k_stack stack_1;
extern struct k_thread get_single_tid;

/*test case main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(),
			      &test_pipe, &put_sem, &get_sem,
			      &sync_sem, &multiple_send_sem,
			      &get_single_tid, &stack_1);

	ztest_test_suite(test_pipe,
			 ztest_user_unit_test(test_pipe_on_single_elements),
			 ztest_user_unit_test(test_pipe_on_multiple_elements),
			 ztest_user_unit_test(test_pipe_forever_wait),
			 ztest_user_unit_test(test_pipe_timeout),
			 ztest_user_unit_test(test_pipe_get_on_empty_pipe),
			 ztest_user_unit_test(test_pipe_forever_timeout),
			 ztest_user_unit_test(test_pipe_get_timeout),
			 ztest_user_unit_test(test_pipe_get_invalid_size)
			 );

	ztest_run_test_suite(test_pipe);
}


