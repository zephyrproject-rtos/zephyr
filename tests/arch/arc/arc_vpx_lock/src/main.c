/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/arch/arc/v2/vpx/arc_vpx.h>

#ifndef CONFIG_ARC_VPX_COOPERATIVE_SHARING
#error "Rebuild with the ARC_VPX_COOPERATIVE_SHARING config option enabled"
#endif

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

static void timer_func(struct k_timer *timer);

K_THREAD_STACK_DEFINE(payload_stack, STACK_SIZE);

static K_TIMER_DEFINE(my_timer, timer_func, NULL);

static struct k_thread payload_thread;

static volatile int isr_result;
static volatile unsigned int isr_vpx_lock_id;

/**
 * Obtain the current CPU id.
 */
static int current_cpu_id_get(void)
{
	int key;
	int id;

	key = arch_irq_lock();
	id = _current_cpu->id;
	arch_irq_unlock(key);

	return id;
}

static void timer_func(struct k_timer *timer)
{
	arc_vpx_unlock_force(isr_vpx_lock_id);
}

static void arc_vpx_lock_unlock_timed_payload(void *p1, void *p2, void *p3)
{
	int status;
	unsigned int cpu_id;
	unsigned int lock_id;

	cpu_id = (unsigned int)(uintptr_t)(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	status = arc_vpx_lock(K_NO_WAIT);
	zassert_equal(0, status, "Expected return value %d, not %d\n", 0, status);

	/*
	 * In 1 second, forcibly release the VPX lock. However, wait up to
	 * 5 seconds before considering this a failure.
	 */

	isr_vpx_lock_id = cpu_id;
	k_timer_start(&my_timer, K_MSEC(1000), K_FOREVER);

	status = arc_vpx_lock(K_MSEC(5000));
	zassert_equal(0, status, "Expected return value %d, not %d\n", 0, status);

	arc_vpx_unlock();
}

ZTEST(vpx_lock, test_arc_vpx_lock_unlock_timed)
{
	int priority;
	int cpu_id;

	priority = k_thread_priority_get(k_current_get());
	cpu_id = current_cpu_id_get();

	k_thread_create(&payload_thread, payload_stack, STACK_SIZE,
			arc_vpx_lock_unlock_timed_payload,
			(void *)(uintptr_t)cpu_id, NULL, NULL,
			priority - 2, 0, K_FOREVER);

#if defined(CONFIG_SCHED_CPU_MASK) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	k_thread_cpu_pin(&payload_thread, cpu_id);
#endif
	k_thread_start(&payload_thread);

	k_thread_join(&payload_thread, K_FOREVER);
}

static void arc_vpx_lock_unlock_payload(void *p1, void *p2, void *p3)
{
	int status;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* The VPX lock is available; take it. */

	status = arc_vpx_lock(K_NO_WAIT);
	zassert_equal(0, status, "Expected return value %d, not %d\n", 0, status);

	/* The VPX lock has already been taken; expect errors */

	status = arc_vpx_lock(K_NO_WAIT);
	zassert_equal(-EBUSY, status, "Expected return value %d (-EBUSY), not %d\n",
		      -EBUSY, status);

	status = arc_vpx_lock(K_MSEC(10));
	zassert_equal(-EAGAIN, status, "Expected return value %d (-EAGAIN), not %d\n",
		      -EAGAIN, status);

	/* Verify that unlocking makes it available */

	arc_vpx_unlock();

	status = arc_vpx_lock(K_NO_WAIT);
	zassert_equal(0, status, "Expected return value %d, not %d\n", 0, status);
	arc_vpx_unlock();
}

ZTEST(vpx_lock, test_arc_vpx_lock_unlock)
{
	int priority;
	int cpu_id;

	priority = k_thread_priority_get(k_current_get());
	cpu_id = current_cpu_id_get();

	k_thread_create(&payload_thread, payload_stack, STACK_SIZE,
			arc_vpx_lock_unlock_payload, NULL, NULL, NULL,
			priority - 2, 0, K_FOREVER);

#if defined(CONFIG_SCHED_CPU_MASK) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	k_thread_cpu_pin(&payload_thread, cpu_id);
#endif
	k_thread_start(&payload_thread);

	k_thread_join(&payload_thread, K_FOREVER);
}

ZTEST_SUITE(vpx_lock, NULL, NULL, NULL, NULL, NULL);
