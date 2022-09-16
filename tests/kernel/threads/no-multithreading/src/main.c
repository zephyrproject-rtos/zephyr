/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <ztest.h>

void test_k_busy_wait(void)
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

	zassert_within(diff, 10, 2, NULL);
}

static void timeout_handler(struct k_timer *timer)
{
	bool *flag = k_timer_user_data_get(timer);

	*flag = true;
}

K_TIMER_DEFINE(timer, timeout_handler, NULL);

void test_irq_locking(void)
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

void test_cpu_idle(void)
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

#define SYS_INIT_CREATE(level) \
	static int pre_kernel_##level##_init_func(const struct device *dev) \
	{ \
		ARG_UNUSED(dev); \
		if (init_order != _SYS_INIT_LEVEL_##level && sys_init_result == 0) { \
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

void test_sys_init(void)
{
	zassert_equal(init_order, 3, "SYS_INIT failed");
}

void test_main(void)
{
	ztest_test_suite(no_multithreading,
			 ztest_unit_test(test_k_busy_wait),
			 ztest_unit_test(test_irq_locking),
			 ztest_unit_test(test_cpu_idle),
			 ztest_unit_test(test_sys_init)
			 );

	ztest_run_test_suite(no_multithreading);
}
