/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SHELL_UTILS_H__
#define SHELL_UTILS_H__

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHELL_MSG_SPECIFY_SUBCOMMAND	"Please specify a subcommand.\n"

int32_t z_row_span_with_buffer_offsets_get(struct shell_multiline_cons *cons,
					   uint16_t offset1,
					   uint16_t offset2);

int32_t z_column_span_with_buffer_offsets_get(struct shell_multiline_cons *cons,
					      uint16_t offset1,
					      uint16_t offset2);

void z_shell_multiline_data_calc(struct shell_multiline_cons *cons,
				 uint16_t buff_pos, uint16_t buff_len);

static inline uint16_t z_shell_strlen(const char *str)
{
	return str == NULL ? 0U : (uint16_t)strlen(str);
}

char z_shell_make_argv(size_t *argc, const char **argv,
		       char *cmd, uint8_t max_argc);

/** @brief Removes pattern and following space
 *
 */
void z_shell_pattern_remove(char *buff, uint16_t *buff_len,
			    const char *pattern);

/** @brief Get subcommand with given index from the root.
 *
 * @param parent	Parent entry. Null to get root command from index.
 * @param idx		Command index.
 * @param dloc	Location used to write dynamic entry.
 *
 * @return Fetched command or null if command with that index does not exist.
 */
const struct shell_static_entry *z_shell_cmd_get(
					const struct shell_static_entry *parent,
					size_t idx,
					struct shell_static_entry *dloc);

const struct shell_static_entry *z_shell_find_cmd(
					const struct shell_static_entry *parent,
					const char *cmd_str,
					struct shell_static_entry *dloc);

/* @internal @brief Function returns pointer to a shell's subcommands array
 * for a level given by argc and matching command patter provided in argv.
 *
 * @param sh		Entry. NULL for root entry.
 * @param argc		Number of arguments.
 * @param argv		Pointer to an array with arguments.
 * @param match_arg	Subcommand level of last matching argument.
 * @param d_entry	Shell static command descriptor.
 * @param only_static	If true search only for static commands.
 *
 * @return		Pointer to found command.
 */
const struct shell_static_entry *z_shell_get_last_command(
					const struct shell_static_entry *entry,
					size_t argc,
					const char *argv[],
					size_t *match_arg,
					struct shell_static_entry *dloc,
					bool only_static);

void z_shell_spaces_trim(char *str);
void z_shell_cmd_trim(const struct shell *sh);

const struct shell_static_entry *root_cmd_find(const char *syntax);

static inline void z_transport_buffer_flush(const struct shell *sh)
{
	z_shell_fprintf_buffer_flush(sh->fprintf_ctx);
}

static inline bool z_shell_in_select_mode(const struct shell *sh)
{
	return sh->ctx->selected_cmd == NULL ? false : true;
}

#ifdef __cplusplus
}
#endif

#endif /* SHELL_UTILS_H__ */
