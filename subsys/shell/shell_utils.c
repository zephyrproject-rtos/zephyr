/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ctype.h>
#include <zephyr/device.h>
#include <zephyr/sys/iterable_sections.h>
#include <stdlib.h>
#include "shell_utils.h"
#include "shell_wildcard.h"

TYPE_SECTION_START_EXTERN(union shell_cmd_entry, shell_dynamic_subcmds);
TYPE_SECTION_END_EXTERN(union shell_cmd_entry, shell_dynamic_subcmds);

TYPE_SECTION_START_EXTERN(union shell_cmd_entry, shell_subcmds);
TYPE_SECTION_END_EXTERN(union shell_cmd_entry, shell_subcmds);

/* Macro creates empty entry at the bottom of the memory section with subcommands
 * it is used to detect end of subcommand set that is located before this marker.
 */
#define Z_SHELL_SUBCMD_END_MARKER_CREATE()				\
	static const TYPE_SECTION_ITERABLE(struct shell_static_entry, \
			z_shell_subcmd_end_marker, shell_subcmds, Z_SHELL_UNDERSCORE(999))

Z_SHELL_SUBCMD_END_MARKER_CREATE();

static inline const union shell_cmd_entry *shell_root_cmd_get(uint32_t id)
{
	const union shell_cmd_entry *cmd;

	TYPE_SECTION_GET(union shell_cmd_entry, shell_root_cmds, id, &cmd);

	return cmd;
}

/* Determine if entry is a dynamic command by checking if address is within
 * dynamic commands memory section.
 */
static inline bool is_dynamic_cmd(const union shell_cmd_entry *entry)
{
	return (entry >= TYPE_SECTION_START(shell_dynamic_subcmds)) &&
		(entry < TYPE_SECTION_END(shell_dynamic_subcmds));
}

/* Determine if entry is a section command by checking if address is within
 * subcommands memory section.
 */
static inline bool is_section_cmd(const union shell_cmd_entry *entry)
{
	return (entry >= TYPE_SECTION_START(shell_subcmds)) &&
		(entry < TYPE_SECTION_END(shell_subcmds));
}

/* Calculates relative line number of given position in buffer */
static uint32_t line_num_with_buffer_offset_get(struct shell_multiline_cons *cons,
					     uint16_t buffer_pos)
{
	return ((buffer_pos + cons->name_len) / cons->terminal_wid);
}

/* Calculates column number of given position in buffer */
static uint32_t col_num_with_buffer_offset_get(struct shell_multiline_cons *cons,
					    uint16_t buffer_pos)
{
	/* columns are counted from 1 */
	return (1 + ((buffer_pos + cons->name_len) % cons->terminal_wid));
}

int32_t z_column_span_with_buffer_offsets_get(struct shell_multiline_cons *cons,
					      uint16_t offset1,
					      uint16_t offset2)
{
	return col_num_with_buffer_offset_get(cons, offset2)
			- col_num_with_buffer_offset_get(cons, offset1);
}

int32_t z_row_span_with_buffer_offsets_get(struct shell_multiline_cons *cons,
					   uint16_t offset1,
					   uint16_t offset2)
{
	return line_num_with_buffer_offset_get(cons, offset2)
		- line_num_with_buffer_offset_get(cons, offset1);
}

void z_shell_multiline_data_calc(struct shell_multiline_cons *cons,
				 uint16_t buff_pos, uint16_t buff_len)
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

static char make_argv(char **ppcmd, uint8_t c)
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
						z_shell_strlen(cmd));
				cmd += 1;
				continue;

			case '\'':
			case '\"':
				memmove(cmd, cmd + 1,
						z_shell_strlen(cmd));
				quote = c;
				continue;
			default:
				break;
			}
		}

		if (quote == c) {
			memmove(cmd, cmd + 1, z_shell_strlen(cmd));
			quote = 0;
			continue;
		}

		if (quote && c == '\\') {
			char t = *(cmd + 1);

			if (t == quote) {
				memmove(cmd, cmd + 1,
						z_shell_strlen(cmd));
				cmd += 1;
				continue;
			}

			if (t == '0') {
				uint8_t i;
				uint8_t v = 0U;

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
						z_shell_strlen(cmd) - (i - 2));
					*cmd++ = v;
					continue;
				}
			}

			if (t == 'x') {
				uint8_t i;
				uint8_t v = 0U;

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
						z_shell_strlen(cmd) - (i - 2));
					*cmd++ = v;
					continue;
				}
			}
		}

		if (!quote && isspace((int) c) != 0) {
			break;
		}

		cmd += 1;
	}
	*ppcmd = cmd;

	return quote;
}


