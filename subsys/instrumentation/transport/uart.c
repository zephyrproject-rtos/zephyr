/*
 * Copyright 2023 Linaro
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 * Copyright (c) 2026 Analog Devices
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/instrumentation/instrumentation.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/ring_buffer.h>

#include "instr_transport.h"
#include "instr_buffer.h"

#define INSTR_START_TAG "-*-#"
#define INSTR_END_TAG   "-*-!\n"

#define COMMAND_BUFFER_SIZE 32
char _cmd_buffer[COMMAND_BUFFER_SIZE];

extern int instr_disable(void);
extern void instr_dump_deltas(void);

__no_instrumentation__ static void uart_isr(const struct device *uart_dev, void *user_data)
{
	uint8_t byte = 0;
	static uint32_t cur;

	ARG_UNUSED(user_data);

	uart_irq_update(uart_dev);

	if (uart_irq_rx_ready(uart_dev) <= 0) {
		return;
	}

	while (uart_fifo_read(uart_dev, &byte, 1) == 1) {
		if (!isprint(byte)) {
			if (byte == '\r') {
				_cmd_buffer[cur] = '\0';
				const char *prefix = "instr_";
				size_t prefix_len = strlen(prefix);

				if (strncmp(_cmd_buffer, prefix, prefix_len) == 0) {
					instr_cmd_handle(_cmd_buffer + prefix_len,
							 cur - prefix_len);
				} else {
					instr_cmd_handle(_cmd_buffer, cur);
				}

				cur = 0U;
			}

			continue;
		}

		if (cur < (COMMAND_BUFFER_SIZE - 1)) {
			_cmd_buffer[cur++] = byte;
		}
	}
}

__no_instrumentation__ static int uart_isr_init(void)
{
	static const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	__ASSERT(device_is_ready(uart_dev), "uart_dev is not ready");

	uart_irq_rx_disable(uart_dev);
	uart_irq_tx_disable(uart_dev);

	/* Set RX irq. handler */
	uart_irq_callback_user_data_set(uart_dev, uart_isr, NULL);

	/* Clean RX FIFO before enabling interrupt. */
	while (uart_irq_rx_ready(uart_dev)) {
		uint8_t c;

		uart_fifo_read(uart_dev, &c, 1);
	}

	/* Enable RX interruption. */
	uart_irq_rx_enable(uart_dev);

	return 0;
}
SYS_INIT(uart_isr_init, APPLICATION, 0);

__no_instrumentation__ void instr_transport_init(void)
{
#if defined(CONFIG_INSTRUMENTATION_MODE_CALLGRAPH)
	instr_buffer_init();
#endif
}

__no_instrumentation__ void instr_transport_push_record(struct instr_record *record)
{
#if defined(CONFIG_INSTRUMENTATION_MODE_CALLGRAPH)
	uint32_t total_size = 0U;
	uint8_t *data = (uint8_t *)record, *buf;
	uint32_t length = sizeof(struct instr_record), claimed_size;

	if (!IS_ENABLED(CONFIG_INSTRUMENTATION_MODE_CALLGRAPH_BUFFER_OVERWRITE) &&
	    ring_buf_space_get(instr_buffer_get_ring_buf()) < sizeof(struct instr_record)) {
#ifdef CONFIG_INSTRUMENTATION_MODE_CALLGRAPH_DUMP_ON_FULL
		instr_transport_cmd_dump_trace();
		instr_buffer_reset();
#else
		instr_disable();
		return;
#endif
	}

	/* If record won't fit, free enough space in the buffer */
	if (ring_buf_space_get(instr_buffer_get_ring_buf()) < sizeof(struct instr_record)) {
		ring_buf_consume(instr_buffer_get_ring_buf(), sizeof(struct instr_record));
	}

	ring_buf_put(instr_buffer_get_ring_buf(), (uint8_t *)record, sizeof(struct instr_record));

#endif
}

__no_instrumentation__ void instr_transport_cmd_dump_trace(void)
{
#if defined(CONFIG_INSTRUMENTATION_MODE_CALLGRAPH)
	static const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	uint8_t *transferring_buf;
	uint32_t transferring_length;

	/* Make sure instrumentation is disabled. */
	instr_disable();

	/* Initiator mark */
	printk(INSTR_START_TAG);

	while (!ring_buf_is_empty(instr_buffer_get_ring_buf())) {
		transferring_length =
			ring_buf_get_ptr(instr_buffer_get_ring_buf(), &transferring_buf, 0);

		for (uint32_t i = 0; i < transferring_length; i++) {
			uart_poll_out(uart_dev, transferring_buf[i]);
		}

		ring_buf_consume(instr_buffer_get_ring_buf(), transferring_length);
	}

	/* Terminator mark */
	printk(INSTR_END_TAG);
#endif
}

__no_instrumentation__ void instr_transport_cmd_dump_profile(void)
{
	instr_dump_deltas();
}

#if defined(CONFIG_INSTRUMENTATION_MODE_STATISTICAL)
__no_instrumentation__ void instr_transport_send_stats(struct disco_func_entry *stats, int count)
{
	static const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	instr_disable();

	/* Initiator mark */
	printk(INSTR_START_TAG);

	for (int i = 0; i < count; i++) {
		uart_poll_out(uart_dev, INSTR_EVENT_PROFILE);
		for (int j = 0; j < sizeof(stats[i].addr); j++) {
			uart_poll_out(uart_dev, *((uint8_t *)&stats[i].addr + j));
		}
		for (int k = 0; k < sizeof(stats[i].delta_t); k++) {
			uart_poll_out(uart_dev, *((uint8_t *)&stats[i].delta_t + k));
		}
	}

	/* Terminator mark */
	printk(INSTR_END_TAG);
}
#endif
