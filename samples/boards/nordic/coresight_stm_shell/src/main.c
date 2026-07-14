/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_remote_common.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

static int cmd_ping(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	LOG_ERR("error %d", 100);
	LOG_WRN("warning %lld", 0x1234567890LL);
	LOG_INF("info %s", "test");
	LOG_DBG("debug %d %d", 1000, 100);
	shell_print(sh, "pong %s", CONFIG_BOARD_TARGET);
	return 0;
}

SHELL_CMD_REGISTER(ping, NULL, "Demo command", cmd_ping);

int main(void)
{
#ifndef CONFIG_MULTITHREADING
	/* If multithreading is not enabled, we need to process the shell commands manually. */
	while (1) {
		shell_remote_cmd_process();
		if (LOG_PROCESS() == false) {
			k_cpu_idle();
		}
	}
#endif
	return 0;
}
