/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
LOG_MODULE_REGISTER(app_test);

void foo(void)
{
	LOG_INF("info message");
	LOG_WRN("warning message");
	LOG_ERR("err message");
}

/* Commands below are added using memory section approach which allows to build
 * a set of subcommands from multiple files.
 */
static int cmd2_handler(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "cmd2 executed");

	return 0;
}

SHELL_SUBCMD_ADD((section_cmd), cmd2, NULL, "help for cmd2", cmd2_handler, 1, 0);

static int sub_cmd1_handler(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "sub cmd1 executed");

	return 0;
}

SHELL_SUBCMD_COND_ADD(1, (section_cmd, cmd1), sub_cmd1, NULL, "help for cmd2",
			sub_cmd1_handler, 1, 0);