char z_shell_make_argv(size_t *argc, const char **argv, char *cmd,
		       uint8_t max_argc)
{
	char quote = 0;
	char c;

	*argc = 0;
	do {
		c = *cmd;
		if (c == '\0') {
			break;
		}

		if (isspace((int) c) != 0) {
			*cmd++ = '\0';
			continue;
		}

		argv[(*argc)++] = cmd;
		if (*argc == max_argc) {
			break;
		}
		quote = make_argv(&cmd, c);
	} while (true);

	return quote;
}

void z_shell_pattern_remove(char *buff, uint16_t *buff_len, const char *pattern)
{
	char *pattern_addr = strstr(buff, pattern);
	uint16_t shift;
	uint16_t pattern_len = z_shell_strlen(pattern);

	if (!pattern_addr) {
		return;
	}

	if (pattern_addr > buff) {
		if (*(pattern_addr - 1) == ' ') {
			pattern_len++; /* space needs to be removed as well */
			pattern_addr--; /* set pointer to space */
		}
	}

	shift = z_shell_strlen(pattern_addr) - pattern_len + 1; /* +1 for EOS */
	*buff_len -= pattern_len;

	memmove(pattern_addr, pattern_addr + pattern_len, shift);
}

static inline uint32_t shell_root_cmd_count(void)
{
	size_t len;

	TYPE_SECTION_COUNT(union shell_cmd_entry, shell_root_cmds, &len);

	return len;
}

/* Function returning pointer to parent command matching requested syntax. */
const struct shell_static_entry *root_cmd_find(const char *syntax)
{
	const size_t cmd_count = shell_root_cmd_count();
	const union shell_cmd_entry *cmd;

	for (size_t cmd_idx = 0; cmd_idx < cmd_count; ++cmd_idx) {
		cmd = shell_root_cmd_get(cmd_idx);
		if (strcmp(syntax, cmd->entry->syntax) == 0) {
			return cmd->entry;
		}
	}

	return NULL;
}

const struct shell_static_entry *z_shell_cmd_get(
					const struct shell_static_entry *parent,
					size_t idx,
					struct shell_static_entry *dloc)
{
	const struct shell_static_entry *res = NULL;

	if (parent == NULL) {
		return  (idx < shell_root_cmd_count()) ?
				shell_root_cmd_get(idx)->entry : NULL;
	}

	__ASSERT_NO_MSG(dloc != NULL);

	if (parent->subcmd) {
		if (is_dynamic_cmd(parent->subcmd)) {
			parent->subcmd->dynamic_get(idx, dloc);
			if (dloc->syntax != NULL) {
				res = dloc;
			}
		} else {
			const struct shell_static_entry *entry_list;

			if (is_section_cmd(parent->subcmd)) {
				/* First element is null */
				entry_list =
					(const struct shell_static_entry *)parent->subcmd;
				idx++;
			} else {
				entry_list = parent->subcmd->entry;
			}


			if (entry_list[idx].syntax != NULL) {
				res = &entry_list[idx];
			}
		}
	}

	return res;
}

/* Function returns pointer to a command matching given pattern.
 *
 * @param cmd		Pointer to commands array that will be searched.
 * @param lvl		Root command indicator. If set to 0 shell will search
 *			for pattern in parent commands.
 * @param cmd_str	Command pattern to be found.
 * @param dloc	Shell static command descriptor.
 *
 * @return		Pointer to found command.
 */
const struct shell_static_entry *z_shell_find_cmd(
					const struct shell_static_entry *parent,
					const char *cmd_str,
					struct shell_static_entry *dloc)
{
	const struct shell_static_entry *entry;
	struct shell_static_entry parent_cpy;
	size_t idx = 0;

	/* Dynamic command operates on shared memory. If we are processing two
	 * dynamic commands at the same time (current and subcommand) they
	 * will operate on the same memory region what can cause undefined
	 * behaviour.
	 * Hence we need a separate memory for each of them.
	 */
	if (parent) {
		memcpy(&parent_cpy, parent, sizeof(struct shell_static_entry));
		parent = &parent_cpy;
	}

	while ((entry = z_shell_cmd_get(parent, idx++, dloc)) != NULL) {
		if (strcmp(cmd_str, entry->syntax) == 0) {
			return entry;
		}
	}

	return NULL;
}

const struct shell_static_entry *z_shell_get_last_command(
					const struct shell_static_entry *entry,
					size_t argc,
					const char *argv[],
					size_t *match_arg,
					struct shell_static_entry *dloc,
					bool only_static)
{
	const struct shell_static_entry *prev_entry = NULL;

	*match_arg = Z_SHELL_CMD_ROOT_LVL;

	while (*match_arg < argc) {

		if (IS_ENABLED(CONFIG_SHELL_WILDCARD)) {
			/* ignore wildcard argument */
			if (z_shell_has_wildcard(argv[*match_arg])) {
				(*match_arg)++;
				continue;
			}
		}

		prev_entry = entry;
		entry = z_shell_find_cmd(entry, argv[*match_arg], dloc);
		if (entry) {
			(*match_arg)++;
		} else {
			entry = prev_entry;
			break;
		}

		if (only_static && (entry == dloc)) {
			(*match_arg)--;
			return NULL;
		}
	}

	return entry;
}

