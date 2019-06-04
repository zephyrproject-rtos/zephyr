/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ctype.h>
#include "shell_utils.h"
#include "shell_wildcard.h"

extern const struct shell_cmd_entry __shell_root_cmds_start[0];
extern const struct shell_cmd_entry __shell_root_cmds_end[0];

static inline const struct shell_cmd_entry *shell_root_cmd_get(u32_t id)
{
	return &__shell_root_cmds_start[id];
}

/* Calculates relative line number of given position in buffer */
static u32_t line_num_with_buffer_offset_get(struct shell_multiline_cons *cons,
					     u16_t buffer_pos)
{
	return ((buffer_pos + cons->name_len) / cons->terminal_wid);
}

/* Calculates column number of given position in buffer */
static u32_t col_num_with_buffer_offset_get(struct shell_multiline_cons *cons,
					    u16_t buffer_pos)
{
	/* columns are counted from 1 */
	return (1 + ((buffer_pos + cons->name_len) % cons->terminal_wid));
}

s32_t column_span_with_buffer_offsets_get(struct shell_multiline_cons *cons,
					  u16_t offset1,
					  u16_t offset2)
{
	return col_num_with_buffer_offset_get(cons, offset2)
			- col_num_with_buffer_offset_get(cons, offset1);
}

s32_t row_span_with_buffer_offsets_get(struct shell_multiline_cons *cons,
				       u16_t offset1,
				       u16_t offset2)
{
	return line_num_with_buffer_offset_get(cons, offset2)
		- line_num_with_buffer_offset_get(cons, offset1);
}

void shell_multiline_data_calc(struct shell_multiline_cons *cons,
			       u16_t buff_pos, u16_t buff_len)
{
	/* Current cursor position in command.
	 * +1 -> because home position is (1, 1)
	 */
	cons->cur_x = (buff_pos + cons->name_len) % cons->terminal_wid + 1;
	cons->cur_y = (buff_pos + cons->name_len) / cons->terminal_wid + 1;

	/* Extreme position when cursor is at the end of command. */
	cons->cur_y_end = (buff_len + cons->name_len) / cons->terminal_wid + 1;
	cons->cur_x_end = (buff_len + cons->name_len) % cons->terminal_wid + 1;
}

static char make_argv(char **ppcmd, u8_t c)
{
	char *cmd = *ppcmd;
	char quote = 0;

	while (1) {
		c = *cmd;

		if (c == '\0') {
			break;
		}

		if (!quote) {
			switch (c) {
			case '\\':
				memmove(cmd, cmd + 1,
						shell_strlen(cmd));
				cmd += 1;
				continue;

			case '\'':
			case '\"':
				memmove(cmd, cmd + 1,
						shell_strlen(cmd));
				quote = c;
				continue;
			default:
				break;
			}
		}

		if (quote == c) {
			memmove(cmd, cmd + 1, shell_strlen(cmd));
			quote = 0;
			continue;
		}

		if (quote && c == '\\') {
			char t = *(cmd + 1);

			if (t == quote) {
				memmove(cmd, cmd + 1,
						shell_strlen(cmd));
				cmd += 1;
				continue;
			}

			if (t == '0') {
				u8_t i;
				u8_t v = 0U;

				for (i = 2U; i < (2 + 3); i++) {
					t = *(cmd + i);

					if (t >= '0' && t <= '7') {
						v = (v << 3) | (t - '0');
					} else {
						break;
					}
				}

				if (i > 2) {
					memmove(cmd, cmd + (i - 1),
						shell_strlen(cmd) - (i - 2));
					*cmd++ = v;
					continue;
				}
			}

			if (t == 'x') {
				u8_t i;
				u8_t v = 0U;

				for (i = 2U; i < (2 + 2); i++) {
					t = *(cmd + i);

					if (t >= '0' && t <= '9') {
						v = (v << 4) | (t - '0');
					} else if ((t >= 'a') &&
						   (t <= 'f')) {
						v = (v << 4) | (t - 'a' + 10);
					} else if ((t >= 'A') && (t <= 'F')) {
						v = (v << 4) | (t - 'A' + 10);
					} else {
						break;
					}
				}

				if (i > 2) {
					memmove(cmd, cmd + (i - 1),
						shell_strlen(cmd) - (i - 2));
					*cmd++ = v;
					continue;
				}
			}
		}

		if (!quote && isspace((int) c)) {
			break;
		}

		cmd += 1;
	}
	*ppcmd = cmd;

	return quote;
}


