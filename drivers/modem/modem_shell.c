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

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/console/uart_mux.h>

#include <zephyr/sys/printk.h>

struct modem_shell_user_data {
	const struct shell *sh;
	void *user_data;
};

#if defined(CONFIG_MODEM_CONTEXT)
#include "modem_context.h"
#define ms_context		modem_context
#define ms_max_context		CONFIG_MODEM_CONTEXT_MAX_NUM
#define ms_send(ctx_, buf_, size_) \
			(ctx_->iface.write(&ctx_->iface, buf_, size_))
#define ms_context_from_id	modem_context_from_id
#define UART_DEV_NAME(ctx)	(ctx->iface.dev->name)
#elif defined(CONFIG_MODEM_RECEIVER)
#include "modem_receiver.h"
#define ms_context		mdm_receiver_context
#define ms_max_context		CONFIG_MODEM_RECEIVER_MAX_CONTEXTS
#define ms_send			mdm_receiver_send
#define ms_context_from_id	mdm_receiver_context_from_id
#define UART_DEV_NAME(ctx_)	(ctx_->uart_dev->name)
#else
#error "MODEM_CONTEXT or MODEM_RECEIVER need to be enabled"
#endif

static int cmd_modem_list(const struct shell *sh, size_t argc,
			  char *argv[])
{
	struct ms_context *mdm_ctx;
	int i, count = 0;

	shell_fprintf(sh, SHELL_NORMAL, "Modem receivers:\n");

	for (i = 0; i < ms_max_context; i++) {
		mdm_ctx = ms_context_from_id(i);
		if (mdm_ctx) {
			count++;
			shell_fprintf(sh, SHELL_NORMAL,
			     "%d:\tIface Device: %s\n"
				"\tManufacturer: %s\n"
				"\tModel:        %s\n"
				"\tRevision:     %s\n"
				"\tIMEI:         %s\n"
#if defined(CONFIG_MODEM_SIM_NUMBERS)
				"\tIMSI:         %s\n"
				"\tICCID:        %s\n"
#endif
#if defined(CONFIG_MODEM_CELL_INFO)
				"\tOperator:     %d\n"
				"\tLAC:          %d\n"
				"\tCellId:       %d\n"
				"\tAcT:          %d\n"
#endif
				"\tRSSI:         %d\n",
			       i,
			       UART_DEV_NAME(mdm_ctx),
			       mdm_ctx->data_manufacturer,
			       mdm_ctx->data_model,
			       mdm_ctx->data_revision,
			       mdm_ctx->data_imei,
#if defined(CONFIG_MODEM_SIM_NUMBERS)
			       mdm_ctx->data_imsi,
			       mdm_ctx->data_iccid,
#endif
#if defined(CONFIG_MODEM_CELL_INFO)
			       mdm_ctx->data_operator,
			       mdm_ctx->data_lac,
			       mdm_ctx->data_cellid,
			       mdm_ctx->data_act,
#endif
			       mdm_ctx->data_rssi ? *mdm_ctx->data_rssi : 0);
		}
	}

	if (!count) {
		shell_fprintf(sh, SHELL_NORMAL, "None found.\n");
	}

	return 0;
}

static int cmd_modem_send(const struct shell *sh, size_t argc,
			  char *argv[])
{
	struct ms_context *mdm_ctx;
	char *endptr;
	int ret, i, arg = 1;

	/* list */
	if (!argv[arg]) {
		shell_fprintf(sh, SHELL_ERROR,
			      "Please enter a modem index\n");
		return -EINVAL;
	}

	/* <index> of modem receiver */
	i = (int)strtol(argv[arg], &endptr, 10);
	if (*endptr != '\0') {
		shell_fprintf(sh, SHELL_ERROR,
			      "Please enter a modem index\n");
		return -EINVAL;
	}

	mdm_ctx = ms_context_from_id(i);
	if (!mdm_ctx) {
		shell_fprintf(sh, SHELL_ERROR, "Modem receiver not found!");
		return 0;
	}

	for (i = arg + 1; i < argc; i++) {
		ret = ms_send(mdm_ctx, argv[i], strlen(argv[i]));
		if (ret < 0) {
			shell_fprintf(sh, SHELL_ERROR,
				      "Error sending '%s': %d\n", argv[i], ret);
			return 0;
		}

		if (i == argc - 1) {
			ret = ms_send(mdm_ctx, "\r", 1);
		} else {
			ret = ms_send(mdm_ctx, " ", 1);
		}

		if (ret < 0) {
			shell_fprintf(sh, SHELL_ERROR,
				      "Error sending (CRLF or space): %d\n",
				      ret);
			return 0;
		}
	}

	return 0;
}

