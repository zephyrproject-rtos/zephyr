/* Copyright (C) 2024 Bang & Olufsen A/S, Denmark
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(serial_shell, CONFIG_UART_LOG_LEVEL);

/* both are 1 as they're not used in the same context */
#define ARGV_DEV     1
#define ARGV_TX_DATA 1

K_SEM_DEFINE(busy_sem, 1, 1);

const struct device *enabled_device;
static char rx_buffer[CONFIG_SERIAL_SHELL_RX_BUFFER_SIZE];
static char tx_buffer[CONFIG_SERIAL_SHELL_TX_BUFFER_SIZE];
static uint8_t tx_write_amount;
static uint8_t tx_buffer_index;

static void uart_isr(const struct device *dev, void *user_data)
{
	uart_irq_update(dev);

	if (uart_irq_rx_ready(dev)) {
		uint8_t c;
		uint8_t rx_buffer_index = 0;

		while (1) {
			if (rx_buffer_index >= CONFIG_SERIAL_SHELL_RX_BUFFER_SIZE) {
				LOG_INF("RX[%u]: %.*s", CONFIG_SERIAL_SHELL_RX_BUFFER_SIZE,
					CONFIG_SERIAL_SHELL_RX_BUFFER_SIZE, rx_buffer);
				rx_buffer_index = 0;
			}

			if (uart_fifo_read(dev, &c, 1) == 0) {
				break;
			}

			rx_buffer[rx_buffer_index++] = c;
		}

		LOG_INF("RX[%u]: %.*s", rx_buffer_index, rx_buffer_index, rx_buffer);
	}

	if (uart_irq_tx_ready(dev)) {
		int size = uart_fifo_fill(dev, &tx_buffer[tx_buffer_index++], 1);

		if (size == 0 || (tx_buffer_index >= tx_write_amount)) {
			if (size == 0) {
				LOG_WRN("TX stopped early %u/%u", tx_buffer_index, tx_write_amount);
			}

			uart_irq_tx_disable(dev);
			k_sem_give(&busy_sem);
		}
	}
}

static int cmd_serial_write(const struct shell *sh, size_t argc, char **argv)
{
	const int write_amount = strlen(argv[ARGV_TX_DATA]);

	if (write_amount > CONFIG_SERIAL_SHELL_TX_BUFFER_SIZE) {
		shell_error(sh, "input data (%u) > tx buffer (%u)", write_amount,
			    CONFIG_SERIAL_SHELL_TX_BUFFER_SIZE);
		return -EOVERFLOW;
	}

	k_sem_take(&busy_sem, K_FOREVER);

	if (enabled_device == NULL) {
		shell_error(sh, "use 'serial enable <device>' before writing");
		k_sem_give(&busy_sem);
		return -ENODEV;
	}

	tx_write_amount = (uint8_t)write_amount;
	tx_buffer_index = 0;
	memcpy(tx_buffer, argv[ARGV_TX_DATA], write_amount);

	uart_irq_tx_enable(enabled_device);

	return 0;
}

static int cmd_serial_enable(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[ARGV_DEV]);

	if (!device_is_ready(dev)) {
		shell_error(sh, "%s is not a valid device", argv[ARGV_DEV]);
		return -ENODEV;
	}

	k_sem_take(&busy_sem, K_FOREVER);
	if (enabled_device != NULL) {
		shell_error(sh, "already enabled");
		k_sem_give(&busy_sem);
		return -EBUSY;
	}

	enabled_device = dev;

	uart_irq_callback_set(enabled_device, uart_isr);
	uart_irq_rx_enable(enabled_device);
	k_sem_give(&busy_sem);

	return 0;
}

static int cmd_serial_disable(const struct shell *sh, size_t argc, char **argv)
{
	if (enabled_device == NULL) {
		return 0;
	}

	k_sem_take(&busy_sem, K_FOREVER);
	uart_irq_rx_disable(enabled_device);
	enabled_device = false;
	k_sem_give(&busy_sem);

	return 0;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_serial_cmds,
	SHELL_CMD_ARG(enable, &dsub_device_name,
		      "Enable shell for given uart device\n"
		      "RX data will be logged for given device\n"
		      "TX data will be sent with command 'write' to given device\n"
		      "Usage: enable <device>",
		      cmd_serial_enable, 2, 0),
	SHELL_CMD_ARG(disable, NULL,
		      "Disable shell for given uart device\n"
		      "Usage: disable",
		      cmd_serial_disable, 1, 0),
	SHELL_CMD_ARG(write, NULL,
		      "Write data to the enabled device\n"
		      "Usage: write [<data>]\n\n"
		      "Example 1: serial write uart0 singleword\n"
		      "Example 2: serial write uart0 'multiple words'\n"
		      "NOTE: 'enable' must have been called first",
		      cmd_serial_write, 2, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(serial, &sub_serial_cmds, "Serial commands", NULL);
