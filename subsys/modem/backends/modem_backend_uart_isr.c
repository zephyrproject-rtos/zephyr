/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "modem_backend_uart_isr.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_backend_uart_isr, CONFIG_MODEM_MODULES_LOG_LEVEL);

#include <string.h>

static void modem_backend_uart_isr_flush(struct modem_backend_uart *backend)
{
	uint8_t c;

	while (uart_fifo_read(backend->uart, &c, 1) > 0) {
		continue;
	}
}

static void modem_backend_uart_isr_irq_handler_receive_ready(struct modem_backend_uart *backend)
{
	uint32_t size;
	struct ring_buf *receive_rb;
	uint8_t *buffer;
	int ret;

	receive_rb = &backend->isr.receive_rdb[backend->isr.receive_rdb_used];
	size = ring_buf_put_claim(receive_rb, &buffer, UINT32_MAX);
	if (size == 0) {
		/* This can be caused by
		 * - a too long CONFIG_MODEM_BACKEND_UART_ISR_RECEIVE_IDLE_TIMEOUT_MS
		 * - or a too small receive_buf_size
		 * relatively to the (too high) baud rate and amount of incoming data.
		 */
		LOG_WRN("Receive buffer overrun");
		ring_buf_put_finish(receive_rb, 0);
		ring_buf_reset(receive_rb);
		size = ring_buf_put_claim(receive_rb, &buffer, UINT32_MAX);
	}

	ret = uart_fifo_read(backend->uart, buffer, size);
	if (ret <= 0) {
		ring_buf_put_finish(receive_rb, 0);
		return;
	}
	ring_buf_put_finish(receive_rb, (uint32_t)ret);

	if (ring_buf_space_get(receive_rb) > ring_buf_capacity_get(receive_rb) / 20) {
		/*
		 * Avoid having the receiver call modem_pipe_receive() too often (e.g. every byte).
		 * It temporarily disables the UART RX IRQ when swapping buffers
		 * which can cause byte loss at higher baud rates.
		 */
		k_work_schedule(&backend->receive_ready_work,
			K_MSEC(CONFIG_MODEM_BACKEND_UART_ISR_RECEIVE_IDLE_TIMEOUT_MS));
	} else {
		/* The buffer is getting full. Run the work item immediately to free up space. */
		k_work_reschedule(&backend->receive_ready_work, K_NO_WAIT);
	}
}

static void modem_backend_uart_isr_irq_handler_transmit_ready(struct modem_backend_uart *backend)
{
	uint32_t size;
	uint8_t *buffer;
	int ret;

	if (ring_buf_is_empty(&backend->isr.transmit_rb) == true) {
		uart_irq_tx_disable(backend->uart);
		k_work_submit(&backend->transmit_idle_work);
		return;
	}

	size = ring_buf_get_claim(&backend->isr.transmit_rb, &buffer, UINT32_MAX);
	ret = uart_fifo_fill(backend->uart, buffer, size);
	if (ret < 0) {
		ring_buf_get_finish(&backend->isr.transmit_rb, 0);
	} else {
		ring_buf_get_finish(&backend->isr.transmit_rb, (uint32_t)ret);

		/* Update transmit buf capacity tracker */
		atomic_sub(&backend->isr.transmit_buf_len, (uint32_t)ret);
	}
}

static void modem_backend_uart_isr_irq_handler(const struct device *uart, void *user_data)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)user_data;

	if (uart_irq_update(uart) < 1) {
		return;
	}

	if (uart_irq_rx_ready(uart)) {
		modem_backend_uart_isr_irq_handler_receive_ready(backend);
	}

	if (uart_irq_tx_ready(uart)) {
		modem_backend_uart_isr_irq_handler_transmit_ready(backend);
	}
}

static int modem_backend_uart_isr_open(void *data)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)data;

	ring_buf_reset(&backend->isr.receive_rdb[0]);
	ring_buf_reset(&backend->isr.receive_rdb[1]);
	ring_buf_reset(&backend->isr.transmit_rb);
	atomic_set(&backend->isr.transmit_buf_len, 0);
	modem_backend_uart_isr_flush(backend);
	uart_irq_rx_enable(backend->uart);
	uart_irq_tx_enable(backend->uart);
	modem_pipe_notify_opened(&backend->pipe);
	return 0;
}

static uint32_t get_transmit_buf_length(struct modem_backend_uart *backend)
{
	return atomic_get(&backend->isr.transmit_buf_len);
}

#if CONFIG_MODEM_STATS
static uint32_t get_receive_buf_length(struct modem_backend_uart *backend)
{
	return ring_buf_size_get(&backend->isr.receive_rdb[0]) +
	       ring_buf_size_get(&backend->isr.receive_rdb[1]);
}

static uint32_t get_receive_buf_size(struct modem_backend_uart *backend)
{
	return ring_buf_capacity_get(&backend->isr.receive_rdb[0]) +
	       ring_buf_capacity_get(&backend->isr.receive_rdb[1]);
}

static uint32_t get_transmit_buf_size(struct modem_backend_uart *backend)
{
	return ring_buf_capacity_get(&backend->isr.transmit_rb);
}

