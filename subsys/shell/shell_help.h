/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_HELP_H__
#define SHELL_HELP_H__

#include <shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Function is printing command help string. */
void z_shell_help_cmd_print(const struct shell *shell,
			    const struct shell_static_entry *cmd);

/* Function is printing subcommands and help string. */
void z_shell_help_subcmd_print(const struct shell *shell,
			       const struct shell_static_entry *cmd,
			       const char *description);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* SHELL_HELP__ */
