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
static void formatted_text_print(const struct shell *sh, const char *str,
				 size_t terminal_offset, bool offset_first_line)
{
	size_t offset = 0;
	size_t length;

	if (str == NULL) {
		return;
	}

	if (offset_first_line) {
		z_shell_op_cursor_horiz_move(sh, terminal_offset);
	}


	/* Skipping whitespace. */
	while (isspace((int) *(str + offset)) != 0) {
		++offset;
	}

	while (true) {
		size_t idx = 0;

		length = z_shell_strlen(str) - offset;

		if (length <=
		    sh->ctx->vt100_ctx.cons.terminal_wid - terminal_offset) {
			for (idx = 0; idx < length; idx++) {
				if (*(str + offset + idx) == '\n') {
					z_transport_buffer_flush(sh);
					z_shell_write(sh, str + offset, idx);
					offset += idx + 1;
					z_cursor_next_line_move(sh);
					z_shell_op_cursor_horiz_move(sh,
							terminal_offset);
					break;
				}
			}

			/* String will fit in one line. */
			z_shell_raw_fprintf(sh->fprintf_ctx, str + offset);

			break;
		}

		/* String is longer than terminal line so text needs to
		 * divide in the way to not divide words.
		 */
		length = sh->ctx->vt100_ctx.cons.terminal_wid
				- terminal_offset;

		while (true) {
			/* Determining line break. */
			if (isspace((int) (*(str + offset + idx))) != 0) {
				length = idx;
				if (*(str + offset + idx) == '\n') {
					break;
				}
			}

			if ((idx + terminal_offset) >=
			    sh->ctx->vt100_ctx.cons.terminal_wid) {
				/* End of line reached. */
				break;
			}

			++idx;
		}

		/* Writing one line, fprintf IO buffer must be flushed
		 * before calling shell_write.
		 */
		z_transport_buffer_flush(sh);
		z_shell_write(sh, str + offset, length);
		offset += length;

		/* Calculating text offset to ensure that next line will
		 * not begin with a space.
		 */
		while (isspace((int) (*(str + offset))) != 0) {
			++offset;
		}

		z_cursor_next_line_move(sh);
		z_shell_op_cursor_horiz_move(sh, terminal_offset);

	}
	z_cursor_next_line_move(sh);
}

static void help_item_print(const struct shell *sh, const char *item_name,
			    uint16_t item_name_width, const char *item_help)
{
	static const uint8_t tabulator[] = "  ";
	static const char sub_cmd_sep[] = ": "; /* subcommands separator */
	const uint16_t offset = 2 * strlen(tabulator) + item_name_width + strlen(sub_cmd_sep);

	if ((item_name == NULL) || (item_name[0] == '\0')) {
		return;
	}

	if (!IS_ENABLED(CONFIG_NEWLIB_LIBC) &&
	    !IS_ENABLED(CONFIG_ARCH_POSIX)) {
		/* print option name */
		z_shell_fprintf(sh, SHELL_NORMAL, "%s%-*s", tabulator,
				item_name_width, item_name);
	} else {
		uint16_t tmp = item_name_width - strlen(item_name);
		char space = ' ';

		z_shell_fprintf(sh, SHELL_NORMAL, "%s%s", tabulator,
				item_name);

		if (item_help) {
			for (uint16_t i = 0; i < tmp; i++) {
				z_shell_write(sh, &space, 1);
			}
		}
	}

	if (item_help == NULL) {
		z_cursor_next_line_move(sh);
		return;
	} else {
		z_shell_fprintf(sh, SHELL_NORMAL, "%s: ", tabulator);
	}
	/* print option help */
	formatted_text_print(sh, item_help, offset, false);
}

/* Function prints all subcommands of the parent command together with their
 * help string
 */
void z_shell_help_subcmd_print(const struct shell *sh,
			       const struct shell_static_entry *parent,
			       const char *description)
{
	const struct shell_static_entry *entry = NULL;
	struct shell_static_entry dloc;
	uint16_t longest = 0U;
	size_t idx = 0;

	/* Searching for the longest subcommand to print. */
	while ((entry = z_shell_cmd_get(parent, idx++, &dloc)) != NULL) {
		longest = Z_MAX(longest, z_shell_strlen(entry->syntax));
	}

	/* No help to print */
	if (longest == 0) {
		return;
	}

	if (description != NULL) {
		z_shell_fprintf(sh, SHELL_NORMAL, description);
	}

	/* Printing subcommands and help string (if exists). */
	idx = 0;

	while ((entry = z_shell_cmd_get(parent, idx++, &dloc)) != NULL) {
		help_item_print(sh, entry->syntax, longest, entry->help);
	}
}

void z_shell_help_cmd_print(const struct shell *sh,
			    const struct shell_static_entry *cmd)
{
	static const char cmd_sep[] = " - "; /* commands separator */
	uint16_t field_width;

	field_width = z_shell_strlen(cmd->syntax) + z_shell_strlen(cmd_sep);

	z_shell_fprintf(sh, SHELL_NORMAL, "%s%s", cmd->syntax, cmd_sep);

	formatted_text_print(sh, cmd->help, field_width, false);
}

bool z_shell_help_request(const char *str)
{
	if (!IS_ENABLED(CONFIG_SHELL_HELP_OPT_PARSE)) {
		return false;
	}

	if (!strcmp(str, "-h") || !strcmp(str, "--help")) {
		return true;
	}

	return false;
}
