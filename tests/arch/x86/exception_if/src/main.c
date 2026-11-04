/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>

#define IF_BIT BIT(9)

/*
 * The IRQ state while servicing an exception must follow the interrupted
 * context: if IRQs were locked when the exception was taken, they must stay
 * locked while the exception is being handled.
 *
 * k_sys_fatal_error_handler() is called after z_fatal_error() has already
 * locked interrupts, so the handler cannot sample the entry IRQ state
 * directly. Sample a side effect instead: arm a kernel timer, let it expire
 * while interrupts are locked and then trigger a synchronous CPU exception.
 * If exception entry wrongly re-enables interrupts, the pending timer
 * interrupt is serviced and the timer expiry callback runs before the fatal
 * error handler is reached.
 */

#define TIMER_DELAY_MS 20
#define BUSY_WAIT_US   60000

static K_THREAD_STACK_DEFINE(crash_stack, 4096);
static struct k_thread crash_thread;

static void test_timer_expiry(struct k_timer *timer);
K_TIMER_DEFINE(test_timer, test_timer_expiry, NULL);

static volatile unsigned long saved_rflags;
static volatile int fatal_count;
static volatile int timer_fired;
static volatile int timer_fired_at_handler;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(reason);

	/* Record the state before any other output */
	saved_rflags = esf->rflags;
	timer_fired_at_handler = timer_fired;
	fatal_count++;
}

static void test_timer_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	timer_fired++;
}

static void crash_locked(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)irq_lock();

	/* Let the timer expire while interrupts are locked, leaving a pending
	 * timer interrupt behind.
	 */
	k_timer_start(&test_timer, K_MSEC(TIMER_DELAY_MS), K_NO_WAIT);
	k_busy_wait(BUSY_WAIT_US);

	__asm__ volatile("ud2");
}

static void crash_unlocked(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	__asm__ volatile("ud2");
}

static void reset_state(void)
{
	k_timer_stop(&test_timer);
	saved_rflags = 0UL;
	fatal_count = 0;
	timer_fired = 0;
	timer_fired_at_handler = 0;
}

static void run_crash_thread(k_thread_entry_t entry)
{
	k_thread_create(&crash_thread, crash_stack, K_THREAD_STACK_SIZEOF(crash_stack), entry, NULL,
			NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	/* Let the (higher priority) crash thread run to completion */
	k_yield();
}

static void wait_for_timer(void)
{
	for (int i = 0; i < 100 && timer_fired == 0; i++) {
		k_busy_wait(1000);
	}
}

ZTEST(exception_irq_state, test_locked_irq_exception_keeps_irqs_locked)
{
	reset_state();
	run_crash_thread(crash_locked);

	zassert_equal(fatal_count, 1, "fatal error handler not invoked");
	zassert_equal(saved_rflags & IF_BIT, 0UL,
		      "interrupted IF state not preserved (RFLAGS 0x%lx)", saved_rflags);
	zassert_equal(timer_fired_at_handler, 0,
		      "pending timer IRQ serviced during exception entry");

	/* The timer must fire once interrupts are enabled again, proving that a
	 * pending interrupt was actually left behind by the crash thread.
	 */
	wait_for_timer();
	zassert_equal(timer_fired, 1, "test timer did not expire");
}

ZTEST(exception_irq_state, test_unlocked_irq_exception_policy)
{
	reset_state();
	run_crash_thread(crash_unlocked);

	zassert_equal(fatal_count, 1, "fatal error handler not invoked");
	zassert_not_equal(saved_rflags & IF_BIT, 0UL,
			  "IF clear for an interrupted context with IRQs enabled (RFLAGS 0x%lx)",
			  saved_rflags);
}

ZTEST_SUITE(exception_irq_state, NULL, NULL, NULL, NULL, NULL);
