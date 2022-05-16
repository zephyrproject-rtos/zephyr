/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <interrupt_util.h>

#define DURATION	5
#define HANDLER_TOKEN	0xDEADBEEF

/* Long enough to be guaranteed a tick "should have fired" */
#define TIMER_DELAY_US (128 * 1000000 / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

static struct k_timer irqlock_timer;
volatile uint32_t handler_result;

static void timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	handler_result = HANDLER_TOKEN;
	printk("timer fired\n");
}

/**
 * @brief Test interrupt prevention
 *
 * @ingroup kernel_interrupt_tests
 *
 * This routine tests if the kernel is capable of preventing interruption, by
 * locking interrupts and busy-waiting to see if the system timer interrupt is
 * serviced while interrupts are locked; in addition, this test also verifies
 * that the system timer interrupt is serviced after interrupts are unlocked.
 */
void test_prevent_interruption(void)
{
	unsigned int key;

	printk("locking interrupts\n");
	key = irq_lock();

	handler_result = 0;

	k_timer_init(&irqlock_timer, timer_handler, NULL);

	/* Start the timer and busy-wait for a bit with IRQs locked. The
	 * timer ought to have fired during this time if interrupts weren't
	 * locked -- but since they are, check_lock_new isn't updated.
	 */
	k_timer_start(&irqlock_timer, K_MSEC(DURATION), K_NO_WAIT);
	k_busy_wait(TIMER_DELAY_US);
	zassert_not_equal(handler_result, HANDLER_TOKEN,
		"timer interrupt was serviced while interrupts are locked");

	printk("unlocking interrupts\n");
	irq_unlock(key);

	k_busy_wait(TIMER_DELAY_US);

	zassert_equal(handler_result, HANDLER_TOKEN,
		"timer should have fired");

	k_timer_stop(&irqlock_timer);
}
