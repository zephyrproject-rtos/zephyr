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

#define SHELL_MSG_SPECIFY_SUBCOMMAND	"Please specify a subcommand.\n"

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

static inline u16_t shell_strlen(const char *str)
{
	return str == NULL ? 0U : (u16_t)strlen(str);
}

char shell_make_argv(size_t *argc, char **argv, char *cmd, uint8_t max_argc);

/** @brief Removes pattern and following space
 *
 */
void shell_pattern_remove(char *buff, u16_t *buff_len, const char *pattern);

/* @brief Function shall be used to search commands.
 *
 * It moves the pointer entry to command of static command structure. If the
 * command cannot be found, the function will set entry to NULL.
 *
 * @param shell		Shell instance.
 * @param command	Pointer to command which will be processed (no matter
 *			the root command).
 * @param lvl		Level of the requested command.
 * @param idx		Index of the requested command.
 * @param entry		Pointer which points to subcommand[idx] after function
 *			execution.
 * @param st_entry	Pointer to the structure where dynamic entry data can
 *			be stored.
 */
void shell_cmd_get(const struct shell *shell,
		   const struct shell_cmd_entry *command, size_t lvl,
		   size_t idx, const struct shell_static_entry **entry,
		   struct shell_static_entry *d_entry);


/* @internal @brief Function returns pointer to a shell's subcommands array
 * for a level given by argc and matching command patter provided in argv.
 *
 * @param shell		Shell instance.
 * @param argc		Number of arguments.
 * @param argv		Pointer to an array with arguments.
 * @param match_arg	Subcommand level of last matching argument.
 * @param d_entry	Shell static command descriptor.
 * @param only_static	If true search only for static commands.
 *
 * @return		Pointer to found command.
 */
const struct shell_static_entry *shell_get_last_command(
					     const struct shell *shell,
					     size_t argc,
					     char *argv[],
					     size_t *match_arg,
					     struct shell_static_entry *d_entry,
					     bool only_static);

int shell_command_add(char *buff, u16_t *buff_len,
		      const char *new_cmd, const char *pattern);

const struct shell_static_entry *shell_root_cmd_find(const char *syntax);

void shell_spaces_trim(char *str);

static inline void transport_buffer_flush(const struct shell *shell)
{
	shell_fprintf_buffer_flush(shell->fprintf_ctx);
}

void shell_cmd_trim(const struct shell *shell);

static inline bool shell_in_select_mode(const struct shell *shell)
{
	return shell->ctx->selected_cmd == NULL ? false : true;
}

#ifdef __cplusplus
}
#endif

#endif /* SHELL_UTILS_H__ */
