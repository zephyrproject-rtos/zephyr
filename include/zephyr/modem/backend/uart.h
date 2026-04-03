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
#include <zephyr/modem/stats.h>

#ifndef ZEPHYR_MODEM_BACKEND_UART_
#define ZEPHYR_MODEM_BACKEND_UART_

#ifdef __cplusplus
extern "C" {
#endif

struct modem_backend_uart_isr {
	struct ring_buf receive_rdb[2];
	struct ring_buf transmit_rb;
	atomic_t transmit_buf_len;
	atomic_t receive_buf_len;
	uint8_t receive_rdb_used;
	uint32_t transmit_buf_put_limit;
};

struct modem_backend_uart_async_common {
	uint8_t *transmit_buf;
	uint32_t transmit_buf_size;
	struct k_work rx_disabled_work;
	atomic_t state;
};

#ifdef CONFIG_MODEM_BACKEND_UART_ASYNC_HWFC

struct rx_queue_event {
	uint8_t *buf;
	size_t len;
};

struct modem_backend_uart_async {
	struct modem_backend_uart_async_common common;
	struct k_mem_slab rx_slab;
	struct k_msgq rx_queue;
	struct rx_queue_event rx_event;
	struct rx_queue_event rx_queue_buf[CONFIG_MODEM_BACKEND_UART_ASYNC_HWFC_BUFFER_COUNT];
	uint32_t rx_buf_size;
	uint8_t rx_buf_count;
};

#else

struct modem_backend_uart_async {
	struct modem_backend_uart_async_common common;
	uint8_t *receive_bufs[2];
	uint32_t receive_buf_size;
	struct ring_buf receive_rb;
	struct k_spinlock receive_rb_lock;
};

#endif /* CONFIG_MODEM_BACKEND_UART_ASYNC_HWFC */

struct modem_backend_uart {
	const struct device *uart;
	const struct gpio_dt_spec *dtr_gpio;
	struct modem_pipe pipe;
	struct k_work_delayable receive_ready_work;
	struct k_work transmit_idle_work;

#if CONFIG_MODEM_STATS
	struct modem_stats_buffer receive_buf_stats;
	struct modem_stats_buffer transmit_buf_stats;
#endif

	union {
		struct modem_backend_uart_isr isr;
		struct modem_backend_uart_async async;
	};
};

struct modem_backend_uart_config {
	const struct device *uart;
	const struct gpio_dt_spec *dtr_gpio;
	/* Address must be word-aligned when CONFIG_MODEM_BACKEND_UART_ASYNC_HWFC is enabled. */
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
