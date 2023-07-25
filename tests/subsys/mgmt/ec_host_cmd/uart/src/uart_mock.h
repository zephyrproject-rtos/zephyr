/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SRC_UART_MOCK_H__
#define SRC_UART_MOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

extern struct device uart_mock;

struct uart_mock_data {
	uint8_t *rx_buf;
	const uint8_t *tx_buf;
	size_t tx_len;
	int32_t rx_timeout;
	size_t rx_buf_size;
	uart_callback_t cb;
	void *user_data;
	struct k_sem resp_sent;
};

int uart_mock_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* SRC_UART_MOCK_H__ */
