/*
 * Copyright (c) 2019,2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <ctype.h>
#include <zephyr/shell/shell.h>

#if CONFIG_PSA_SHELL

static int
psa_shell_invalid_arg(const struct shell *sh, char *arg_name)
{
	shell_print(sh, "Error: invalid argument \"%s\"\n", arg_name);

	return -EINVAL;
}

static int
psa_shell_cmd_version(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "%s", "0.0.0");

	return 0;
}

/* Subcommand array for "psa" (level 1). */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_psa,
	/* 'version' command handler. */
	SHELL_CMD(version, NULL, "app version", psa_shell_cmd_version),
	/* Array terminator. */
	SHELL_SUBCMD_SET_END
);

/* Root command "psa" (level 0). */
SHELL_CMD_REGISTER(psa, &sub_psa, "PSA commands", NULL);

#endif
