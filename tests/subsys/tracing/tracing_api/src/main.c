/*
 * Copyright (c) 2021 Intel Corporation+
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include <tracing_buffer.h>
#include <tracing_core.h>
#include <tracing/tracing_format.h>
#include <tracing_cpu_stats.h>
#if defined(CONFIG_TRACING_BACKEND_UART) || defined(CONFIG_TRACING_BACKEND_USB)
#include "../../../../subsys/tracing/include/tracing_backend.h"
#endif

/* size of stack area used by each thread */
#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACKSIZE)
#define LOOP_TIMES 100

static struct k_thread thread;
K_THREAD_STACK_DEFINE(thread_stack, STACK_SIZE);

/* define 2 semaphores and a mutex */
struct k_sem thread1_sem;
struct k_sem thread2_sem;
struct k_mutex mutex;

/* thread handle for switch */
void thread_handle(void *p1, void *self_sem, void *other_sem)
{
	for (int i = 0; i < LOOP_TIMES; i++) {
		/* take my semaphore */
		k_sem_take(self_sem, K_FOREVER);
		k_sem_give(other_sem);

		k_mutex_lock(&mutex, K_FOREVER);
		k_mutex_unlock(&mutex);

		/* wait for a while, then let other thread have a turn. */
		k_sleep(K_MSEC(10));
	}
}

void test_tracing_sys_api(void)
{
	int old_prio = k_thread_priority_get(k_current_get());
	int new_prio = 10;

#if defined(CONFIG_TRACING_CPU_STATS)
	struct cpu_stats cpu_stats_ns;

	cpu_stats_reset_counters();
#endif

	k_sem_init(&thread1_sem, 1, 1);
	k_sem_init(&thread2_sem, 0, 1);
	k_mutex_init(&mutex);

	k_thread_priority_set(k_current_get(), new_prio);

	k_thread_create(&thread, thread_stack, STACK_SIZE,
		thread_handle, NULL, &thread2_sem, &thread1_sem,
		K_PRIO_PREEMPT(0), K_INHERIT_PERMS,
		K_NO_WAIT);

	thread_handle(NULL, &thread1_sem, &thread2_sem);

	/* Waitting for enough data generated */
	k_thread_join(&thread, K_FOREVER);

#if defined(CONFIG_TRACING_CPU_STATS)
	cpu_stats_get_ns(&cpu_stats_ns);
	printk("idle time(ns): %lld\n", cpu_stats_ns.idle);
	printk("non_idle time(ns): %lld\n", cpu_stats_ns.non_idle);
	printk("sched time(ns): %lld\n", cpu_stats_ns.sched);
#endif

	k_thread_priority_set(k_current_get(), old_prio);
}

void test_tracing_cmd_manual(void)
{
	uint32_t length = 0;
	uint8_t *cmd = NULL;
	uint8_t cmd0[] = " ";
	uint8_t cmd1[] = "enable";
	uint8_t cmd2[] = "disable";

	length = tracing_cmd_buffer_alloc(&cmd);
	cmd = cmd0;
	zassert_true(sizeof(cmd0) < length, "cmd0 is too long");
	tracing_cmd_handle(cmd, sizeof(cmd0));
	zassert_true(is_tracing_enabled(),
		"Failed to check default status of tracing");

	length = tracing_cmd_buffer_alloc(&cmd);
	cmd = cmd1;
	zassert_true(sizeof(cmd1) < length, "cmd1 is too long");
	tracing_cmd_handle(cmd, sizeof(cmd1));
	zassert_true(is_tracing_enabled(), "Failed to enable tracing");

	length = tracing_cmd_buffer_alloc(&cmd);
	cmd = cmd2;
	zassert_true(sizeof(cmd2) < length, "cmd2 is too long");
	tracing_cmd_handle(cmd, sizeof(cmd2));
	zassert_false(is_tracing_enabled(), "Failed to disable tracing");
}


void test_main(void)
{
	ztest_test_suite(test_tracing,
			 ztest_unit_test(test_tracing_sys_api),
			 ztest_unit_test(test_tracing_cmd_manual)
			 );
	ztest_run_test_suite(test_tracing);
}
