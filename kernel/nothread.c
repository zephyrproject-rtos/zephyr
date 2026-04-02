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

#ifdef CONFIG_SYS_CLOCK_EXISTS
static K_TIMER_DEFINE(sleep_timer, NULL, NULL);
#endif

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

#ifdef CONFIG_SYS_CLOCK_EXISTS
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
#else
	/* Fallback to busy-wait when no system clock is available */
	k_ticks_t ticks;
	uint32_t ticks_to_wait;

	ticks = timeout.ticks;
	if (Z_IS_TIMEOUT_RELATIVE(timeout)) {
		/* ticks is delta timeout */
		ticks_to_wait = ticks;
	} else {
		/* ticks is absolute timeout expiration */
		uint32_t curr_ticks = sys_clock_tick_get_32();

		if (Z_TICK_ABS(ticks) > curr_ticks) {
			ticks_to_wait = Z_TICK_ABS(ticks) - curr_ticks;
		} else {
			ticks_to_wait = 0;
		}
	}

	/* busy wait to be time coherent since subsystems may depend on it */
	z_impl_k_busy_wait(k_ticks_to_us_ceil32(ticks_to_wait));

	int32_t ret = k_ticks_to_ms_ceil64(0);

	SYS_PORT_TRACING_FUNC_EXIT(k_thread, sleep, timeout, ret);

	return ret;
#endif
}