char shell_make_argv(size_t *argc, char **argv, char *cmd, u8_t max_argc)
{
	char quote = 0;
	char c;

	*argc = 0;
	do {
		c = *cmd;
		if (c == '\0') {
			break;
		}

		if (isspace((int) c)) {
			*cmd++ = '\0';
			continue;
		}

		argv[(*argc)++] = cmd;
		quote = make_argv(&cmd, c);
	} while (*argc < max_argc);

	argv[*argc] = 0;

	return quote;
}

void shell_pattern_remove(char *buff, u16_t *buff_len, const char *pattern)
{
	char *pattern_addr = strstr(buff, pattern);
	u16_t shift;
	u16_t pattern_len = shell_strlen(pattern);

	if (!pattern_addr) {
		return;
	}

	if (pattern_addr > buff) {
		if (*(pattern_addr - 1) == ' ') {
			pattern_len++; /* space needs to be removed as well */
			pattern_addr--; /* set pointer to space */
		}
	}

	shift = shell_strlen(pattern_addr) - pattern_len + 1; /* +1 for EOS */
	*buff_len -= pattern_len;

	memmove(pattern_addr, pattern_addr + pattern_len, shift);
}

static inline u32_t shell_root_cmd_count(void)
{
	return ((u8_t *)__shell_root_cmds_end -
			(u8_t *)__shell_root_cmds_start)/
				sizeof(struct shell_cmd_entry);
}

/* Function returning pointer to root command matching requested syntax. */
const struct shell_static_entry *shell_root_cmd_find(const char *syntax)
{
	const size_t cmd_count = shell_root_cmd_count();
	const struct shell_cmd_entry *cmd;

	for (size_t cmd_idx = 0; cmd_idx < cmd_count; ++cmd_idx) {
		cmd = shell_root_cmd_get(cmd_idx);
		if (strcmp(syntax, cmd->u.entry->syntax) == 0) {
			return cmd->u.entry;
		}
	}

	return NULL;
}

void shell_cmd_get(const struct shell *shell,
		   const struct shell_cmd_entry *command, size_t lvl,
		   size_t idx, const struct shell_static_entry **entry,
		   struct shell_static_entry *d_entry)
{
	__ASSERT_NO_MSG(entry != NULL);
	__ASSERT_NO_MSG(d_entry != NULL);

	*entry = NULL;

	if (lvl == SHELL_CMD_ROOT_LVL) {
		if (shell_in_select_mode(shell)	&&
		    IS_ENABLED(CONFIG_SHELL_CMDS_SELECT)) {
			const struct shell_static_entry *ptr =
						       shell->ctx->selected_cmd;
			if (ptr->subcmd->u.entry[idx].syntax != NULL) {
				*entry = &ptr->subcmd->u.entry[idx];
			}
		} else if (idx < shell_root_cmd_count()) {
			const struct shell_cmd_entry *cmd;

			cmd = shell_root_cmd_get(idx);
			*entry = cmd->u.entry;
		}
		return;
	}

	if (command == NULL) {
		return;
	}

	if (command->is_dynamic) {
		command->u.dynamic_get(idx, d_entry);
		if (d_entry->syntax != NULL) {
			*entry = d_entry;
		}
	} else {
		if (command->u.entry[idx].syntax != NULL) {
			*entry = &command->u.entry[idx];
		}
	}
}

/* Function returns pointer to a command matching given pattern.
 *
 * @param shell		Shell instance.
 * @param cmd		Pointer to commands array that will be searched.
 * @param lvl		Root command indicator. If set to 0 shell will search
 *			for pattern in root commands.
 * @param cmd_str	Command pattern to be found.
 * @param d_entry	Shell static command descriptor.
 *
 * @return		Pointer to found command.
 */
static const struct shell_static_entry *find_cmd(
					     const struct shell *shell,
					     const struct shell_cmd_entry *cmd,
					     size_t lvl,
					     char *cmd_str,
					     struct shell_static_entry *d_entry)
{
	const struct shell_static_entry *entry = NULL;
	size_t idx = 0;

