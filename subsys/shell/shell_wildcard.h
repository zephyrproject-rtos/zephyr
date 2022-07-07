/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_SHELL_WILDCARDS_H__
#define SHELL_SHELL_WILDCARDS_H__

#include <zephyr/shell/shell.h>

enum shell_wildcard_status {
	SHELL_WILDCARD_CMD_ADDED,
	SHELL_WILDCARD_CMD_MISSING_SPACE,
	SHELL_WILDCARD_CMD_NO_MATCH_FOUND, /* no matching command */
	SHELL_WILDCARD_NOT_FOUND /* wildcard character not found */
};

/* Function initializing wildcard expansion procedure.
 *
 * @param[in] shell	Pointer to the shell instance.
 */
void z_shell_wildcard_prepare(const struct shell *shell);

/* Function returns true if string contains wildcard character: '?' or '*'
 *
 * @param[in] str	Pointer to string.
 *
 * @retval true		wildcard character found
 * @retval false	wildcard character not found
 */
bool z_shell_has_wildcard(const char *str);

/* Function expands wildcards in the shell temporary buffer
 *
 * @brief Function evaluates one command. If command contains wildcard patter,
 * function will expand this command with all commands matching wildcard
 * pattern.
 *
 * Function will print a help string with: the currently entered command, its
 * options,and subcommands (if they exist).
 *
 * @param[in] shell	Pointer to the shell instance.
 * @param[in] cmd	Pointer to command which will be processed.
 * @param[in] pattern	Pointer to wildcard pattern.
 */
enum shell_wildcard_status z_shell_wildcard_process(const struct shell *shell,
					const struct shell_static_entry *cmd,
					const char *pattern);

/* Function finalizing wildcard expansion procedure.
 *
 * @param[in] shell	Pointer to the shell instance.
 */
void z_shell_wildcard_finalize(const struct shell *shell);


#endif /* SHELL_SHELL_WILDCARDS_H__ */
