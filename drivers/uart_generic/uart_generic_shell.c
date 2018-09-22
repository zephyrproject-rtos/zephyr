/** @file
 * @brief Modem shell module
 *
 * Provide some modem shell commands that can be useful to applications.
 */

/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stdlib.h>
#include <string.h>
#include <device.h>
#include <shell/shell.h>

#include <misc/printk.h>

#include <drivers/generic_uart/generic_uart_drv.h>

#define UART_SHELL_MODULE "uart"
int uart_shell_selected_device = 0;

int uart_shell_cmd_list(int argc, char *argv[])
{
	struct uart_drv_context *drv_ctx;
	int i, count = 0;

	printk("Lora devices:\n");

	for (i = 0; i < MAX_UART_DRV_CTX; i++) {
		drv_ctx = uart_drv_context_from_id(i);
		if (drv_ctx) {
			count++;
			printk("%d:\tUART Name:    %s\n", i,

			       drv_ctx->uart_dev->config->name);
			     //  drv_ctx->data_manufacturer,
			     //  drv_ctx->data_model,
			     //  drv_ctx->data_revision,
			     //  drv_ctx->data_imei,
			     //  drv_ctx->data_rssi);
		}
	}

	if (!count) {
		printk("None found.\n");
	}

	return 0;
}

int uart_shell_cmd_select(int argc, char *argv[]) {

	struct uart_drv_context *drv_ctx;
	char *endptr;
	int ret, i, arg = 1;

	if (!argv[arg]) {
		printk("Please enter a device index\n");
		return -EINVAL;
	}


    /* <index> of device */
    i = (int)strtol(argv[arg], &endptr, 10);
    if (*endptr != '\0') {
        printk("Please enter a device index\n");
        return -EINVAL;
    }


    drv_ctx = uart_drv_context_from_id(i);
	if (!drv_ctx) {
		printk("Device not found!");
		return 0;
	}


	uart_shell_selected_device = i;
	return 0;
}

int uart_shell_cmd_send(int argc, char *argv[])
{

	struct uart_drv_context *drv_ctx;
	char *endptr;
	int ret, i, arg = 1;

	drv_ctx = uart_drv_context_from_id(uart_shell_selected_device);
	if (!drv_ctx) {
		printk("Device not found!\n");
		return 0;
	}

	char uart_command[128];
	size_t uart_command_len = 0;
	memset(uart_command, 0, 128);

    for (i = arg; i < argc; i++) {
        size_t frag_len = strlen(argv[i]);
        memcpy(&uart_command[uart_command_len], argv[i], frag_len);
        uart_command_len += frag_len;

        if (i == argc - 1) {
            // TODO
            uart_command[uart_command_len] = '\r';
            uart_command_len++;
            uart_command[uart_command_len] = '\n';
            uart_command_len++;
        } else {
            uart_command[uart_command_len] = ' ';
            uart_command_len++;
        }
    }
    uart_drv_send(drv_ctx, uart_command, uart_command_len);

	return 0;
}


static struct shell_cmd uart_commands[] = {
	/* Keep the commands in alphabetical order */
    { "device", uart_shell_cmd_select, "\n\tSelect device at <index>" },
	{ "list", uart_shell_cmd_list, "\n\tList registered devices" },
	{ "send", uart_shell_cmd_send,
		"\n\tSend an AT <command> to selected device:"
		"\n\tsend <command>" },
	{ NULL, NULL, NULL }
};

SHELL_REGISTER(UART_SHELL_MODULE, uart_commands);
