/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>

ZTEST(no_multithreading, test_k_busy_wait)
{
	int64_t now = k_uptime_get();
	uint32_t watchdog = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

	while (k_uptime_get() != now) {
		/* Wait until uptime progresses */
		watchdog--;
		if (watchdog == 0) {
			zassert_false(true, "No progress in uptime");
		}
	}

	now = k_uptime_get();
	/* Check that k_busy_wait is working as expected. */
	k_busy_wait(10000);

	int64_t diff = k_uptime_get() - now;

	zassert_within(diff, 10, 2);
}

static void timeout_handler(struct k_timer *timer)
{
	bool *flag = k_timer_user_data_get(timer);

	*flag = true;
}

K_TIMER_DEFINE(timer, timeout_handler, NULL);

ZTEST(no_multithreading, test_irq_locking)
{
	volatile bool timeout_run = false;

	k_timer_user_data_set(&timer, (void *)&timeout_run);
	k_timer_start(&timer, K_MSEC(10), K_NO_WAIT);

	unsigned int key = irq_lock();

	/* Wait long enough to cover the 10 ms timeout plus one full
	 * tick of slack for z_add_timeout()'s "at least N" round-up
	 * plus a few ms of measurement margin.
	 */
	k_busy_wait(10000 + k_ticks_to_us_ceil32(1) + 5000);
	zassert_false(timeout_run, "Timeout should not expire because irq is locked");

	irq_unlock(key);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/*
		 * On a tickful kernel, the timer ISR announces exactly one
		 * tick per invocation. While IRQs were masked, multiple tick
		 * interrupts fired but the IRQ controller's pending bit
		 * coalesces them, so only a single ISR runs when IRQs are
		 * unlocked. Our K_MSEC(10) timer needs two announces to
		 * reach its deadline under the "at least N ticks" contract,
		 * so the first pending delivery only brings dticks from 2
		 * down to 1. Wait for the next tick so a second ISR fires
		 * and the timer actually expires.
		 */
		k_busy_wait(k_ticks_to_us_ceil32(1) + 1000);
	}

	zassert_true(timeout_run, "Timeout should expire because irq got unlocked");
}

ZTEST(no_multithreading, test_cpu_idle)
{
	volatile bool timeout_run = false;
	int64_t now, diff;

	k_timer_user_data_set(&timer, (void *)&timeout_run);
	now = k_uptime_get();
	/* Start timer and go to idle, cpu should sleep until it is waken up
	 * by sys clock interrupt.
	 */
	k_timer_start(&timer, K_MSEC(10), K_NO_WAIT);

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/*
		 * On a tickless kernel the only scheduled wakeup while
		 * we are idle is our own timer, so k_cpu_idle() returns
		 * once with the timer callback already executed.
		 */
		k_cpu_idle();
		zassert_true(timeout_run, "Timeout should expire");
	} else {
		/*
		 * On a tickful kernel periodic tick interrupts wake
		 * k_cpu_idle() on every tick regardless of our timer.
		 * Loop back to idle until the timer callback has actually
		 * run, which under the "at least N ticks" contract may
		 * take more than one tick for K_MSEC() values that round
		 * to a single tick.
		 */
		while (!timeout_run) {
			k_cpu_idle();
		}
	}

	diff = k_uptime_get() - now;
	/* Timer fires at least 10 ms from now (minimum delay), at most
	 * 10 ms plus one tick for the round-up in z_add_timeout() and
	 * a couple of ms of measurement margin.
	 */
	zassert_between_inclusive(diff, 10, 10 + k_ticks_to_ms_ceil32(1) + 2,
				  "Unexpected time passed: %d ms", (int)diff);
}

#define IDX_PRE_KERNEL_1 0
#define IDX_PRE_KERNEL_2 1
#define IDX_POST_KERNEL 2

#define SYS_INIT_CREATE(level) \
	static int pre_kernel_##level##_init_func(void) \
	{ \
		if (init_order != IDX_##level && sys_init_result == 0) { \
			sys_init_result = -1; \
			return -EIO; \
		} \
		init_order++; \
		return 0;\
	} \
	SYS_INIT(pre_kernel_##level##_init_func, level, 0)

static int init_order;
static int sys_init_result;

FOR_EACH(SYS_INIT_CREATE, (;), PRE_KERNEL_1, PRE_KERNEL_2, POST_KERNEL);

ZTEST(no_multithreading, test_sys_init)
{
	zassert_equal(init_order, 3, "SYS_INIT failed: %d", init_order);
}

ZTEST_SUITE(no_multithreading, NULL, NULL, NULL, NULL, NULL);