#if defined(CONFIG_GSM_MUX)
static void uart_mux_cb(const struct device *uart, const struct device *dev,
			int dlci_address, void *user_data)
{
	struct modem_shell_user_data *data = user_data;
	const struct shell *sh = data->shell;
	int *count = data->user_data;
	const char *ch = "?";

	if (*count == 0) {
		shell_fprintf(sh, SHELL_NORMAL,
			      "\nReal UART\tMUX UART\tDLCI\n");
	}

	(*count)++;

	if (dlci_address == CONFIG_GSM_MUX_DLCI_AT) {
		ch = "AT";
	} else if (dlci_address == CONFIG_GSM_MUX_DLCI_PPP) {
		ch = "PPP";
	} else if (dlci_address == 0) {
		ch = "control";
	}

	shell_fprintf(sh, SHELL_NORMAL,
		      "%s\t\t%s\t\t%d (%s)\n",
		      uart->name, dev->name, dlci_address, ch);
}
#endif

static int cmd_modem_info(const struct shell *sh, size_t argc, char *argv[])
{
	struct ms_context *mdm_ctx;
	char *endptr;
	int i, arg = 1;

	/* info */
	if (!argv[arg]) {
		shell_fprintf(sh, SHELL_ERROR,
			      "Please enter a modem index\n");
		return -EINVAL;
	}

	/* <index> of modem receiver */
	i = (int)strtol(argv[arg], &endptr, 10);
	if (*endptr != '\0') {
		shell_fprintf(sh, SHELL_ERROR,
			      "Please enter a modem index\n");
		return -EINVAL;
	}

	mdm_ctx = ms_context_from_id(i);
	if (!mdm_ctx) {
		shell_fprintf(sh, SHELL_ERROR, "Modem receiver not found!");
		return 0;
	}

	shell_fprintf(sh, SHELL_NORMAL,
		      "Modem index      : %d\n"
		      "Iface Device     : %s\n"
		      "Manufacturer     : %s\n"
		      "Model            : %s\n"
		      "Revision         : %s\n"
		      "IMEI             : %s\n"
		      "RSSI             : %d\n",
		      i,
		      UART_DEV_NAME(mdm_ctx),
		      mdm_ctx->data_manufacturer,
		      mdm_ctx->data_model,
		      mdm_ctx->data_revision,
		      mdm_ctx->data_imei,
		      mdm_ctx->data_rssi ? *mdm_ctx->data_rssi : 0);

	shell_fprintf(sh, SHELL_NORMAL,
		      "GSM 07.10 muxing : %s\n",
		      IS_ENABLED(CONFIG_GSM_MUX) ? "enabled" : "disabled");

#if defined(CONFIG_GSM_MUX)
	struct modem_shell_user_data user_data;
	int count = 0;

	user_data.shell = sh;
	user_data.user_data = &count;

	uart_mux_foreach(uart_mux_cb, &user_data);
#endif

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_modem,
	SHELL_CMD(info, NULL, "Show information for a modem", cmd_modem_info),
	SHELL_CMD(list, NULL, "List registered modems", cmd_modem_list),
	SHELL_CMD(send, NULL, "Send an AT <command> to a registered modem "
			      "receiver", cmd_modem_send),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(modem, &sub_modem, "Modem commands", NULL);
