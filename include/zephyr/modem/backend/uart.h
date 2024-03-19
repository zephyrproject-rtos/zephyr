/*
 * Copyright (c) 2022 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/modem/pipe.h>

#ifndef ZEPHYR_MODEM_BACKEND_UART_
#define ZEPHYR_MODEM_BACKEND_UART_

#ifdef __cplusplus
extern "C" {
#endif

struct modem_backend_uart_isr {
	struct ring_buf receive_rdb[2];
	struct ring_buf transmit_rb;
	atomic_t transmit_buf_len;
	uint8_t receive_rdb_used;
	uint32_t transmit_buf_put_limit;
};

struct modem_backend_uart_async {
	uint8_t *receive_bufs[2];
	uint32_t receive_buf_size;
	struct ring_buf receive_rb;
	struct k_spinlock receive_rb_lock;
	uint8_t *transmit_buf;
	uint32_t transmit_buf_size;
	struct k_work rx_disabled_work;
	atomic_t state;
};

struct modem_backend_uart {
	const struct device *uart;
	struct modem_pipe pipe;
	struct k_work_delayable receive_ready_work;
	struct k_work transmit_idle_work;

	union {
		struct modem_backend_uart_isr isr;
		struct modem_backend_uart_async async;
	};
};

struct modem_backend_uart_config {
	const struct device *uart;
	uint8_t *receive_buf;
	uint32_t receive_buf_size;
	uint8_t *transmit_buf;
	uint32_t transmit_buf_size;
};

struct modem_pipe *modem_backend_uart_init(struct modem_backend_uart *backend,
					   const struct modem_backend_uart_config *config);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODEM_BACKEND_UART_ */