int shell_set_root_cmd(const char *cmd)
{
	const struct shell_static_entry *entry;

	entry = cmd ? root_cmd_find(cmd) : NULL;

	if (cmd && (entry == NULL)) {
		return -EINVAL;
	}

	STRUCT_SECTION_FOREACH(shell, sh) {
		sh->ctx->selected_cmd = entry;
	}

	return 0;
}




void z_shell_spaces_trim(char *str)
{
	uint16_t len = z_shell_strlen(str);
	uint16_t shift = 0U;

	if (len == 0U) {
		return;
	}

	for (uint16_t i = 0; i < len - 1; i++) {
		if (isspace((int)str[i]) != 0) {
			for (uint16_t j = i + 1; j < len; j++) {
				if (isspace((int)str[j]) != 0) {
					shift++;
					continue;
				}

				if (shift > 0) {
					/* +1 for EOS */
					memmove(&str[i + 1],
						&str[j],
						len - j + 1);
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
static void buffer_trim(char *buff, uint16_t *buff_len)
{
	uint16_t i = 0U;

	/* no command in the buffer */
	if (buff[0] == '\0') {
		return;
	}

	while (isspace((int) buff[*buff_len - 1U]) != 0) {
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
	while (isspace((int) buff[i++]) != 0) {
	}


	/* Removing counted whitespace characters. */
	if (--i > 0) {
		memmove(buff, buff + i, (*buff_len + 1U) - i); /* +1 for '\0' */
		*buff_len = *buff_len - i;
	}
}

void z_shell_cmd_trim(const struct shell *sh)
{
	buffer_trim(sh->ctx->cmd_buff, &sh->ctx->cmd_buff_len);
	sh->ctx->cmd_buff_pos = sh->ctx->cmd_buff_len;
}

static const struct device *shell_device_internal(size_t idx,
						  const char *prefix,
						  shell_device_filter_t filter)
{
	size_t match_idx = 0;
	const struct device *dev;
	size_t len = z_device_get_all_static(&dev);
	const struct device *dev_end = dev + len;

	while (dev < dev_end) {
		if (device_is_ready(dev)
		    && (dev->name != NULL)
		    && (strlen(dev->name) != 0)
		    && ((prefix == NULL)
			|| (strncmp(prefix, dev->name,
				    strlen(prefix)) == 0))
		    && (filter == NULL || filter(dev))) {
			if (match_idx == idx) {
				return dev;
			}
			++match_idx;
		}
		++dev;
	}

	return NULL;
}

const struct device *shell_device_filter(size_t idx,
					 shell_device_filter_t filter)
{
	return shell_device_internal(idx, NULL, filter);
}

const struct device *shell_device_lookup(size_t idx,
					 const char *prefix)
{
	return shell_device_internal(idx, prefix, NULL);
}

long shell_strtol(const char *str, int base, int *err)
{
	long val;
	char *endptr = NULL;

	errno = 0;
	val = strtol(str, &endptr, base);
	if (errno == ERANGE) {
		*err = -ERANGE;
		return 0;
	} else if (errno || endptr == str || *endptr) {
		*err = -EINVAL;
		return 0;
	}

	return val;
}

unsigned long shell_strtoul(const char *str, int base, int *err)
{
	unsigned long val;
	char *endptr = NULL;

	if (*str == '-') {
		*err = -EINVAL;
		return 0;
	}

	errno = 0;
	val = strtoul(str, &endptr, base);
	if (errno == ERANGE) {
		*err = -ERANGE;
		return 0;
	} else if (errno || endptr == str || *endptr) {
		*err = -EINVAL;
		return 0;
	}

	return val;
}

unsigned long long shell_strtoull(const char *str, int base, int *err)
{
	unsigned long long val;
	char *endptr = NULL;

	if (*str == '-') {
		*err = -EINVAL;
		return 0;
	}

	errno = 0;
	val = strtoull(str, &endptr, base);
	if (errno == ERANGE) {
		*err = -ERANGE;
		return 0;
	} else if (errno || endptr == str || *endptr) {
		*err = -EINVAL;
		return 0;
	}

	return val;
}

bool shell_strtobool(const char *str, int base, int *err)
{
	if (!strcmp(str, "on") || !strcmp(str, "enable") || !strcmp(str, "true")) {
		return true;
	}

	if (!strcmp(str, "off") || !strcmp(str, "disable") || !strcmp(str, "false")) {
		return false;
	}

	return shell_strtoul(str, base, err);
}
