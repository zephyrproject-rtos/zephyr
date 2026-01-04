/*
 * Copyright (C) 2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <kernel_internal.h>

/* We are not building thread.c when MULTITHREADING=n, so we
 * need to provide a few stubs here.
 */
bool k_is_in_isr(void)
{
	return arch_is_in_isr();
}

static volatile bool k_sleep_timedout;

static void k_sleep_nothread_timeout(struct k_timer *timer)
{
	k_sleep_timedout = true;
}

K_TIMER_DEFINE(k_sleep_nothread, k_sleep_nothread_timeout, NULL);

/* This is a fallback implementation of k_sleep() for when multi-threading is
 * disabled. The main implementation is in sched.c.
 */
int32_t z_impl_k_sleep(k_timeout_t timeout)
{
	__ASSERT(!arch_is_in_isr(), "");
	__ASSERT_NO_MSG(!k_sleep_timedout);

	unsigned int key;

	SYS_PORT_TRACING_FUNC_ENTER(k_thread, sleep, timeout);

	/* in case of K_FOREVER, we suspend */
	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		/* In Single Thread, just wait for an interrupt saving power */
		k_cpu_idle();
		SYS_PORT_TRACING_FUNC_EXIT(k_thread, sleep, timeout, (int32_t) K_TICKS_FOREVER);

		return (int32_t) K_TICKS_FOREVER;
	}

	k_sleep_timedout = false;
	k_timer_start(&k_sleep_nothread, timeout, K_NO_WAIT);

	/* Without locking interrupts, a race condition can occur if the k_timer interrupt
	 * is raised after checking k_sleep_timedout but before entering k_cpu_idle().
	 * Therefore, lock interrupts before checking whether the timeout has expired,
	 * and if it hasn't, use k_cpu_atomic_idle() to re-enable IRQs and idle
	 * the CPU atomically.
	 */

	for (;;) {
		key = irq_lock();
		if (k_sleep_timedout) {
			irq_unlock(key);
			break;
		} else {
			k_cpu_atomic_idle(key);
		}
	}

	SYS_PORT_TRACING_FUNC_EXIT(k_thread, sleep, timeout, ret);

	/* This implementation is compiled when multithreading is disabled.
	 * Therefore, unlike its multithreaded counterpart, it shall always
	 * sleep for the entire timeout duration (no thread to awake).
	 */
	return 0;
}
