/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_remote_cli.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>

/* Messages are checked by the test running on the application core. */
LOG_MODULE_REGISTER(remote, LOG_LEVEL_DBG);

#define EARLY_LOG_CNT 3

/* Runs before IPC is initialized so messages are created before the endpoint is bound. */
static int early_log(void)
{
	for (int i = 0; i < EARLY_LOG_CNT; i++) {
		LOG_INF("early %d/%d", i, EARLY_LOG_CNT);
	}

	return 0;
}

SYS_INIT(early_log, PRE_KERNEL_2, 0);

static int cmd_levels(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	char rw_str[] = "rw_str";
	static const uint8_t data[] = {0xde, 0xad, 0xbe, 0xef, 0x01, 0x02, 0x03, 0x04};

	LOG_ERR("lvl err %d", -1);
	LOG_WRN("lvl wrn %s", "ro_str");
	LOG_INF("lvl inf %s", rw_str);
	LOG_DBG("lvl dbg %d %lld", 100, 0x1234567890LL);
	LOG_HEXDUMP_INF(data, sizeof(data), "lvl hexdump");
	/* Marks that all previous messages were processed. */
	LOG_ERR("lvl end");

	shell_print(sh, "done");
	return 0;
}

static int cmd_burst(const struct shell *sh, size_t argc, char **argv)
{
	int cnt = (int)strtol(argv[1], NULL, 10);

	for (int i = 0; i < cnt; i++) {
		LOG_INF("burst %d", i);
	}

	shell_print(sh, "done");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_log_gen,
	SHELL_CMD_ARG(levels, NULL, "Log a message on each level and a hexdump", cmd_levels, 1, 0),
	SHELL_CMD_ARG(burst, NULL, "<count> Log count messages", cmd_burst, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(log_gen, &sub_log_gen, "Generate log messages", NULL);

int main(void)
{
#ifndef CONFIG_MULTITHREADING
	while (1) {
		shell_remote_cmd_process();
		if (LOG_PROCESS() == false) {
			k_cpu_idle();
		}
	}
#endif
	return 0;
}
