/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <fnmatch.h>
#include "shell_wildcard.h"
#include "shell_utils.h"
#include "shell_ops.h"

static enum shell_wildcard_status command_add(char *buff, uint16_t *buff_len,
					      char const *cmd,
					      char const *pattern)
{
	uint16_t cmd_len = z_shell_strlen(cmd);
	char *completion_addr;
	uint16_t shift;

	/* +1 for space */
	if ((*buff_len + cmd_len + 1) > CONFIG_SHELL_CMD_BUFF_SIZE) {
		return SHELL_WILDCARD_CMD_MISSING_SPACE;
	}

	completion_addr = strstr(buff, pattern);

	if (!completion_addr) {
		return SHELL_WILDCARD_CMD_NO_MATCH_FOUND;
	}

	shift = z_shell_strlen(completion_addr);

	/* make place for new command: + 1 for space + 1 for EOS */
	memmove(completion_addr + cmd_len + 1, completion_addr, shift + 1);
	memcpy(completion_addr, cmd, cmd_len);
	/* adding space to not brake next command in the buffer */
	completion_addr[cmd_len] = ' ';

	*buff_len += cmd_len + 1; /* + 1 for space */

	return SHELL_WILDCARD_CMD_ADDED;
}

/**
 * @internal @brief Function for searching and adding commands to the temporary
 * shell buffer matching to wildcard pattern.
 *
 * Function will search commands tree for commands matching wildcard pattern
 * stored in argv[cmd_lvl]. When match is found wildcard pattern will be
 * replaced by matching commands. If there is no space in the buffer to add all
 * matching commands function will add as many as possible. Next it will
 * continue to search for next wildcard pattern and it will try to add matching
 * commands.
 *
 *
 * This function is internal to shell module and shall be not called directly.
 *
 * @param[in/out] shell		Pointer to the CLI instance.
 * @param[in]	  cmd		Pointer to command which will be processed
 * @param[in]	  pattern	Pointer to wildcard pattern.
 *
 * @retval WILDCARD_CMD_ADDED	All matching commands added to the buffer.
 * @retval WILDCARD_CMD_ADDED_MISSING_SPACE  Not all matching commands added
 *					     because CONFIG_SHELL_CMD_BUFF_SIZE
 *					     is too small.
 * @retval WILDCARD_CMD_NO_MATCH_FOUND No matching command found.
 */
static enum shell_wildcard_status commands_expand(const struct shell *shell,
					const struct shell_static_entry *cmd,
					const char *pattern)
{
	enum shell_wildcard_status ret_val = SHELL_WILDCARD_CMD_NO_MATCH_FOUND;
	struct shell_static_entry const *entry = NULL;
	struct shell_static_entry dloc;
	size_t cmd_idx = 0;
	size_t cnt = 0;

	while ((entry = z_shell_cmd_get(cmd, cmd_idx++, &dloc)) != NULL) {

		if (fnmatch(pattern, entry->syntax, 0) == 0) {
			ret_val = command_add(shell->ctx->temp_buff,
					      &shell->ctx->cmd_tmp_buff_len,
					      entry->syntax, pattern);
			if (ret_val == SHELL_WILDCARD_CMD_MISSING_SPACE) {
				z_shell_fprintf(shell, SHELL_WARNING,
					"Command buffer is too short to"
					" expand all commands matching"
					" wildcard pattern: %s\n", pattern);
				break;
			} else if (ret_val != SHELL_WILDCARD_CMD_ADDED) {
				break;
			}
			cnt++;
		}
	};

	if (cnt > 0) {
		z_shell_pattern_remove(shell->ctx->temp_buff,
				       &shell->ctx->cmd_tmp_buff_len, pattern);
	}

	return ret_val;
}

bool z_shell_has_wildcard(const char *str)
{
	uint16_t str_len = z_shell_strlen(str);

	for (size_t i = 0; i < str_len; i++) {
		if ((str[i] == '?') || (str[i] == '*')) {
			return true;
		}
	}

	return false;
}

void z_shell_wildcard_prepare(const struct shell *shell)
{
	/* Wildcard can be correctly handled under following conditions:
	 * - wildcard command does not have a handler
	 * - wildcard command is on the deepest commands level
	 * - other commands on the same level as wildcard command shall also not
	 *   have a handler
	 *
	 * Algorithm:
	 * 1. Command buffer: ctx->cmd_buff is copied to temporary buffer:
	 *    ctx->temp_buff.
	 * 2. Algorithm goes through command buffer to find handlers and
	 *    subcommands.
	 * 3. If algorithm will find a wildcard character it switches to
	 *    Temporary buffer.
	 * 4. In the Temporary buffer command containing wildcard character is
	 *    replaced by matching command(s).
	 * 5. Algorithm switch back to Command buffer and analyzes next command.
	 * 6. When all arguments are analyzed from Command buffer, Temporary
	 *    buffer with all expanded commands is copied to Command buffer.
	 * 7. Deepest found handler is executed and all lower level commands,
	 *    including expanded commands, are passed as arguments.
	 */

	memset(shell->ctx->temp_buff, 0, sizeof(shell->ctx->temp_buff));
	memcpy(shell->ctx->temp_buff,
			shell->ctx->cmd_buff,
			shell->ctx->cmd_buff_len);

	/* Function shell_spaces_trim must be used instead of shell_make_argv.
	 * At this point it is important to keep temp_buff as one string.
	 * It will allow to find wildcard commands easily with strstr function.
	 */
	z_shell_spaces_trim(shell->ctx->temp_buff);

	/* +1 for EOS*/
	shell->ctx->cmd_tmp_buff_len = z_shell_strlen(shell->ctx->temp_buff) + 1;
}


enum shell_wildcard_status z_shell_wildcard_process(const struct shell *shell,
					const struct shell_static_entry *cmd,
					const char *pattern)
{
	enum shell_wildcard_status ret_val = SHELL_WILDCARD_NOT_FOUND;

	if (cmd == NULL) {
		return ret_val;
	}

	if (!z_shell_has_wildcard(pattern)) {
		return ret_val;
	}

	/* Function will search commands tree for commands matching wildcard
	 * pattern stored in argv[cmd_lvl]. When match is found wildcard pattern
	 * will be replaced by matching commands. If there is no space in the
	 * buffer to add all matching commands function will add as many as
	 * possible. Next it will continue to search for next wildcard pattern
	 * and it will try to add matching commands.
	 */
	ret_val = commands_expand(shell, cmd, pattern);

	return ret_val;
}

void z_shell_wildcard_finalize(const struct shell *shell)
{
	memcpy(shell->ctx->cmd_buff,
	       shell->ctx->temp_buff,
	       shell->ctx->cmd_tmp_buff_len);
	shell->ctx->cmd_buff_len = shell->ctx->cmd_tmp_buff_len;
}
