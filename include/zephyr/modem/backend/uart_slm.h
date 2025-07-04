/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/modem/stats.h>

#ifndef ZEPHYR_MODEM_BACKEND_UART_SLM_
#define ZEPHYR_MODEM_BACKEND_UART_SLM_

#ifdef __cplusplus
extern "C" {
#endif

struct slm_rx_queue_event {
	uint8_t *buf;
	size_t len;
};

struct modem_backend_uart_slm {
	const struct device *uart;
	struct modem_pipe pipe;
	struct k_work_delayable receive_ready_work;
	struct k_work transmit_idle_work;

#ifdef CONFIG_MODEM_STATS
	struct modem_stats_buffer receive_buf_stats;
	struct modem_stats_buffer transmit_buf_stats;
#endif
	struct ring_buf transmit_rb;
	struct k_work rx_disabled_work;
	atomic_t state;

	struct k_mem_slab rx_slab;
	struct k_msgq rx_queue;
	struct slm_rx_queue_event rx_event;
	struct slm_rx_queue_event rx_queue_buf[CONFIG_MODEM_BACKEND_UART_SLM_BUFFER_COUNT];
	uint32_t rx_buf_size;
	uint8_t rx_buf_count;
};

struct modem_backend_uart_slm_config {
	const struct device *uart;
	uint8_t *receive_buf; /* Address must be word-aligned. */
	uint32_t receive_buf_size;
	uint8_t *transmit_buf;
	uint32_t transmit_buf_size;
};

struct modem_pipe *modem_backend_uart_slm_init(struct modem_backend_uart_slm *backend,
					       const struct modem_backend_uart_slm_config *config);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODEM_BACKEND_UART_SLM_ */