	do {
		shell_cmd_get(shell, cmd, lvl, idx++, &entry, d_entry);
		if (entry && (strcmp(cmd_str, entry->syntax) == 0)) {
			return entry;
		}
	} while (entry);

	return NULL;
}

const struct shell_static_entry *shell_get_last_command(
					     const struct shell *shell,
					     size_t argc,
					     char *argv[],
					     size_t *match_arg,
					     struct shell_static_entry *d_entry,
					     bool only_static)
{
	const struct shell_static_entry *prev_entry = NULL;
	const struct shell_static_entry *entry = NULL;
	const struct shell_cmd_entry *cmd = NULL;

	*match_arg = SHELL_CMD_ROOT_LVL;

	while (*match_arg < argc) {

		if (IS_ENABLED(CONFIG_SHELL_WILDCARD)) {
			/* ignore wildcard argument */
			if (shell_wildcard_character_exist(argv[*match_arg])) {
				(*match_arg)++;
				continue;
			}
		}

		entry = find_cmd(shell, cmd, *match_arg, argv[*match_arg],
				 d_entry);
		if (entry) {
			cmd = entry->subcmd;
			prev_entry = entry;
			(*match_arg)++;
		} else {
			break;
		}

		if (cmd == NULL) {
			return NULL;
		}

		if (only_static && cmd->is_dynamic) {
			(*match_arg)--;
			return NULL;
		}
	}

	return entry;
}

int shell_command_add(char *buff, u16_t *buff_len,
		      const char *new_cmd, const char *pattern)
{
	u16_t cmd_len = shell_strlen(new_cmd);
	char *cmd_source_addr;
	u16_t shift;

	/* +1 for space */
	if ((*buff_len + cmd_len + 1) > CONFIG_SHELL_CMD_BUFF_SIZE) {
		return -ENOMEM;
	}

	cmd_source_addr = strstr(buff, pattern);

	if (!cmd_source_addr) {
		return -EINVAL;
	}

	shift = shell_strlen(cmd_source_addr);

	/* make place for new command: + 1 for space + 1 for EOS */
	memmove(cmd_source_addr + cmd_len + 1, cmd_source_addr, shift + 1);
	memcpy(cmd_source_addr, new_cmd, cmd_len);
	cmd_source_addr[cmd_len] = ' ';

	*buff_len += cmd_len + 1; /* + 1 for space */

	return 0;
}

void shell_spaces_trim(char *str)
{
	u16_t len = shell_strlen(str);
	u16_t shift = 0U;

	if (!str) {
		return;
	}

	for (u16_t i = 0; i < len - 1; i++) {
		if (isspace((int)str[i])) {
			for (u16_t j = i + 1; j < len; j++) {
				if (isspace((int)str[j])) {
					shift++;
					continue;
				}

				if (shift > 0) {
					/* +1 for EOS */
					memmove(&str[i + 1],
						&str[j],
						len - shift + 1);
					len -= shift;
					shift = 0U;
				}

				break;
			}
		}
	}
}

/** @brief Remove white chars from beginning and end of command buffer.
 *
 */
static void buffer_trim(char *buff, u16_t *buff_len)
{
	u16_t i = 0U;

	/* no command in the buffer */
	if (buff[0] == '\0') {
		return;
	}

	while (isspace((int) buff[*buff_len - 1U])) {
		*buff_len -= 1U;
		if (*buff_len == 0U) {
			buff[0] = '\0';
			return;
		}
	}
	buff[*buff_len] = '\0';

	/* Counting whitespace characters starting from beginning of the
	 * command.
	 */
	while (isspace((int) buff[i++])) {
	}


	/* Removing counted whitespace characters. */
	if (--i > 0) {
		memmove(buff, buff + i, (*buff_len + 1U) - i); /* +1 for '\0' */
		*buff_len = *buff_len - i;
	}
}

void shell_cmd_trim(const struct shell *shell)
{
	buffer_trim(shell->ctx->cmd_buff, &shell->ctx->cmd_buff_len);
	shell->ctx->cmd_buff_pos = shell->ctx->cmd_buff_len;
}
