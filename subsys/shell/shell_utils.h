/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SHELL_UTILS_H__
#define SHELL_UTILS_H__

#include <zephyr.h>
#include <shell/shell.h>
#ifdef __cplusplus
extern "C" {
#endif

#define SHELL_MSG_SPECIFY_SUBCOMMAND	"Please specify a subcommand.\r\n"

#define SHELL_DEFAULT_TERMINAL_WIDTH	(80u) /* Default PuTTY width. */
#define SHELL_DEFAULT_TERMINAL_HEIGHT	(24u) /* Default PuTTY height. */



s32_t row_span_with_buffer_offsets_get(struct shell_multiline_cons *cons,
				       u16_t offset1,
				       u16_t offset2);

s32_t column_span_with_buffer_offsets_get(struct shell_multiline_cons *cons,
					  u16_t offset1,
					  u16_t offset2);

void shell_multiline_data_calc(struct shell_multiline_cons *cons,
				   u16_t buff_pos, u16_t buff_len);

static inline size_t shell_strlen(const char *str)
{
	return str == NULL ? 0 : strlen(str);
}

char shell_make_argv(size_t *argc, char **argv, char *cmd, uint8_t max_argc);

/** @brief Removes pattern and following space
 *
 */
void shell_pattern_remove(char *buff, u16_t *buff_len, const char *pattern);

int shell_command_add(char *buff, u16_t *buff_len,
		      const char *new_cmd, const char *pattern);

void shell_spaces_trim(char *str);

/** @brief Remove white chars from beginning and end of command buffer.
 *
 */
void shell_buffer_trim(char *buff, u16_t *buff_len);

/* Function checks how many identical characters have two strings starting
 * from the first character.
 */
u16_t shell_str_similarity_check(const char *str_a, const char *str_b);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_UTILS_H__ */
