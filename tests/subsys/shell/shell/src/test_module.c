/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/shell/shell.h>

static int cmd2_handler(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return 20;
}

SHELL_SUBCMD_ADD((section_cmd), cmd2, NULL, "help for cmd2", cmd2_handler, 1, 0);

static int sub_cmd1_handler(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return 11;
}

SHELL_SUBCMD_COND_ADD(1, (section_cmd, cmd1), sub_cmd1, NULL,
		      "help for sub cmd1", sub_cmd1_handler, 1, 0);

static int sub_cmd2_handler(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return 12;
}

/* Flag set to 0, command will not be compiled. Add to validate that setting
 * flag to 0 does not add command and does not generate any warnings.
 */
SHELL_SUBCMD_COND_ADD(0, (section_cmd, cmd1), sub_cmd2, NULL,
		      "help for sub cmd2", sub_cmd2_handler, 1, 0);
