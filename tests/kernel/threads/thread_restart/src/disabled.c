/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#define STACK_SIZE (2048 + CONFIG_TEST_EXTRA_STACK_SIZE)

static struct k_thread *volatile expected_thread;
static atomic_t runs;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	if ((reason != K_ERR_KERNEL_OOPS) || (k_current_get() != expected_thread)) {
		printk("Unexpected fatal error %u in %p\n", reason, k_current_get());
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}
}

static void oops_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	atomic_inc(&runs);
	k_oops();
}

ZTEST(thread_restart_disabled, test_restartable_thread_aborted)
{
	static K_THREAD_STACK_DEFINE(stack, STACK_SIZE);
	static struct k_thread thread;
	k_tid_t tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), oops_entry,
				      NULL, NULL, NULL, K_PRIO_PREEMPT(1), K_RESTARTABLE,
				      K_FOREVER);

	expected_thread = tid;
	k_thread_start(tid);

	zassert_ok(k_thread_join(tid, K_SECONDS(10)));
	zassert_equal(atomic_get(&runs), 1);
}

ZTEST_SUITE(thread_restart_disabled, NULL, NULL, NULL, NULL, NULL);
