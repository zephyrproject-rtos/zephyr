/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/interrupt_util.h>

/*
 * The test uses GIC SGI (software generated interrupt) lines, raised through
 * trigger_irq(). SGIs 0-2 are used by Zephyr for SMP IPIs and SGIs 8-15 are
 * inaccessible from the Non-Secure state, so use lines 5-7.
 *
 * For the nested case the inner line must preempt the outer handler:
 * IRQ_DEFAULT_PRIORITY and the highest possible priority 0x0 are far enough
 * apart to guarantee preemption for any legal GICC BPR setting.
 */
#define SIMPLE_LINE	5
#define OUTER_LINE	6
#define INNER_LINE	7

#define SIMPLE_PRIO	IRQ_DEFAULT_PRIORITY
#define OUTER_PRIO	IRQ_DEFAULT_PRIORITY
#define INNER_PRIO	0x0

/* Generous bound on how long the outer handler waits to be preempted */
#define PREEMPT_WAIT_US	10000

static volatile unsigned int simple_seen;
static volatile bool simple_ran;

static volatile unsigned int outer_seen_before;
static volatile unsigned int outer_seen_after;
static volatile unsigned int inner_seen;
static volatile bool inner_ran;
static volatile bool inner_ran_during_outer;
static volatile bool outer_ran;

static void simple_isr(const void *arg)
{
	ARG_UNUSED(arg);

	simple_seen = k_irq_get_active();
	simple_ran = true;
}

static void inner_isr(const void *arg)
{
	ARG_UNUSED(arg);

	inner_seen = k_irq_get_active();
	inner_ran = true;
}

static void outer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	outer_seen_before = k_irq_get_active();

	trigger_irq(INNER_LINE);

	/*
	 * Interrupts are re-enabled while a registered handler runs, so the
	 * higher-priority inner SGI preempts this handler right here.
	 */
	for (unsigned int i = 0U; i < PREEMPT_WAIT_US && !inner_ran; i++) {
		k_busy_wait(1);
	}
	inner_ran_during_outer = inner_ran;

	outer_seen_after = k_irq_get_active();
	outer_ran = true;
}

/**
 * @brief Test that no active IRQ is reported outside interrupt context
 *
 * @ingroup kernel_interrupt_tests
 *
 * @see k_irq_get_active()
 */
ZTEST(irq_active_tracking, test_irq_active_none_in_thread)
{
	zassert_equal(k_irq_get_active(), K_IRQ_ACTIVE_NONE,
		      "an active IRQ is reported in thread context");
}

/**
 * @brief Test that a handler observes its own interrupt line as active
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details Trigger a software generated interrupt and let its handler call
 * k_irq_get_active(): it must see the line it was connected to, and the thread
 * must see no active line once the handler has returned.
 *
 * @see k_irq_get_active()
 */
ZTEST(irq_active_tracking, test_irq_active_in_isr)
{
	IRQ_CONNECT(SIMPLE_LINE, SIMPLE_PRIO, simple_isr, NULL, 0);
	irq_enable(SIMPLE_LINE);

	trigger_irq(SIMPLE_LINE);

	for (unsigned int i = 0U; i < PREEMPT_WAIT_US && !simple_ran; i++) {
		k_busy_wait(1);
	}

	zassert_true(simple_ran, "interrupt was not delivered");
	zassert_equal(simple_seen, SIMPLE_LINE,
		      "handler saw active line %u, not %u", simple_seen, SIMPLE_LINE);
	zassert_equal(k_irq_get_active(), K_IRQ_ACTIVE_NONE,
		      "an active IRQ is reported after the handler returned");
}

/**
 * @brief Test that nested interrupts unwind the active line correctly
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details A low-priority handler raises a high-priority interrupt that
 * preempts it. The inner handler must see its own line as active, and once
 * it returns the outer handler must see its own line again, exercising the
 * save/restore of the tracked value across a nested interrupt.
 *
 * @see k_irq_get_active()
 */
ZTEST(irq_active_tracking, test_irq_active_nested)
{
	IRQ_CONNECT(OUTER_LINE, OUTER_PRIO, outer_isr, NULL, 0);
	IRQ_CONNECT(INNER_LINE, INNER_PRIO, inner_isr, NULL, 0);
	irq_enable(OUTER_LINE);
	irq_enable(INNER_LINE);

	trigger_irq(OUTER_LINE);

	for (unsigned int i = 0U; i < PREEMPT_WAIT_US && !outer_ran; i++) {
		k_busy_wait(1);
	}

	zassert_true(outer_ran, "outer interrupt was not delivered");
	zassert_true(inner_ran_during_outer,
		     "inner interrupt did not preempt the outer handler");
	zassert_equal(outer_seen_before, OUTER_LINE,
		      "outer handler saw active line %u, not %u",
		      outer_seen_before, OUTER_LINE);
	zassert_equal(inner_seen, INNER_LINE,
		      "inner handler saw active line %u, not %u",
		      inner_seen, INNER_LINE);
	zassert_equal(outer_seen_after, OUTER_LINE,
		      "active line was not restored to %u after nesting, got %u",
		      OUTER_LINE, outer_seen_after);
	zassert_equal(k_irq_get_active(), K_IRQ_ACTIVE_NONE,
		      "an active IRQ is reported after the handlers returned");
}

ZTEST_SUITE(irq_active_tracking, NULL, NULL, NULL, NULL, NULL);
