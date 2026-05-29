/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_mock.h>

#include "uart_mock.h"

static struct uart_mock_data mock_data;
static struct device_state mock_state = {
	.init_res = 0,
	.initialized = 1,
};

static int uart_mock_callback_set(const struct device *dev, uart_callback_t callback,
				  void *user_data)
{
	struct uart_mock_data *data = dev->data;

	data->user_data = user_data;
	data->cb = callback;

	return 0;
}

int uart_mock_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout)
{
	struct uart_mock_data *data = dev->data;

	data->tx_buf = buf;

	ztest_check_expected_data(buf, len);
	ztest_check_expected_value(len);

	k_sem_give(&data->resp_sent);

	return 0;
}

static int uart_mock_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	struct uart_mock_data *data = dev->data;

	data->rx_buf = buf;
	data->rx_buf_size = len;
	data->rx_timeout = timeout;

	return 0;
}

static int uart_mock_rx_disable(const struct device *dev)
{
	return 0;
}

static DEVICE_API(uart, mock_api) = {
	.callback_set = uart_mock_callback_set,
	.tx = uart_mock_tx,
	.rx_enable = uart_mock_rx_enable,
	.rx_disable = uart_mock_rx_disable,
};

struct device uart_mock = {
	.api = &mock_api,
	.data = &mock_data,
	.state = &mock_state,
};