static void advertise_transmit_buf_stats(struct modem_backend_uart *backend)
{
	uint32_t length;

	length = get_transmit_buf_length(backend);
	modem_stats_buffer_advertise_length(&backend->transmit_buf_stats, length);
}

static void advertise_receive_buf_stats(struct modem_backend_uart *backend)
{
	uint32_t length;

	uart_irq_rx_disable(backend->uart);
	length = get_receive_buf_length(backend);
	uart_irq_rx_enable(backend->uart);
	modem_stats_buffer_advertise_length(&backend->receive_buf_stats, length);
}
#endif

static bool modem_backend_uart_isr_transmit_buf_above_limit(struct modem_backend_uart *backend)
{
	return backend->isr.transmit_buf_put_limit < get_transmit_buf_length(backend);
}

static int modem_backend_uart_isr_transmit(void *data, const uint8_t *buf, size_t size)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)data;
	int written;

	if (modem_backend_uart_isr_transmit_buf_above_limit(backend) == true) {
		return 0;
	}

	uart_irq_tx_disable(backend->uart);
	written = ring_buf_put(&backend->isr.transmit_rb, buf, size);
	uart_irq_tx_enable(backend->uart);

	/* Update transmit buf capacity tracker */
	atomic_add(&backend->isr.transmit_buf_len, written);

#if CONFIG_MODEM_STATS
	advertise_transmit_buf_stats(backend);
#endif

	return written;
}

static int modem_backend_uart_isr_receive(void *data, uint8_t *buf, size_t size)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)data;

	uint32_t read_bytes;
	uint8_t receive_rdb_unused;

#if CONFIG_MODEM_STATS
	advertise_receive_buf_stats(backend);
#endif

	read_bytes = 0;
	receive_rdb_unused = (backend->isr.receive_rdb_used == 1) ? 0 : 1;

	/* Read data from unused ring double buffer first */
	read_bytes += ring_buf_get(&backend->isr.receive_rdb[receive_rdb_unused], buf, size);

	if (ring_buf_is_empty(&backend->isr.receive_rdb[receive_rdb_unused]) == false) {
		return (int)read_bytes;
	}

	/* Swap receive ring double buffer */
	uart_irq_rx_disable(backend->uart);
	backend->isr.receive_rdb_used = receive_rdb_unused;
	uart_irq_rx_enable(backend->uart);

	/* Read data from previously used buffer */
	receive_rdb_unused = (backend->isr.receive_rdb_used == 1) ? 0 : 1;

	read_bytes += ring_buf_get(&backend->isr.receive_rdb[receive_rdb_unused],
				   &buf[read_bytes], (size - read_bytes));

	return (int)read_bytes;
}

static int modem_backend_uart_isr_close(void *data)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)data;

	uart_irq_rx_disable(backend->uart);
	uart_irq_tx_disable(backend->uart);
	modem_pipe_notify_closed(&backend->pipe);
	return 0;
}

struct modem_pipe_api modem_backend_uart_isr_api = {
	.open = modem_backend_uart_isr_open,
	.transmit = modem_backend_uart_isr_transmit,
	.receive = modem_backend_uart_isr_receive,
	.close = modem_backend_uart_isr_close,
};

#if CONFIG_MODEM_STATS
static void init_stats(struct modem_backend_uart *backend)
{
	char name[CONFIG_MODEM_STATS_BUFFER_NAME_SIZE];
	uint32_t receive_buf_size;
	uint32_t transmit_buf_size;

	receive_buf_size = get_receive_buf_size(backend);
	transmit_buf_size = get_transmit_buf_size(backend);

	snprintk(name, sizeof(name), "%s_%s", backend->uart->name, "rx");
	modem_stats_buffer_init(&backend->receive_buf_stats, name, receive_buf_size);
	snprintk(name, sizeof(name), "%s_%s", backend->uart->name, "tx");
	modem_stats_buffer_init(&backend->transmit_buf_stats, name, transmit_buf_size);
}
#endif

void modem_backend_uart_isr_init(struct modem_backend_uart *backend,
				 const struct modem_backend_uart_config *config)
{
	uint32_t receive_double_buf_size;

	backend->isr.transmit_buf_put_limit =
		config->transmit_buf_size - (config->transmit_buf_size / 4);

	receive_double_buf_size = config->receive_buf_size / 2;

	ring_buf_init(&backend->isr.receive_rdb[0], receive_double_buf_size,
		      &config->receive_buf[0]);

	ring_buf_init(&backend->isr.receive_rdb[1], receive_double_buf_size,
		      &config->receive_buf[receive_double_buf_size]);

	ring_buf_init(&backend->isr.transmit_rb, config->transmit_buf_size,
		      config->transmit_buf);

	atomic_set(&backend->isr.transmit_buf_len, 0);
	uart_irq_rx_disable(backend->uart);
	uart_irq_tx_disable(backend->uart);
	uart_irq_callback_user_data_set(backend->uart, modem_backend_uart_isr_irq_handler,
					backend);

	modem_pipe_init(&backend->pipe, backend, &modem_backend_uart_isr_api);

#if CONFIG_MODEM_STATS
	init_stats(backend);
#endif
}
