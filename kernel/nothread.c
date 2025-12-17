/*
 * Copyright (C) 2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>

/* We are not building thread.c when MULTITHREADING=n, so we
 * need to provide a few stubs here.
 */
bool k_is_in_isr(void)
{
	return arch_is_in_isr();
}

static K_TIMER_DEFINE(sleep_timer, NULL, NULL);

/* This is a fallback implementation of k_sleep() for when multi-threading is
 * disabled. The main implementation is in sched.c.
 */
int32_t z_impl_k_sleep(k_timeout_t timeout)
{
	__ASSERT(!arch_is_in_isr(), "");

	SYS_PORT_TRACING_FUNC_ENTER(k_thread, sleep, timeout);

	/* in case of K_FOREVER, we suspend */
	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		/* In Single Thread, just wait for an interrupt saving power */
		k_cpu_idle();
		SYS_PORT_TRACING_FUNC_EXIT(k_thread, sleep, timeout, (int32_t) K_TICKS_FOREVER);

		return (int32_t) K_TICKS_FOREVER;
	}

	k_timer_start(&sleep_timer, timeout, K_NO_WAIT);

	/* When multithreading is disabled, this will enter a low power state
	 * using k_cpu_idle() until the timer has expired.
	 */
	k_timer_status_sync(&sleep_timer);

	SYS_PORT_TRACING_FUNC_EXIT(k_thread, sleep, timeout, 0);

	/* This implementation is compiled when multithreading is disabled.
	 * Therefore, unlike its multithreaded counterpart, it shall always
	 * sleep for the entire timeout duration (no thread to awake).
	 */
	return 0;
}
