/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/interrupt_util.h>

static volatile unsigned int handler_runs;

static void pending_isr(const void *arg)
{
	ARG_UNUSED(arg);
	handler_runs++;
}

/*
 * Reserve an interrupt line and keep it disabled, so that a latched interrupt
 * survives until the line is enabled. The caller decides what to latch on it
 * and when to enable.
 *
 * This test application deliberately has no other statically connected
 * interrupts, because get_available_nvic_line() can hand back a line that is
 * owned by a currently disabled IRQ_CONNECT() entry.
 */
static unsigned int connect_disabled_irq_line(void)
{
	unsigned int irq = get_available_nvic_line(CONFIG_NUM_IRQS);

	handler_runs = 0;

	zassert_true(arch_irq_connect_dynamic(irq, 1, pending_isr, NULL, 0) > 0,
		     "irq connect dynamic failed");

	k_irq_disable(irq);

	return irq;
}

/* As above, plus an interrupt already latched by the hardware trigger path. */
static unsigned int pend_disabled_irq_line(void)
{
	unsigned int irq = connect_disabled_irq_line();

	trigger_irq(irq);

	zassert_equal(handler_runs, 0, "handler ran while the line was disabled");

	return irq;
}

/**
 * @brief Test that a latched interrupt is delivered once the line is enabled
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details Control case for test_irq_clear_pending(). Without this the clear
 * test could pass vacuously, having never latched anything to begin with.
 *
 * @see k_irq_enable()
 */
ZTEST(irq_pending, test_irq_pending_without_clear)
{
	unsigned int irq = pend_disabled_irq_line();

	k_irq_enable(irq);
	k_irq_disable(irq);

	zassert_equal(handler_runs, 1, "latched interrupt was not delivered (%u)", handler_runs);
}

/**
 * @brief Test that k_irq_clear_pending() discards a latched interrupt
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details Latches an interrupt on a disabled line, clears it, then enables
 * the line. The handler must not run, because the pending state was dropped
 * before delivery could happen.
 *
 * @see k_irq_clear_pending()
 */
ZTEST(irq_pending, test_irq_clear_pending)
{
	unsigned int irq = pend_disabled_irq_line();

	k_irq_clear_pending(irq);

	k_irq_enable(irq);
	k_irq_disable(irq);

	zassert_equal(handler_runs, 0, "cleared interrupt was still delivered (%u)", handler_runs);
}

/**
 * @brief Test that k_irq_set_pending() latches an interrupt from software
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details Latches an interrupt with k_irq_set_pending() on a disabled line,
 * then enables the line and checks the handler ran exactly once, without the
 * peripheral trigger path being involved at all.
 *
 * @see k_irq_set_pending()
 */
ZTEST(irq_pending, test_irq_set_pending)
{
	unsigned int irq = connect_disabled_irq_line();

	k_irq_set_pending(irq);

	zassert_equal(handler_runs, 0, "handler ran while the line was disabled");

	k_irq_enable(irq);
	k_irq_disable(irq);

	zassert_equal(handler_runs, 1,
		      "software-latched interrupt was not delivered (%u)", handler_runs);
}

/**
 * @brief Test that k_irq_is_pending() tracks the latched state
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details Walks one interrupt line through not-pending, pending and
 * cleared states, checking k_irq_is_pending() reports each transition.
 *
 * @see k_irq_is_pending()
 */
ZTEST(irq_pending, test_irq_is_pending)
{
	unsigned int irq = connect_disabled_irq_line();

	zassert_false(k_irq_is_pending(irq), "line pending before anything was latched");

	k_irq_set_pending(irq);

	zassert_true(k_irq_is_pending(irq), "latched interrupt not reported as pending");

	k_irq_clear_pending(irq);

	zassert_false(k_irq_is_pending(irq), "cleared interrupt still reported as pending");

	zassert_equal(handler_runs, 0, "handler ran while the line was disabled");
}

ZTEST_SUITE(irq_pending, NULL, NULL, NULL, NULL, NULL);
