/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_SHELL_WILDCARDS_H__
#define SHELL_SHELL_WILDCARDS_H__

#include <shell/shell.h>

enum shell_wildcard_status {
	SHELL_WILDCARD_CMD_ADDED,
	SHELL_WILDCARD_CMD_MISSING_SPACE,
	SHELL_WILDCARD_CMD_NO_MATCH_FOUND, /* no matching command */
	SHELL_WILDCARD_NOT_FOUND /* wildcard character not found */
};


bool shell_wildcard_character_exist(const char *str);

void shell_wildcard_prepare(const struct shell *shell);

/* Function expands wildcards in the shell temporary buffer */
enum shell_wildcard_status shell_wildcard_process(const struct shell *shell,
					      const struct shell_cmd_entry *cmd,
					      const char *pattern);

void shell_wildcard_finalize(const struct shell *shell);


#endif /* SHELL_SHELL_WILDCARDS_H__ */
