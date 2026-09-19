/*
 * Copyright (c) 2026 Picoheart Inc.
 * Copyright (c) 2026 Zhan Gao <gaozhan.9426@picoheart.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Exercises IRQ_STORM_DETECTION end-to-end on a RISC-V QEMU target.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/sys/irq_storm.h>
#include <zephyr/ztest.h>

#define UART0_NODE DT_NODELABEL(uart0)
#if !DT_NODE_HAS_STATUS(UART0_NODE, okay)
#error "Node 'uart0' is not available/enabled in the devicetree."
#endif
#define UART0_IRQN DT_IRQN(UART0_NODE)

/* Generous but finite bound on busy-spin polling below, so a genuine
 * regression fails the test instead of hanging the run forever.
 */
#define SPIN_LIMIT 100000000

/*
 * PLIC-triggered storm: drive uart0's TX-ready (THRE) interrupt. This
 * exercises the PLIC driver's irq_dispatch() path and confirms that
 * IRQ_STORM_DETECTION's irq_disable() masks the storming hwirq at the PLIC.
 *
 * THRE is edge-latched, so the callback below re-fills the FIFO on every
 * dispatch to keep the interrupt re-firing, mimicking an unacknowledged
 * level-triggered source.
 */

static const struct device *const uart0_dev = DEVICE_DT_GET(UART0_NODE);

static volatile uint32_t uart_storm_hits;
static volatile uint32_t uart_storm_budget;

static void uart_storm_cb(const struct device *dev, void *user_data)
{
	static const uint8_t filler = ' ';

	ARG_UNUSED(user_data);

	uart_irq_update(dev);

	if (!uart_irq_tx_ready(dev)) {
		return;
	}

	uart_storm_hits++;

	if (uart_storm_budget > 0U) {
		uart_storm_budget--;
		/* Write a byte so THR transitions non-empty -> empty
		 * again once it drains, re-latching THRE.
		 */
		(void)uart_fifo_fill(dev, &filler, 1);
	} else {
		/* Detection did not mask the line in time. Stop generating the
		 * condition ourselves rather than looping forever.
		 */
		uart_irq_tx_disable(dev);
	}
}

/* Poll irq_is_enabled() instead of sleeping: the pending storming external
 * interrupt outranks the timer interrupt (MEI > MTI), so k_msleep() would
 * never return while the storm is live.
 */
static bool spin_until_uart_masked(void)
{
	for (uint32_t i = 0; i < SPIN_LIMIT; i++) {
		if (!irq_is_enabled(UART0_IRQN)) {
			return true;
		}
	}
	return false;
}

ZTEST(irq_storm, test_plic_storm_gets_masked)
{
	zassert_true(device_is_ready(uart0_dev), "uart0 device is not ready");

	uart_storm_hits = 0;
	uart_storm_budget = 10 * CONFIG_IRQ_STORM_THRESHOLD;

	uart_irq_callback_user_data_set(uart0_dev, uart_storm_cb, NULL);

	/* THR is empty, so enabling TX IRQ fires the line immediately. */
	uart_irq_tx_enable(uart0_dev);

	zassert_true(spin_until_uart_masked(),
		     "PLIC irq storm was never masked (hits=%u, budget=%u)", uart_storm_hits,
		     uart_storm_budget);

	zassert_false(irq_is_enabled(UART0_IRQN),
		      "irq_disable() should have masked the PLIC-routed storming line");
	/* The safety-net budget should not have been the reason it stopped. */
	zassert_true(uart_storm_budget > 0,
		     "PLIC storm was bounded by the fallback budget, not by detection");

	/* Re-enable and confirm the line stays quiet with nothing asserting it. */
	uart_irq_tx_disable(uart0_dev);
	irq_enable(UART0_IRQN);
	uart_irq_callback_user_data_set(uart0_dev, NULL, NULL);

	zassert_true(irq_is_enabled(UART0_IRQN), "line should be re-armed after recovery");
}

ZTEST_SUITE(irq_storm, NULL, NULL, NULL, NULL, NULL);
