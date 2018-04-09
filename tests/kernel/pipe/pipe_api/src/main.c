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
#include "test_pipe.h"

static unsigned char __kernel_noinit __aligned(4) pipe_buf[PIPE_LEN];

extern struct k_pipe pipe, kpipe;
extern struct k_sem end_sema;
K_THREAD_STACK_EXTERN(tstack);
extern struct k_thread tdata;

/*test case main entry*/
void test_main(void)
{
	k_pipe_init(&pipe, pipe_buf, PIPE_LEN);
	k_thread_access_grant(k_current_get(),
			      &kpipe, &pipe, &end_sema, &tdata, &tstack,
			      NULL);

	ztest_test_suite(test_pipe_api,
			 ztest_user_unit_test(test_pipe_thread2thread),
			 ztest_unit_test(test_pipe_put_fail),
			 ztest_unit_test(test_pipe_get_fail),
			 ztest_unit_test(test_pipe_block_put),
			 ztest_unit_test(test_pipe_block_put_sema),
			 ztest_unit_test(test_pipe_get_put));
	ztest_run_test_suite(test_pipe_api);
}
