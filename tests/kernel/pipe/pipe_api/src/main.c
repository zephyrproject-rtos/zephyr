/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup kernel_pipe_tests PIPEs
 * @ingroup all_tests
 * @{
 * @}
 */

#include <zephyr/ztest.h>

/* k objects */
extern struct k_pipe pipe, kpipe, khalfpipe, put_get_pipe;
extern struct k_sem end_sema;
extern struct k_stack tstack;
extern struct k_thread tdata;
extern struct k_heap test_pool;

static void *pipe_api_setup(void)
{
	k_thread_access_grant(k_current_get(), &pipe,
			      &kpipe, &end_sema, &tdata, &tstack,
			      &khalfpipe, &put_get_pipe);

	k_thread_heap_assign(k_current_get(), &test_pool);

	return NULL;
}

ZTEST_SUITE(pipe_api, NULL, pipe_api_setup, NULL, NULL, NULL);

ZTEST_SUITE(pipe_api_1cpu, NULL, pipe_api_setup,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
