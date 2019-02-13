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

#define LOG_MODULE_NAME modem_shell

#include <zephyr.h>
#include <stdlib.h>
#include <string.h>
#include <device.h>
#include <shell/shell.h>

#include <misc/printk.h>

#include <drivers/modem/modem_receiver.h>

static int cmd_modem_list(const struct shell *shell, size_t argc,
			  char *argv[])
{
	struct mdm_receiver_context *mdm_ctx;
	int i, count = 0;

	shell_fprintf(shell, SHELL_NORMAL, "Modem receivers:\n");

	for (i = 0; i < CONFIG_MODEM_RECEIVER_MAX_CONTEXTS; i++) {
		mdm_ctx = mdm_receiver_context_from_id(i);
		if (mdm_ctx) {
			count++;
			shell_fprintf(shell, SHELL_NORMAL,
				"%d:\tUART Name:    %s\n"
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
		shell_fprintf(shell, SHELL_NORMAL, "None found.\n");
	}

	return 0;
}

static int cmd_modem_send(const struct shell *shell, size_t argc,
			  char *argv[])
{
	struct mdm_receiver_context *mdm_ctx;
	char *endptr;
	int ret, i, arg = 1;

	/* list */
	if (!argv[arg]) {
		shell_fprintf(shell, SHELL_ERROR,
			      "Please enter a modem index\n");
		return -EINVAL;
	}

	/* <index> of modem receiver */
	i = (int)strtol(argv[arg], &endptr, 10);
	if (*endptr != '\0') {
		shell_fprintf(shell, SHELL_ERROR,
			      "Please enter a modem index\n");
		return -EINVAL;
	}

	mdm_ctx = mdm_receiver_context_from_id(i);
	if (!mdm_ctx) {
		shell_fprintf(shell, SHELL_ERROR, "Modem receiver not found!");
		return 0;
	}

	for (i = arg + 1; i < argc; i++) {
		ret = mdm_receiver_send(mdm_ctx, argv[i], strlen(argv[i]));
		if (ret < 0) {
			shell_fprintf(shell, SHELL_ERROR,
				      "Error sending '%s': %d\n", argv[i], ret);
			return 0;
		}

		if (i == argc - 1) {
			ret = mdm_receiver_send(mdm_ctx, "\r\n", 2);
		} else {
			ret = mdm_receiver_send(mdm_ctx, " ", 1);
		}

		if (ret < 0) {
			shell_fprintf(shell, SHELL_ERROR,
				      "Error sending (CRLF or space): %d\n",
				      ret);
			return 0;
		}
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_modem,
	SHELL_CMD(list, NULL, "List registered modems", cmd_modem_list),
	SHELL_CMD(send, NULL, "Send an AT <command> to a registered modem "
			      "receiver", cmd_modem_send),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(modem, &sub_modem, "Modem commands", NULL);
