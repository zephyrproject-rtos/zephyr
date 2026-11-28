/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <ksched.h>
#include <kswap.h>

extern void z_yield_testing_only(void);

static struct k_thread alt_thread;
K_THREAD_STACK_DEFINE(alt_thread_stack, 1024 + CONFIG_TEST_EXTRA_STACK_SIZE);

static K_SEM_DEFINE(sem, 0, 1);

static void alt_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
}

/**
 * @brief Verify arch_swap() keeps a value stored before the swap
 *
 * A waker can ready the thread between the pend and the swap (native_sim
 * does not exclude interrupts across that window) and store the result of
 * the completed wait. arch_swap() must not overwrite it with -EAGAIN.
 */
ZTEST(arch_posix_swap, test_keeps_waker_value)
{
	int prio = k_thread_priority_get(_current);
	unsigned int key;
	int ret;

	/* Same cooperative priority, so creating the thread does not preempt */
	k_thread_priority_set(_current, K_PRIO_COOP(0));
	k_thread_create(&alt_thread, alt_thread_stack,
			K_THREAD_STACK_SIZEOF(alt_thread_stack),
			alt_thread_entry, NULL, NULL, NULL, K_PRIO_COOP(0), 0,
			K_NO_WAIT);

	key = arch_irq_lock();

	/* Queue the current thread so the swap below switches to the peer */
	z_yield_testing_only();

	/* Stand in for a waker that has already completed the wait */
	arch_thread_return_value_set(_current, 0);

	ret = arch_swap(key);

	k_thread_priority_set(_current, prio);

	zassert_equal(ret, 0, "arch_swap() overwrote the waker's value (%d)", ret);
}

/**
 * @brief Verify a real timeout still reports -EAGAIN
 */
ZTEST(arch_posix_swap, test_timeout_returns_eagain)
{
	zassert_equal(k_sem_take(&sem, K_MSEC(10)), -EAGAIN,
		      "timeout did not return -EAGAIN");

	k_sem_give(&sem);
	zassert_equal(k_sem_take(&sem, K_MSEC(10)), 0,
		      "completed wait did not return 0");
}

ZTEST_SUITE(arch_posix_swap, NULL, NULL, NULL, NULL, NULL);
