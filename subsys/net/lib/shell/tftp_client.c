/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/tftp.h>
#include <zephyr/sys/byteorder.h>

#include "net_shell_private.h"

#define DEFAULT_TFTP_PORT 69

#if defined(CONFIG_TFTP_LIB)
static const struct shell *tftp_shell;

struct tftp_client_shell_ctx {
	struct tftpc client;
	uint8_t *file_name;
	uint8_t *data;
	uint8_t *mode;
};

static void tftp_client_event_cb(const struct tftp_evt *evt)
{
	switch (evt->type) {
	case TFTP_EVT_DATA:
		shell_hexdump(tftp_shell, evt->param.data.data_ptr, evt->param.data.len);
		break;
	case TFTP_EVT_ERROR:
		shell_error(tftp_shell, "Error code %d msg: %s", evt->param.error.code,
			    evt->param.error.msg);
		break;
	default:
		break;
	}
}

static int tftp_client_args_to_params(const struct shell *sh, struct tftp_client_shell_ctx *ctx,
				      size_t argc, char *argv[])
{
	struct sys_getopt_state *state;
	uint8_t default_mode[6] = "octet";
	int opt_index = 0;
	int opt;

	static const struct sys_getopt_option long_opts[] = {
		{"server", sys_getopt_required_argument, 0, 's'},
		{"file", sys_getopt_required_argument, 0, 'f'},
		{"mode", sys_getopt_required_argument, 0, 'm'},
		{"data", sys_getopt_required_argument, 0, 'd'},
		{0, 0, 0, 0}};

	/* Defaults */
	ctx->client.callback = tftp_client_event_cb;
	ctx->mode = default_mode;
	ctx->data = NULL;

	while ((opt = sys_getopt_long(argc, argv, "s:f:m:d:", long_opts, &opt_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 's': {
			memset(&ctx->client.server_addr, 0, sizeof(struct net_sockaddr_storage));
			if (!net_ipaddr_parse(state->optarg, strlen(state->optarg),
					      &ctx->client.server)) {
				PR_ERROR("Invalid server address: %s\n", state->optarg);
				return -EINVAL;
			}
			break;
		}
		case 'f':
			ctx->file_name = (uint8_t *)state->optarg;
			break;
		case 'm':
			ctx->mode = (uint8_t *)state->optarg;
			break;
		case 'd':
			ctx->data = (uint8_t *)state->optarg;
			break;
		case '?':
		default:
			PR_ERROR("Invalid option or option usage: %s\n", argv[opt_index + 1]);
			return -ENOEXEC;
		}
	}

	if (net_sin(&ctx->client.server)->sin_port == 0) {
		net_sin(&ctx->client.server)->sin_port = sys_cpu_to_be16(DEFAULT_TFTP_PORT);
	}

	return 0;
}
#endif

static int cmd_tftp_get(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_TFTP_LIB)
	struct tftp_client_shell_ctx ctx;
	int ret;

	tftp_shell = sh;
	ret = tftp_client_args_to_params(sh, &ctx, argc, argv);
	if (ret < 0) {
		return ret;
	}

	if (ctx.file_name == NULL) {
		PR_ERROR("Invalid argument(s)\n");
		return -EINVAL;
	}

	ret = tftp_get(&ctx.client, ctx.file_name, ctx.mode);
	if (ret < 0) {
		PR_ERROR("Error while getting file (%d)\n", ret);
		return ret;
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_TFTP_LIB", "TFTP client");
#endif
	return 0;
}

static int cmd_tftp_put(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_TFTP_LIB)
	struct tftp_client_shell_ctx ctx;
	int ret;

	tftp_shell = sh;
	ret = tftp_client_args_to_params(sh, &ctx, argc, argv);
	if (ret < 0) {
		return ret;
	}

	if (ctx.file_name == NULL || ctx.data == NULL) {
		PR_ERROR("Invalid argument(s)\n");
		return -EINVAL;
	}

	ret = tftp_put(&ctx.client, ctx.file_name, ctx.mode, ctx.data, strlen(ctx.data));
	if (ret < 0) {
		PR_ERROR("Error while putting file (%d)\n", ret);
		return ret;
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_TFTP_LIB", "TFTP client");
#endif
	return 0;
}

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(tftp_cmds,
	SHELL_CMD_ARG(get, NULL,
		      SHELL_HELP("Read file content.",
			"-s | --server <address>[:<port>]\n"
			"-f | --file <file_name>\n"
			"[-m | --mode <mode>]"),
			cmd_tftp_get, 5, 2),
	SHELL_CMD_ARG(put, NULL,
		      SHELL_HELP("Write to a file.",
			"-s | --server <address>[:<port>]\n"
			"-f | --file <file_name>\n"
			"-d | --data <data>\n"
			"[-m | --mode <mode>]"),
			cmd_tftp_put, 7, 2),
	SHELL_SUBCMD_SET_END);
/* clang-format on */

SHELL_SUBCMD_ADD((net), tftp, &tftp_cmds, "TFTP client commands", NULL, 1, 0);
