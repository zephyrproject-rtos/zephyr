/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/ztest.h>

static const struct device *const irq_dev = DEVICE_DT_GET(DT_NODELABEL(euart_irq));
static const struct device *const async_dev = DEVICE_DT_GET(DT_NODELABEL(euart_async));
static const struct device *const poll_dev = DEVICE_DT_GET(DT_NODELABEL(euart_poll));

static void irq_cb(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);
}

static void async_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(evt);
	ARG_UNUSED(user_data);
}

ZTEST(uart_emul_modes, test_polling)
{
	const struct device *const devs[] = {irq_dev, async_dev, poll_dev};

	for (size_t i = 0; i < ARRAY_SIZE(devs); i++) {
		uint8_t c = 0U;

		uart_poll_out(devs[i], 'z');
		zassert_equal(uart_emul_get_tx_data(devs[i], &c, 1), 1);
		zassert_equal(c, 'z');
	}
}

ZTEST(uart_emul_modes, test_interrupt_driven)
{
	uint8_t c = 'z';

	Z_TEST_SKIP_IFNDEF(CONFIG_UART_INTERRUPT_DRIVEN);

	zassert_ok(uart_irq_callback_user_data_set(irq_dev, irq_cb, NULL));
	zassert_equal(uart_irq_callback_user_data_set(async_dev, irq_cb, NULL), -ENOSYS);
	zassert_equal(uart_irq_callback_user_data_set(poll_dev, irq_cb, NULL), -ENOSYS);
	zassert_equal(uart_fifo_fill(async_dev, &c, 1), -ENOSYS);
	zassert_equal(uart_fifo_fill(poll_dev, &c, 1), -ENOSYS);
}

ZTEST(uart_emul_modes, test_async)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_UART_ASYNC_API);

	zassert_ok(uart_callback_set(async_dev, async_cb, NULL));
	zassert_equal(uart_callback_set(irq_dev, async_cb, NULL), -ENOSYS);
	zassert_equal(uart_callback_set(poll_dev, async_cb, NULL), -ENOSYS);
}

ZTEST_SUITE(uart_emul_modes, NULL, NULL, NULL, NULL, NULL);
