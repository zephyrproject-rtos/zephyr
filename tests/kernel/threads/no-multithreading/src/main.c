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
	uint32_t watchdog = sys_clock_hw_cycles_per_sec();

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

	k_busy_wait(15000);
	zassert_false(timeout_run, "Timeout should not expire because irq is locked");

	irq_unlock(key);

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

	k_cpu_idle();

	diff = k_uptime_get() - now;
	zassert_true(timeout_run, "Timeout should expire");
	zassert_within(diff, 10, 2, "Unexpected time passed: %d ms", (int)diff);
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
