/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <stm32_ll_usart.h>

#define UART_NODE DT_ALIAS(test_uart)

struct tx_status {
	struct k_sem done;
	int pending;
	int ready;
	int complete;
	int filled;
	int pending_disabled;
	int ready_disabled;
	bool tc_enabled;
	bool fill;
};

static void tx_callback(const struct device *dev, void *user_data)
{
	struct tx_status *status = user_data;
	const uint8_t byte = 0x55U;

	uart_irq_update(dev);
	status->pending = uart_irq_is_pending(dev);
	status->ready = uart_irq_tx_ready(dev);
	status->complete = uart_irq_tx_complete(dev);
	status->tc_enabled = LL_USART_IsEnabledIT_TC((USART_TypeDef *)DT_REG_ADDR(UART_NODE));
	if (status->fill && status->ready > 0) {
		status->filled = uart_fifo_fill(dev, &byte, sizeof(byte));
	}
	uart_irq_tx_disable(dev);
	status->pending_disabled = uart_irq_is_pending(dev);
	status->ready_disabled = uart_irq_tx_ready(dev);
	k_sem_give(&status->done);
}

static void check_tx_irq(bool fill)
{
	const struct device *dev = DEVICE_DT_GET(UART_NODE);
	USART_TypeDef *usart = (USART_TypeDef *)DT_REG_ADDR(UART_NODE);
	struct tx_status status = {.fill = fill};
	unsigned int key;
	int ret;

	zassert_true(device_is_ready(dev));
	zassert_ok(k_sem_init(&status.done, 0, 1));
	zassert_ok(uart_irq_callback_user_data_set(dev, tx_callback, &status));

	key = irq_lock();
	/* Isolate TXE from TC without relying on the emulator's transmit timing. */
	LL_USART_ClearFlag_TC(usart);
	uart_irq_tx_enable(dev);
	irq_unlock(key);

	ret = k_sem_take(&status.done, K_MSEC(100));
	/* Remove the callback even when the expected interrupt never arrives. */
	uart_irq_tx_disable(dev);
	zassert_ok(uart_irq_callback_user_data_set(dev, NULL, NULL));
	zassert_ok(ret, "No TX interrupt while TXE is set and TC is clear");
	zassert_true(status.pending > 0, "TXE interrupt is not reported pending");
	zassert_true(status.ready > 0, "TXE interrupt is not reported ready");
	zassert_equal(status.complete, 0, "TXE must not imply transmission complete");
	zassert_true(status.tc_enabled, "TC completion interrupt was not preserved");
	zassert_equal(status.pending_disabled, 0, "Disabled TX interrupt is reported pending");
	zassert_equal(status.ready_disabled, 0, "Disabled TX interrupt is reported ready");
	zassert_equal(LL_USART_IsEnabledIT_TXE(usart), 0, "TXE interrupt still enabled");
	zassert_equal(LL_USART_IsEnabledIT_TC(usart), 0, "TC interrupt still enabled");
	if (fill) {
		zassert_equal(status.filled, 1, "TXE callback could not enqueue a byte");
	}
}

ZTEST(uart_stm32_tx_irq, test_txe_without_tc)
{
	check_tx_irq(false);
}

ZTEST(uart_stm32_tx_irq, test_fifo_fill_without_tc)
{
	check_tx_irq(true);
}

ZTEST_SUITE(uart_stm32_tx_irq, NULL, NULL, NULL, NULL, NULL);
