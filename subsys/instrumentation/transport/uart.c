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

#define COMMAND_BUFFER_SIZE 32
char _cmd_buffer[COMMAND_BUFFER_SIZE];

__no_instrumentation__
void handle_cmd(char *cmd)
{
	char *beginptr;
	char *endptr;
	long address;

	if (strcmp("reboot", cmd) == 0) {
		sys_reboot(SYS_REBOOT_COLD);
	} else if (strcmp("status", cmd) == 0) {
		printk("%d %d\n", instr_tracing_supported(), instr_profiling_supported());
	} else if (strcmp("ping", cmd) == 0) {
		printk("pong\n");
	} else if (strcmp("dump_trace", cmd) == 0) {
		instr_dump_buffer_uart();
	} else if (strcmp("dump_profile", cmd) == 0) {
		instr_dump_deltas_uart();
	} else if (strncmp(cmd, "trigger", strlen("trigger")) == 0) {
		beginptr = cmd + strlen("trigger");
		address = strtol(beginptr, &endptr, 16);
		if (endptr != beginptr) {
			instr_set_trigger_func((void *)address);
		} else {
			printk("invalid argument for 'trigger' in: '%s'\n", cmd);
		}
	} else if (strncmp(cmd, "stopper", strlen("stopper")) == 0) {
		beginptr = cmd + strlen("stopper");
		address = strtol(beginptr, &endptr, 16);
		if (endptr != beginptr) {
			instr_set_stop_func((void *)address);
		} else {
			printk("invalid argument for 'stopper' in: '%s'\n", cmd);
		}
	} else if (strcmp("listsets", cmd) == 0) {
		void *address;

		address = instr_get_trigger_func();
		if (address) {
			printk("trigger: %p\n", address);
		} else {
			printk("trigger not set.\n");
		}

		address = instr_get_stop_func();
		if (address) {
			printk("stopper: %p\n", address);
		} else {
			printk("stopper not set.\n");
		}
	} else {
		printk("invalid command %s\n", cmd);
	}
}

__no_instrumentation__
static void uart_isr(const struct device *dev, void *user_data)
{
	int ret;
	uint8_t byte = 0;
	static uint8_t cur;

	ARG_UNUSED(user_data);

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (!uart_irq_rx_ready(dev)) {
			continue;
		}

		ret = uart_fifo_read(dev, &byte, 1);
		if (ret < 0) {
			uart_irq_rx_disable(dev);
			return;
		}

		if (!isprint(byte)) {
			if (byte == '\r') {
				_cmd_buffer[cur] = '\0';
				handle_cmd(_cmd_buffer);
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
static int uart_isr_init(void)
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
