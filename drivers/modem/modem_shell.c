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

#include <drivers/modem/modem_receiver.h>

#define MODEM_SHELL_MODULE "modem"

int modem_shell_cmd_list(int argc, char *argv[])
{
	struct mdm_receiver_context *mdm_ctx;
	int i, count = 0;

	printk("Modem receivers:\n");

	for (i = 0; i < CONFIG_MODEM_RECEIVER_MAX_CONTEXTS; i++) {
		mdm_ctx = mdm_receiver_context_from_id(i);
		if (mdm_ctx) {
			count++;
			printk("%d:\tUART Name:    %s\n"
				"\tManufacturer: %s\n"
				"\tModel:        %s\n"
				"\tRevision:     %s\n"
				"\tIMEI:         %s\n"
				"\tRSSI:         %d\n", i,
			       mdm_ctx->uart_dev->config->name,
			       mdm_ctx->data_manufacturer,
			       mdm_ctx->data_model,
			       mdm_ctx->data_revision,
			       mdm_ctx->data_imei,
			       mdm_ctx->data_rssi);
		}
	}

	if (!count) {
		printk("None found.\n");
	}

	return 0;
}

int modem_shell_cmd_send(int argc, char *argv[])
{
	struct mdm_receiver_context *mdm_ctx;
	char *endptr;
	int ret, i, arg = 1;

	/* list */
	if (!argv[arg]) {
		printk("Please enter a modem index\n");
		return -EINVAL;
	}

	/* <index> of modem receiver */
	i = (int)strtol(argv[arg], &endptr, 10);
	if (*endptr != '\0') {
		printk("Please enter a modem index\n");
		return -EINVAL;
	}

	mdm_ctx = mdm_receiver_context_from_id(i);
	if (!mdm_ctx) {
		printk("Modem receiver not found!");
		return 0;
	}

	for (i = arg + 1; i < argc; i++) {
		ret = mdm_receiver_send(mdm_ctx, argv[i], strlen(argv[i]));
		if (ret < 0) {
			printk("Error sending '%s': %d\n", argv[i], ret);
			return 0;
		}

		if (i == argc - 1) {
			ret = mdm_receiver_send(mdm_ctx, "\r\n", 2);
		} else {
			ret = mdm_receiver_send(mdm_ctx, " ", 1);
		}

		if (ret < 0) {
			printk("Error sending (CRLF or space): %d\n", ret);
			return 0;
		}
	}

	return 0;
}

static struct shell_cmd modem_commands[] = {
	/* Keep the commands in alphabetical order */
	{ "list", modem_shell_cmd_list, "\n\tList registered modems" },
	{ "send", modem_shell_cmd_send,
		"\n\tSend an AT <command> to a registered modem receiver:"
		"\n\tsend <index> <command>" },
	{ NULL, NULL, NULL }
};

SHELL_REGISTER(MODEM_SHELL_MODULE, modem_commands);
