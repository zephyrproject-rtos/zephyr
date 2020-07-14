/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ctype.h>
#include "shell_ops.h"
#include "shell_help.h"
#include "shell_utils.h"


/* Function prints a string on terminal screen with requested margin.
 * It takes care to not divide words.
 *   shell		Pointer to shell instance.
 *   p_str		Pointer to string to be printed.
 *   terminal_offset	Requested left margin.
 *   offset_first_line	Add margin to the first printed line.
 */
static void formatted_text_print(const struct shell *shell, const char *str,
				 size_t terminal_offset, bool offset_first_line)
{
	size_t offset = 0;
	size_t length;

	if (str == NULL) {
		return;
	}

	if (offset_first_line) {
		shell_op_cursor_horiz_move(shell, terminal_offset);
	}


	/* Skipping whitespace. */
	while (isspace((int) *(str + offset))) {
		++offset;
	}

	while (true) {
		size_t idx = 0;

		length = shell_strlen(str) - offset;

		if (length <=
		    shell->ctx->vt100_ctx.cons.terminal_wid - terminal_offset) {
			for (idx = 0; idx < length; idx++) {
				if (*(str + offset + idx) == '\n') {
					transport_buffer_flush(shell);
					shell_write(shell, str + offset, idx);
					offset += idx + 1;
					cursor_next_line_move(shell);
					shell_op_cursor_horiz_move(shell,
							terminal_offset);
					break;
				}
			}

			/* String will fit in one line. */
			shell_raw_fprintf(shell->fprintf_ctx, str + offset);

			break;
		}

		/* String is longer than terminal line so text needs to
		 * divide in the way to not divide words.
		 */
		length = shell->ctx->vt100_ctx.cons.terminal_wid
				- terminal_offset;

		while (true) {
			/* Determining line break. */
			if (isspace((int) (*(str + offset + idx)))) {
				length = idx;
				if (*(str + offset + idx) == '\n') {
					break;
				}
			}

			if ((idx + terminal_offset) >=
			    shell->ctx->vt100_ctx.cons.terminal_wid) {
				/* End of line reached. */
				break;
			}

			++idx;
		}

		/* Writing one line, fprintf IO buffer must be flushed
		 * before calling shell_write.
		 */
		transport_buffer_flush(shell);
		shell_write(shell, str + offset, length);
		offset += length;

		/* Calculating text offset to ensure that next line will
		 * not begin with a space.
		 */
		while (isspace((int) (*(str + offset)))) {
			++offset;
		}

		cursor_next_line_move(shell);
		shell_op_cursor_horiz_move(shell, terminal_offset);

	}
	cursor_next_line_move(shell);
}

static void help_item_print(const struct shell *shell, const char *item_name,
			    uint16_t item_name_width, const char *item_help)
{
	static const uint8_t tabulator[] = "  ";
	const uint16_t offset = 2 * strlen(tabulator) + item_name_width + 1;

	if (item_name == NULL) {
		return;
	}

	if (!IS_ENABLED(CONFIG_NEWLIB_LIBC) && !IS_ENABLED(CONFIG_ARCH_POSIX)) {
		/* print option name */
		shell_internal_fprintf(shell, SHELL_NORMAL, "%s%-*s%s:",
				       tabulator,
				       item_name_width, item_name,
				       tabulator);
	} else {
		uint16_t tmp = item_name_width - strlen(item_name);
		char space = ' ';

		shell_internal_fprintf(shell, SHELL_NORMAL, "%s%s", tabulator,
				       item_name);
		for (uint16_t i = 0; i < tmp; i++) {
			shell_write(shell, &space, 1);
		}
		shell_internal_fprintf(shell, SHELL_NORMAL, "%s:", tabulator);
	}

	if (item_help == NULL) {
		cursor_next_line_move(shell);
		return;
	}
	/* print option help */
	formatted_text_print(shell, item_help, offset, false);
}

/* Function is printing command help, its subcommands name and subcommands
 * help string.
 */
void shell_help_subcmd_print(const struct shell *shell)
{
	const struct shell_static_entry *entry = NULL;
	const struct shell_static_entry *parent = &shell->ctx->active_cmd;
	struct shell_static_entry dloc;
	uint16_t longest = 0U;
	size_t idx = 0;

	/* Searching for the longest subcommand to print. */
	while ((entry = shell_cmd_get(parent, idx++, &dloc)) != NULL) {
		longest = Z_MAX(longest, shell_strlen(entry->syntax));
	};

	/* No help to print */
	if (longest == 0) {
		return;
	}

	shell_internal_fprintf(shell, SHELL_NORMAL, "Subcommands:\n");

	/* Printing subcommands and help string (if exists). */
	idx = 0;

	while ((entry = shell_cmd_get(parent, idx++, &dloc)) != NULL) {
		help_item_print(shell, entry->syntax, longest, entry->help);
	}
}

void shell_help_cmd_print(const struct shell *shell)
{
	static const char cmd_sep[] = " - ";	/* commands separator */

	uint16_t field_width = shell_strlen(shell->ctx->active_cmd.syntax) +
							  shell_strlen(cmd_sep);

	shell_internal_fprintf(shell, SHELL_NORMAL, "%s%s",
			       shell->ctx->active_cmd.syntax, cmd_sep);

	formatted_text_print(shell, shell->ctx->active_cmd.help,
			     field_width, false);
}
