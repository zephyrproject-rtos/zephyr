/*
 * Copyright 2023 Linaro
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
#include <instr_backend.h>

#define COMMAND_BUFFER_SIZE 32
char _cmd_buffer[COMMAND_BUFFER_SIZE];

static const struct device *const instr_uart_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

__no_instrumentation__
void handle_cmd(char *cmd, uint32_t length)
{
	char *beginptr;
	char *endptr;
	long address;

	if (strncmp("reboot", cmd, length) == 0) {
		sys_reboot(SYS_REBOOT_COLD);
	} else if (strncmp("status", cmd, length) == 0) {
		printk("%d %d %d\n", instr_tracing_supported(), instr_profiling_supported(),
		       instr_dynamic_trigger_supported());
	} else if (strncmp("ping", cmd, length) == 0) {
		printk("pong\n");
	} else if (strncmp("dump_trace", cmd, length) == 0) {
		instr_dump_buffer();
	} else if (strncmp("dump_profile", cmd, length) == 0) {
		instr_dump_deltas();
	} else if (strncmp(cmd, "trigger", strlen("trigger")) == 0) {
		beginptr = cmd + strlen("trigger");
		address = strtol(beginptr, &endptr, 16);
		if (endptr != beginptr) {
			instr_set_trigger_func((void *)address);
		} else {
			printk("trigger: invalid argument in: '%s'\n", cmd);
		}
	} else if (strncmp(cmd, "stopper", strlen("stopper")) == 0) {
		beginptr = cmd + strlen("stopper");
		address = strtol(beginptr, &endptr, 16);
		if (endptr != beginptr) {
			instr_set_stop_func((void *)address);
		} else {
			printk("stopper: invalid argument in: '%s'\n", cmd);
		}
	} else if (strncmp("listsets", cmd, length) == 0) {
		void *address;

		address = instr_get_trigger_func();
		if (address) {
			printk("trigger: %p\n", address);
		} else {
			printk("trigger: not set.\n");
		}

		address = instr_get_stop_func();
		if (address) {
			printk("stopper: %p\n", address);
		} else {
			printk("stopper: not set.\n");
		}
	} else {
		printk("invalid command %s\n", cmd);
	}
}

__no_instrumentation__
static void uart_isr(const struct device *uart_dev, void *user_data)
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
				handle_cmd(_cmd_buffer, cur);
				cur = 0U;
			}

			continue;
		}

		if (cur < (COMMAND_BUFFER_SIZE - 1)) {
			_cmd_buffer[cur++] = byte;
		}
	}
}

__no_instrumentation__
static void instr_backend_uart_output(const struct instr_backend *backend, uint8_t *data,
				      uint32_t length)
{
	ARG_UNUSED(backend);

	for (uint32_t i = 0; i < length; i++) {
		uart_poll_out(instr_uart_dev, data[i]);
	}
}

static const struct instr_backend_api instr_backend_uart_api = {
	.output = instr_backend_uart_output,
};

INSTR_BACKEND_DEFINE(instr_backend_uart, instr_backend_uart_api);

/*
 * Host-command ISR setup must wait until the console UART is ready.
 * Data egress only needs the device at dump time.
 */
__no_instrumentation__
static int instr_uart_cmd_init(void)
{
	__ASSERT(device_is_ready(instr_uart_dev), "uart_dev is not ready");

	uart_irq_rx_disable(instr_uart_dev);
	uart_irq_tx_disable(instr_uart_dev);

	uart_irq_callback_user_data_set(instr_uart_dev, uart_isr, NULL);

	while (uart_irq_rx_ready(instr_uart_dev)) {
		uint8_t c;

		uart_fifo_read(instr_uart_dev, &c, 1);
	}

	uart_irq_rx_enable(instr_uart_dev);

	return 0;
}

SYS_INIT(instr_uart_cmd_init, APPLICATION, 0);
