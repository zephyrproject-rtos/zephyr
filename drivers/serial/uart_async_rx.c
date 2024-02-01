/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/serial/uart_async_rx.h>

static uint8_t inc(struct uart_async_rx *rx_data, uint8_t val)
{
	return (val + 1) & (rx_data->config->buf_cnt - 1);
}

static struct uart_async_rx_buf *get_buf(struct uart_async_rx *rx_data, uint8_t idx)
{
	uint8_t *p = rx_data->config->buffer;

	p += idx * (rx_data->buf_len + sizeof(struct uart_async_rx_buf));

	return (struct uart_async_rx_buf *)p;
}

uint8_t *uart_async_rx_buf_req(struct uart_async_rx *rx_data)
{
	uint8_t *data = NULL;

	if (rx_data->free_buf_cnt != 0) {
		struct uart_async_rx_buf *buf = get_buf(rx_data, rx_data->drv_buf_idx);

		data = buf->buffer;
		rx_data->drv_buf_idx = inc(rx_data, rx_data->drv_buf_idx);

		atomic_dec(&rx_data->free_buf_cnt);
	}

	return data;
}

void uart_async_rx_on_rdy(struct uart_async_rx *rx_data, uint8_t *buffer, size_t length)
{
	/* Cannot use CONTAINER_OF because validation fails due to type mismatch:
	 * uint8_t * vs uint8_t [].
	 */
	struct uart_async_rx_buf *rx_buf =
		(struct uart_async_rx_buf *)(buffer - offsetof(struct uart_async_rx_buf, buffer));

	rx_buf->wr_idx += length;
	__ASSERT_NO_MSG(rx_buf->wr_idx <= rx_data->buf_len);

	atomic_add(&rx_data->pending_bytes, length);
}

static void buf_reset(struct uart_async_rx_buf *buf)
{
	buf->rd_idx = 0;
	buf->wr_idx = 0;
	buf->completed = 0;
}
static void usr_rx_buf_release(struct uart_async_rx *rx_data, struct uart_async_rx_buf *buf)
{
	buf_reset(buf);
	rx_data->rd_buf_idx = inc(rx_data, rx_data->rd_buf_idx);
	atomic_inc(&rx_data->free_buf_cnt);
	__ASSERT_NO_MSG(rx_data->free_buf_cnt <= rx_data->config->buf_cnt);
}

void uart_async_rx_on_buf_rel(struct uart_async_rx *rx_data, uint8_t *buffer)
{
	/* Cannot use CONTAINER_OF because validation fails due to type mismatch:
	 * uint8_t * vs uint8_t [].
	 */
	struct uart_async_rx_buf *rx_buf =
		(struct uart_async_rx_buf *)(buffer - offsetof(struct uart_async_rx_buf, buffer));

	rx_buf->completed = 1;
}

size_t uart_async_rx_data_claim(struct uart_async_rx *rx_data, uint8_t **data, size_t length)
{
	struct uart_async_rx_buf *buf;
	int rem;

	if ((rx_data->pending_bytes == 0) || (length == 0)) {
		return 0;
	}

	do {
		buf = get_buf(rx_data, rx_data->rd_buf_idx);
		if ((buf->rd_idx == buf->wr_idx) && (buf->completed == 1)) {
			usr_rx_buf_release(rx_data, buf);
		} else {
			break;
		}
	} while (1);

	*data = &buf->buffer[buf->rd_idx];
	rem = buf->wr_idx - buf->rd_idx;

	return MIN(length, rem);
}

void uart_async_rx_data_consume(struct uart_async_rx *rx_data, size_t length)
{
	struct uart_async_rx_buf *buf = get_buf(rx_data, rx_data->rd_buf_idx);

	buf->rd_idx += length;

	atomic_sub(&rx_data->pending_bytes, length);

	__ASSERT_NO_MSG(buf->rd_idx <= buf->wr_idx);
}

void uart_async_rx_reset(struct uart_async_rx *rx_data)
{
	rx_data->free_buf_cnt = rx_data->config->buf_cnt;
	for (uint8_t i = 0; i < rx_data->config->buf_cnt; i++) {
		buf_reset(get_buf(rx_data, i));
	}
}

int uart_async_rx_init(struct uart_async_rx *rx_data,
		       const struct uart_async_rx_config *config)
{
	__ASSERT_NO_MSG(config->length / config->buf_cnt <= UINT8_MAX);
	memset(rx_data, 0, sizeof(*rx_data));
	rx_data->config = config;
	rx_data->buf_len = (config->length / config->buf_cnt) - UART_ASYNC_RX_BUF_OVERHEAD;
	uart_async_rx_reset(rx_data);

	return 0;
}
