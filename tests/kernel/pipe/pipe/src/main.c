/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern struct k_pipe test_pipe;
extern struct k_pipe small_pipe;
extern struct k_sem put_sem, get_sem, sync_sem, multiple_send_sem;
extern struct k_stack stack_1;
extern struct k_thread get_single_tid;

static void *pipe_setup(void)
{
	k_thread_access_grant(k_current_get(),
			      &test_pipe, &small_pipe, &put_sem, &get_sem,
			      &sync_sem, &multiple_send_sem,
			      &get_single_tid, &stack_1);

	return NULL;
}

ZTEST_SUITE(pipe, NULL, pipe_setup, NULL, NULL, NULL);
