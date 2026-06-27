/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/*
 * Counters bumped by the out-of-tree format header
 * (format/custom_trace_format.h), selected via CONFIG_TRACING_FORMAT_HEADER.
 */
volatile unsigned int custom_trace_sem_give_count;
volatile unsigned int custom_trace_thread_create_count;

K_SEM_DEFINE(test_sem, 0, 1);

/*
 * Verify that a tracing format supplied purely through
 * CONFIG_TRACING_FORMAT_HEADER (no edits to the Zephyr tree) is actually wired
 * into the kernel hook points: exercising k_sem_give() must drive the format's
 * sys_port_trace_k_sem_give_enter() hook.
 */
ZTEST(tracing_format_module, test_format_hook_fires)
{
	unsigned int before = custom_trace_sem_give_count;

	k_sem_give(&test_sem);

	zassert_true(custom_trace_sem_give_count > before,
		     "out-of-tree format hook was not invoked on k_sem_give()");
}

ZTEST_SUITE(tracing_format_module, NULL, NULL, NULL, NULL, NULL);
