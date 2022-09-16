/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define LOG_MODULE_NAME net_lwm2m_shell
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stddef.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/shell/shell.h>

#define LWM2M_HELP_CMD "LwM2M commands"
#define LWM2M_HELP_SEND "LwM2M SEND operation\nsend [OPTION]... [PATH]...\n" \
	"-n\tSend as non-confirmable\n" \
	"Paths are inserted without leading '/'\n" \
	"Root-level operation is unsupported"

static int cmd_send(const struct shell *sh, size_t argc, char **argv)
{
	int ret = 0;
	struct lwm2m_ctx *ctx = lwm2m_rd_client_ctx();
	int path_cnt = argc - 1;
	bool confirmable = true;
	int ignore_cnt = 1; /* Subcmd + arguments preceding the path list */

	if (!ctx) {
		shell_error(sh, "no lwm2m context yet\n");
		return -ENOEXEC;
	}

	if (argc < 2) {
		shell_error(sh, "no arguments or path(s)\n");
		shell_help(sh);
		return -EINVAL;
	}

	if (strcmp(argv[1], "-n") == 0) {
		confirmable = false;
		path_cnt--;
		ignore_cnt++;
	}

	if ((argc - ignore_cnt) == 0) {
		shell_error(sh, "no path(s)\n");
		shell_help(sh);
		return -EINVAL;
	}

	for (int idx = ignore_cnt; idx < argc; idx++) {
		if (argv[idx][0] < '0' || argv[idx][0] > '9') {
			shell_error(sh, "invalid path: %s\n", argv[idx]);
			shell_help(sh);
			return -EINVAL;
		}
	}

	ret = lwm2m_engine_send(ctx, (const char **)&(argv[ignore_cnt]),
			path_cnt, confirmable);
	if (ret < 0) {
		shell_error(sh, "can't do send operation, request failed\n");
		return -ENOEXEC;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_lwm2m,
			       SHELL_COND_CMD_ARG(CONFIG_LWM2M_VERSION_1_1, send, NULL,
						  LWM2M_HELP_SEND, cmd_send, 1, 9),
			       SHELL_SUBCMD_SET_END);

SHELL_COND_CMD_ARG_REGISTER(CONFIG_LWM2M_SHELL, lwm2m, &sub_lwm2m, LWM2M_HELP_CMD, NULL, 1, 0);
