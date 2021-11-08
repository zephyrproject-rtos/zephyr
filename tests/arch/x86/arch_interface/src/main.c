/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <sys/arch_interface.h>
#include <sys/printk.h>
#include <timing/timing.h>

extern int arch_printk_char_out(int c);

#define EFLAGS_IF_MASK 0X200
/* Get the bit value of IRQ status */
#define CHECK_IF_BIT(x, y) {        \
	x = irq_lock();                 \
	y = (x & EFLAGS_IF_MASK);       \
	irq_unlock(x);                  \
}
static struct k_timer idle_timer;
static void idle_timer_expiry_function(struct k_timer *timer_id)
{
	/* nothing to do */
}

/**
 * @brief Verify arch interface of timing functions
 *
 * @details Initializing arch timing and start it,
 * Get timing counter and halt CPU for 1000us, and
 * then re-get the timing counter, computing the diff,
 * convert to us, record it. Then stop it.
 *
 * @see timing_counter_get()
 * arch_timing_cycles_to_ns()
 */
void test_arch_timing_func(void)
{
	uint64_t cycle_to_us = 0;

	timing_t timestamp_start = 0;
	timing_t timestamp_end = 0;

	timing_init();
	timing_start();

	timestamp_start = timing_counter_get();
	k_busy_wait(1000);
	timestamp_end = timing_counter_get();

	int diff = timing_cycles_get(&timestamp_start,
					&timestamp_end);
	cycle_to_us = arch_timing_cycles_to_ns(diff)/1000;

	TC_PRINT("diff: %u\n", diff);
	TC_PRINT("Cycle time/count:%llu ns\n", cycle_to_us);

	/* stop the timer */
	timing_stop();
}

/**
 * @brief Verify the bit value of IF in EFLAGS register is not changed
 * before and after call CPU idle function.
 *
 * @details Get the value of EFLAGES, and locked the IRQ first, then
 * call k_cpu_atomic_idle to make CPU idle, and wake up the CPU by
 * timer interrupt, then compare the value of IF before and after
 * call CPU idle function.
 *
 * @see k_cpu_atomic_idle()
 */
void test_kernel_cpu_idle_no_impact_if(void)
{
	unsigned int eflags = 0;
	unsigned int if_value1 = 0, if_value2 = 0;

	/* initialized a timer to wake up the CPU */
	k_timer_init(&idle_timer, idle_timer_expiry_function, NULL);
	k_timer_start(&idle_timer, K_MSEC(200), K_MSEC(1));

	/* locked irq first */
	unsigned int key = irq_lock();

	CHECK_IF_BIT(eflags, if_value1);
	zassert_false(if_value1, "The IF is not low");

	k_cpu_atomic_idle(eflags);

	CHECK_IF_BIT(eflags, if_value2);
	zassert_true(if_value1 == if_value2, "The IF changed");

	/* unlock IRQ */
	irq_unlock(key);
	k_timer_stop(&idle_timer);
}

void test_main(void)
{
	ztest_test_suite(arch_interface,
			  ztest_unit_test(test_arch_timing_func),
			  ztest_unit_test(test_kernel_cpu_idle_no_impact_if));
	ztest_run_test_suite(arch_interface);
}
