/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/interrupt_controller/intc_infineon_aurix.h>

/* General purpose service requests GPSR0_SR0 and GPSR0_SR1 (software only) */
#if defined(CONFIG_CPU_TC18)
#define DIRECT_IRQ 1304
#else
#define DIRECT_IRQ 612
#endif
#define REGULAR_IRQ (DIRECT_IRQ + 1)

#define DIRECT_PRIO  20
#define REGULAR_PRIO 21

#define WAKER_STACK_SIZE 1024

static volatile uint32_t direct_count;
static volatile bool direct_in_isr;
static volatile bool direct_wake;
static volatile uint32_t regular_count;
static volatile uint32_t sequence;
static volatile uint32_t woken_at;

static K_SEM_DEFINE(wake_sem, 0, 1);
static K_THREAD_STACK_DEFINE(waker_stack, WAKER_STACK_SIZE);
static struct k_thread waker_thread;

ISR_DIRECT_DECLARE(direct_isr)
{
	direct_count++;
	direct_in_isr = k_is_in_isr();

	if (direct_wake) {
		k_sem_give(&wake_sem);
		return 1;
	}

	return 0;
}

static void regular_isr(const void *arg)
{
	ARG_UNUSED(arg);

	regular_count++;
}

static void wait_count(volatile uint32_t *count, uint32_t expected)
{
	for (int i = 0; i < 1000000 && *count != expected; i++) {
	}
}

static void waker_entry(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	k_sem_take(&wake_sem, K_FOREVER);
	woken_at = ++sequence;
}

static void *direct_irq_setup(void)
{
	IRQ_DIRECT_CONNECT(DIRECT_IRQ, DIRECT_PRIO, direct_isr, 0);
	IRQ_CONNECT(REGULAR_IRQ, REGULAR_PRIO, regular_isr, NULL, 0);
	irq_enable(DIRECT_IRQ);
	irq_enable(REGULAR_IRQ);

	return NULL;
}

ZTEST(tricore_direct_irq, test_direct_isr)
{
	uint32_t direct = direct_count;
	uint32_t regular = regular_count;

	direct_wake = false;
	intc_aurix_ir_irq_raise(DIRECT_IRQ);
	wait_count(&direct_count, direct + 1);
	zassert_equal(direct_count, direct + 1, "direct ISR did not run");
	zassert_true(direct_in_isr, "direct ISR not in interrupt context");

	intc_aurix_ir_irq_raise(REGULAR_IRQ);
	wait_count(&regular_count, regular + 1);
	zassert_equal(regular_count, regular + 1, "regular ISR did not run");
}

ZTEST(tricore_direct_irq, test_direct_isr_reschedule)
{
	uint32_t direct = direct_count;
	uint32_t raised_at;

	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(5));
	k_thread_create(&waker_thread, waker_stack, WAKER_STACK_SIZE, waker_entry, NULL, NULL,
			NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	/* Let the waker block on the semaphore */
	k_sleep(K_MSEC(10));

	direct_wake = true;
	intc_aurix_ir_irq_raise(DIRECT_IRQ);
	wait_count(&direct_count, direct + 1);
	raised_at = ++sequence;

	zassert_equal(direct_count, direct + 1, "direct ISR did not run");
	zassert_true(woken_at != 0 && woken_at < raised_at,
		     "woken thread did not run at the ISR exit (woken %u, raiser %u)", woken_at,
		     raised_at);

	k_thread_join(&waker_thread, K_FOREVER);
}

ZTEST_SUITE(tricore_direct_irq, NULL, direct_irq_setup, NULL, NULL, NULL);
