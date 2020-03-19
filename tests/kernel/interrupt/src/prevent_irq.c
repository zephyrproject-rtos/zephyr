/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "interrupt_util.h"

#define DURATION	5

static struct k_timer irqlock_timer;

volatile u32_t check_lock_new;
u32_t check_lock_old = 0xBEEF;

static void timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	check_lock_new = 0xBEEF;
	printk("timer fired\n");
}

void test_prevent_interruption(void)
{
	int key;

	printk("locking interrupts\n");
	key = irq_lock();

	check_lock_new = 0;

	k_timer_init(&irqlock_timer, timer_handler, NULL);

	/* Start the timer and busy-wait for a bit with IRQs locked. The
	 * timer ought to have fired during this time if interrupts weren't
	 * locked -- but since they are, check_lock_new isn't updated.
	 */
	k_timer_start(&irqlock_timer, DURATION, K_NO_WAIT);
	k_busy_wait(MS_TO_US(1000));
	zassert_not_equal(check_lock_new, check_lock_old,
		"Interrupt locking didn't work properly");

	printk("unlocking interrupts\n");
	irq_unlock(key);

	k_busy_wait(MS_TO_US(1000));

	zassert_equal(check_lock_new, check_lock_old,
		"timer should have fired");

	k_timer_stop(&irqlock_timer);
}
