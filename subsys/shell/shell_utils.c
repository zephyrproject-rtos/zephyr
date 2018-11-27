/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "shell_utils.h"
#include <ctype.h>

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
				u8_t v = 0;

				for (i = 2; i < (2 + 3); i++) {
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
				u8_t v = 0;

				for (i = 2; i < (2 + 2); i++) {
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
	u16_t pattern_len = shell_strlen(pattern);
	size_t shift;

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
	u16_t shift = 0;

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
					shift = 0;
				}

				break;
			}
		}
	}
}

void shell_buffer_trim(char *buff, u16_t *buff_len)
{
	u16_t i = 0;

	/* no command in the buffer */
	if (buff[0] == '\0') {
		return;
	}

	while (isspace((int) buff[*buff_len - 1])) {
		*buff_len -= 1;
		if (*buff_len == 0) {
			buff[0] = '\0';
			return;
		}
	}
	buff[*buff_len] = '\0';

	/* Counting whitespace characters starting from beginning of the
	 * command.
	 */
	while (isspace((int) buff[i++])) {
		if (i == 0) {
			buff[0] = '\0';
			return;
		}
	}

	/* Removing counted whitespace characters. */
	if (--i > 0) {
		memmove(buff, buff + i, (*buff_len + 1) - i); /* +1 for '\0' */
		*buff_len = *buff_len - i;
	}
}

u16_t shell_str_similarity_check(const char *str_a, const char *str_b)
{
	u16_t cnt = 0;

	while (str_a[cnt] != '\0') {
		if (str_a[cnt] != str_b[cnt]) {
			return cnt;
		}

		if (++cnt == 0) {
			return --cnt; /* too long strings */
		}
	}

	return cnt;
}
