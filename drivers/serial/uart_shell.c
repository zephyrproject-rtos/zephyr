/*
 * Copyright (c) 2024 Yishai Jaffe <yishai1999@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_shell, CONFIG_LOG_DEFAULT_LEVEL);

static bool device_is_uart(const struct device *dev)
{
	return DEVICE_API_IS(uart, dev);
}

static int cmd_uart_write(const struct shell *sh, size_t argc, char **argv)
{
	char *s_dev_name = argv[1];
	const struct device *dev = shell_device_get_binding(s_dev_name);

	if (!dev || !device_is_uart(dev)) {
		shell_error(sh, "UART: Device driver %s not found.", s_dev_name);
		return -ENODEV;
	}

	char *buf = argv[2];
	int msg_len = strlen(buf);

	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(dev, buf[i]);
	}

	return 0;
}

static int cmd_uart_baudrate(const struct shell *sh, size_t argc, char **argv)
{
	char *s_dev_name = argv[1];
	const struct device *dev;
	struct uart_config cfg;
	uint32_t baudrate;
	int ret;

	dev = shell_device_get_binding(s_dev_name);
	if (!dev || !device_is_uart(dev)) {
		shell_error(sh, "UART: Device driver %s not found.", s_dev_name);
		return -ENODEV;
	}

	baudrate = strtol(argv[2], NULL, 10);
	ret = uart_config_get(dev, &cfg);
	if (ret < 0) {
		shell_error(sh, "UART: Failed to get current configuration: %d", ret);
		return ret;
	}
	cfg.baudrate = baudrate;

	ret = uart_configure(dev, &cfg);
	if (ret < 0) {
		shell_error(sh, "UART: Failed to configure device: %d", ret);
		return ret;
	}
	return 0;
}

static int cmd_uart_flow_control(const struct shell *sh, size_t argc, char **argv)
{
	char *s_dev_name = argv[1];
	const struct device *dev;
	struct uart_config cfg;
	uint8_t flow_control;
	int ret;

	dev = shell_device_get_binding(s_dev_name);
	if (!dev || !device_is_uart(dev)) {
		shell_error(sh, "UART: Device driver %s not found.", s_dev_name);
		return -ENODEV;
	}

	if (!strcmp(argv[2], "none")) {
		flow_control = UART_CFG_FLOW_CTRL_NONE;
	} else if (!strcmp(argv[2], "rtscts")) {
		flow_control = UART_CFG_FLOW_CTRL_RTS_CTS;
	} else if (!strcmp(argv[2], "dtrdsr")) {
		flow_control = UART_CFG_FLOW_CTRL_DTR_DSR;
	} else if (!strcmp(argv[2], "rs485")) {
		flow_control = UART_CFG_FLOW_CTRL_RS485;
	} else {
		shell_error(sh, "Unknown: '%s'", argv[2]);
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	ret = uart_config_get(dev, &cfg);
	if (ret < 0) {
		shell_error(sh, "UART: Failed to get current configuration: %d", ret);
		return ret;
	}
	cfg.flow_ctrl = flow_control;

	ret = uart_configure(dev, &cfg);
	if (ret < 0) {
		shell_error(sh, "UART: Failed to configure device: %d", ret);
		return ret;
	}
	return 0;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_uart);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_uart_cmds,
	SHELL_CMD_ARG(write, &dsub_device_name,
		      "Write data to the UART device\n"
		      "Usage: write <device> <data>",
		      cmd_uart_write, 3, 0),
	SHELL_CMD_ARG(baudrate, &dsub_device_name,
		      "Configure UART device baudrate\n"
		      "Usage: baudrate <device> <baudrate>",
		      cmd_uart_baudrate, 3, 0),
	SHELL_CMD_ARG(fc, &dsub_device_name,
		      "Configure UART device flow control\n"
		      "Usage: fc <device> <none|rtscts|dtrdsr|rs485>",
		      cmd_uart_flow_control, 3, 0),
	SHELL_SUBCMD_SET_END     /* Array terminated. */
);

SHELL_CMD_REGISTER(uart, &sub_uart_cmds, "UART commands", NULL);
