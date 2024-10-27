/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/shell/shell.h>
#include <zephyr/sys/iterable_sections.h>

#include "shell_utils.h"
#include "shell_help.h"
#include "shell_ops.h"
#include "shell_vt100.h"

#define SHELL_MSG_CMD_NOT_SUPPORTED	"Command not supported.\n"
#define SHELL_HELP_COMMENT		"Ignore lines beginning with 'rem '"
#define SHELL_HELP_RETVAL		"Print return value of most recent command"
#define SHELL_HELP_CLEAR		"Clear screen."
#define SHELL_HELP_BACKENDS		"List active shell backends.\n"
#define SHELL_HELP_BACKSPACE_MODE	"Toggle backspace key mode.\n"	      \
	"Some terminals are not sending separate escape code for "	      \
	"backspace and delete button. This command forces shell to interpret" \
	" delete key as backspace."
#define SHELL_HELP_BACKSPACE_MODE_BACKSPACE	"Set different escape"	 \
	" code for backspace and delete key."
#define SHELL_HELP_BACKSPACE_MODE_DELETE	"Set the same escape"	 \
	" code for backspace and delete key."

#define SHELL_HELP_COLORS		"Toggle colored syntax."
#define SHELL_HELP_COLORS_OFF		"Disable colored syntax."
#define SHELL_HELP_COLORS_ON		"Enable colored syntax."
#define SHELL_HELP_VT100		"Toggle vt100 commands."
#define SHELL_HELP_VT100_OFF		"Disable vt100 commands."
#define SHELL_HELP_VT100_ON		"Enable vt100 commands."
#define SHELL_HELP_PROMPT		"Toggle prompt."
#define SHELL_HELP_PROMPT_OFF		"Disable prompt."
#define SHELL_HELP_PROMPT_ON		"Enable prompt."
#define SHELL_HELP_STATISTICS		"Shell statistics."
#define SHELL_HELP_STATISTICS_SHOW	\
	"Get shell statistics for the Logger module."
#define SHELL_HELP_STATISTICS_RESET	\
	"Reset shell statistics for the Logger module."
#define SHELL_HELP_RESIZE						\
	"Console gets terminal screen size or assumes default in case"	\
	" the readout fails. It must be executed after each terminal"	\
	" width change to ensure correct text display."
#define SHELL_HELP_RESIZE_DEFAULT				\
	"Assume 80 chars screen width and send this setting "	\
	"to the terminal."
#define SHELL_HELP_HISTORY	"Command history."
#define SHELL_HELP_ECHO		"Toggle shell echo."
#define SHELL_HELP_ECHO_ON	"Enable shell echo."
#define SHELL_HELP_ECHO_OFF	\
	"Disable shell echo. Editing keys and meta-keys are not handled"

#define SHELL_HELP_SELECT	"Selects new root command. In order for the " \
	"command to be selected, it must meet the criteria:\n"		      \
	" - it is a static command\n"					      \
	" - it is not preceded by a dynamic command\n"			      \
	" - it accepts arguments\n"					      \
	"Return to the main command tree is done by pressing alt+r."

#define SHELL_HELP_SHELL		"Useful, not Unix-like shell commands."
#define SHELL_HELP_HELP			"Prints help message."

#define SHELL_MSG_UNKNOWN_PARAMETER	" unknown parameter: "

#define SHELL_MAX_TERMINAL_SIZE		(250u)

/* 10 == {esc, [, 2, 5, 0, ;, 2, 5, 0, '\0'} */
#define SHELL_CURSOR_POSITION_BUFFER	(10u)

/* Function reads cursor position from terminal. */
static int cursor_position_get(const struct shell *sh, uint16_t *x, uint16_t *y)
{
	uint16_t buff_idx = 0U;
	size_t cnt;
	char c = 0;

	*x = 0U;
	*y = 0U;

	memset(sh->ctx->temp_buff, 0, sizeof(sh->ctx->temp_buff));

	/* escape code asking terminal about its size */
	static char const cmd_get_terminal_size[] = "\033[6n";

	z_shell_raw_fprintf(sh->fprintf_ctx, cmd_get_terminal_size);

	/* fprintf buffer needs to be flushed to start sending prepared
	 * escape code to the terminal.
	 */
	z_transport_buffer_flush(sh);

	/* timeout for terminal response = ~1s */
	for (uint16_t i = 0; i < 1000; i++) {
		do {
			(void)sh->iface->api->read(sh->iface, &c,
						      sizeof(c), &cnt);
			if (cnt == 0) {
				k_busy_wait(1000);
				break;
			}
			if ((c != SHELL_VT100_ASCII_ESC) &&
			    (sh->ctx->temp_buff[0] !=
					    SHELL_VT100_ASCII_ESC)) {
				continue;
			}

			if (c == 'R') { /* End of response from the terminal. */
				sh->ctx->temp_buff[buff_idx] = '\0';
				if (sh->ctx->temp_buff[1] != '[') {
					sh->ctx->temp_buff[0] = 0;
					return -EIO;
				}

				/* Index start position in the buffer where 'y'
				 * is stored.
				 */
				buff_idx = 2U;

				while (sh->ctx->temp_buff[buff_idx] != ';') {
					*y = *y * 10U +
					(sh->ctx->temp_buff[buff_idx++] -
									  '0');
					if (buff_idx >=
						CONFIG_SHELL_CMD_BUFF_SIZE) {
						return -EMSGSIZE;
					}
				}

				if (++buff_idx >= CONFIG_SHELL_CMD_BUFF_SIZE) {
					return -EIO;
				}

				while (sh->ctx->temp_buff[buff_idx]
							     != '\0') {
					*x = *x * 10U +
					(sh->ctx->temp_buff[buff_idx++] -
									   '0');

					if (buff_idx >=
						CONFIG_SHELL_CMD_BUFF_SIZE) {
						return -EMSGSIZE;
					}
				}
				/* horizontal cursor position */
				if (*x > SHELL_MAX_TERMINAL_SIZE) {
					*x = SHELL_MAX_TERMINAL_SIZE;
				}

				/* vertical cursor position */
				if (*y > SHELL_MAX_TERMINAL_SIZE) {
					*y = SHELL_MAX_TERMINAL_SIZE;
				}

				sh->ctx->temp_buff[0] = 0;

				return 0;
			}

			sh->ctx->temp_buff[buff_idx] = c;

			if (++buff_idx > SHELL_CURSOR_POSITION_BUFFER - 1) {
				sh->ctx->temp_buff[0] = 0;
				/* data_buf[SHELL_CURSOR_POSITION_BUFFER - 1]
				 * is reserved for '\0'
				 */
				return -ENOMEM;
			}

		} while (cnt > 0);
	}

	return -ETIMEDOUT;
}

/* Function gets terminal width and height. */
static int terminal_size_get(const struct shell *sh)
{
	uint16_t x; /* horizontal position */
	uint16_t y; /* vertical position */
	int ret_val = 0;

	z_cursor_save(sh);

	/* Assumption: terminal width and height < 999. */
	/* Move to last column. */
	z_shell_op_cursor_vert_move(sh, -SHELL_MAX_TERMINAL_SIZE);
	/* Move to last row. */
	z_shell_op_cursor_horiz_move(sh, SHELL_MAX_TERMINAL_SIZE);

	if (cursor_position_get(sh, &x, &y) == 0) {
		sh->ctx->vt100_ctx.cons.terminal_wid = x;
		sh->ctx->vt100_ctx.cons.terminal_hei = y;
	} else {
		ret_val = -ENOTSUP;
	}

	z_cursor_restore(sh);
	return ret_val;
}

static int cmd_comment(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return 0;
}

static int cmd_clear(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argv);

	Z_SHELL_VT100_CMD(sh, SHELL_VT100_CURSORHOME);
	Z_SHELL_VT100_CMD(sh, SHELL_VT100_CLEARSCREEN);

	return 0;
}

static int cmd_backends(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	uint16_t cnt = 0;

	shell_print(sh, "Active shell backends:");
	STRUCT_SECTION_FOREACH(shell, obj) {
		shell_print(sh, "  %2d. :%s (%s)", cnt++, obj->ctx->prompt, obj->name);
	}

	return 0;
}

static int cmd_bacskpace_mode_backspace(const struct shell *sh, size_t argc,
					char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	z_flag_mode_delete_set(sh, false);

	return 0;
}

static int cmd_bacskpace_mode_delete(const struct shell *sh, size_t argc,
				      char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	z_flag_mode_delete_set(sh, true);

	return 0;
}

static int cmd_colors_off(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	z_flag_use_colors_set(sh, false);

	return 0;
}

static int cmd_colors_on(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(argv);

	z_flag_use_colors_set(sh, true);

	return 0;
}

static int cmd_vt100_off(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	z_flag_use_vt100_set(sh, false);

	return 0;
}

static int cmd_vt100_on(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(argv);

	z_flag_use_vt100_set(sh, true);

	return 0;
}

static int cmd_prompt_off(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_prompt_change(sh, "");

	return 0;
}

static int cmd_prompt_on(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(argv);

	shell_prompt_change(sh, sh->default_prompt);

	return 0;
}

static int cmd_echo_off(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	z_flag_echo_set(sh, false);

	return 0;
}

static int cmd_echo_on(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	z_flag_echo_set(sh, true);

	return 0;
}

static int cmd_echo(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 2) {
		shell_error(sh, "%s:%s%s", argv[0],
			    SHELL_MSG_UNKNOWN_PARAMETER, argv[1]);
		return -EINVAL;
	}

	shell_print(sh, "Echo status: %s",
		    z_flag_echo_get(sh) ? "on" : "off");

	return 0;
}

static int cmd_history(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	size_t i = 0;
	uint16_t len;

	while (1) {
		z_shell_history_get(sh->history, true,
				    sh->ctx->temp_buff, &len);

		if (len) {
			shell_print(sh, "[%3d] %s",
				    (int)i++, sh->ctx->temp_buff);

		} else {
			break;
		}
	}

	sh->ctx->temp_buff[0] = '\0';

	return 0;
}

static int cmd_shell_stats_show(const struct shell *sh, size_t argc,
				char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Lost logs: %lu", sh->stats->log_lost_cnt);

	return 0;
}

static int cmd_shell_stats_reset(const struct shell *sh,
				 size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	sh->stats->log_lost_cnt = 0;

	return 0;
}

static int cmd_resize_default(const struct shell *sh,
			      size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	Z_SHELL_VT100_CMD(sh, SHELL_VT100_SETCOL_80);
	sh->ctx->vt100_ctx.cons.terminal_wid = CONFIG_SHELL_DEFAULT_TERMINAL_WIDTH;
	sh->ctx->vt100_ctx.cons.terminal_hei = CONFIG_SHELL_DEFAULT_TERMINAL_HEIGHT;

	return 0;
}

static int cmd_resize(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	if (argc != 1) {
		shell_error(sh, "%s:%s%s", argv[0],
			    SHELL_MSG_UNKNOWN_PARAMETER, argv[1]);
		return -EINVAL;
	}

	err = terminal_size_get(sh);
	if (err != 0) {
		sh->ctx->vt100_ctx.cons.terminal_wid =
				CONFIG_SHELL_DEFAULT_TERMINAL_WIDTH;
		sh->ctx->vt100_ctx.cons.terminal_hei =
				CONFIG_SHELL_DEFAULT_TERMINAL_HEIGHT;
		shell_warn(sh, "No response from the terminal, assumed 80x24"
			   " screen size");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_get_retval(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "%d", shell_get_return_value(sh));
	return 0;
}

static bool no_args(const struct shell_static_entry *entry)
{
	return (entry->args.mandatory == 1) && (entry->args.optional == 0);
}

static int cmd_select(const struct shell *sh, size_t argc, char **argv)
{
	const struct shell_static_entry *candidate = NULL;
	struct shell_static_entry entry;
	size_t matching_argc;

	argc--;
	argv = argv + 1;
	candidate = z_shell_get_last_command(sh->ctx->selected_cmd,
					     argc, (const char **)argv,
					     &matching_argc, &entry, true);

	if ((candidate != NULL) && !no_args(candidate)
	    && (argc == matching_argc)) {
		sh->ctx->selected_cmd = candidate;
		return 0;
	}

	shell_error(sh, "Cannot select command");

	return -EINVAL;
}

SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_colors,
	SHELL_COND_CMD_ARG(CONFIG_SHELL_VT100_COMMANDS, off, NULL,
			   SHELL_HELP_COLORS_OFF, cmd_colors_off, 1, 0),
	SHELL_COND_CMD_ARG(CONFIG_SHELL_VT100_COMMANDS, on, NULL,
			   SHELL_HELP_COLORS_ON, cmd_colors_on, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_vt100,
	SHELL_COND_CMD_ARG(CONFIG_SHELL_VT100_COMMANDS, off, NULL,
			   SHELL_HELP_VT100_OFF, cmd_vt100_off, 1, 0),
	SHELL_COND_CMD_ARG(CONFIG_SHELL_VT100_COMMANDS, on, NULL,
			   SHELL_HELP_VT100_ON, cmd_vt100_on, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_prompt,
	SHELL_CMD_ARG(off, NULL, SHELL_HELP_PROMPT_OFF, cmd_prompt_off, 1, 0),
	SHELL_CMD_ARG(on, NULL, SHELL_HELP_PROMPT_ON, cmd_prompt_on, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_echo,
	SHELL_CMD_ARG(off, NULL, SHELL_HELP_ECHO_OFF, cmd_echo_off, 1, 0),
	SHELL_CMD_ARG(on, NULL, SHELL_HELP_ECHO_ON, cmd_echo_on, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_shell_stats,
	SHELL_CMD_ARG(reset, NULL, SHELL_HELP_STATISTICS_RESET,
			cmd_shell_stats_reset, 1, 0),
	SHELL_CMD_ARG(show, NULL, SHELL_HELP_STATISTICS_SHOW,
			cmd_shell_stats_show, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_backspace_mode,
	SHELL_CMD_ARG(backspace, NULL, SHELL_HELP_BACKSPACE_MODE_BACKSPACE,
			cmd_bacskpace_mode_backspace, 1, 0),
	SHELL_CMD_ARG(delete, NULL, SHELL_HELP_BACKSPACE_MODE_DELETE,
			cmd_bacskpace_mode_delete, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_shell,
	SHELL_CMD_ARG(backends, NULL, SHELL_HELP_BACKENDS, cmd_backends, 1, 0),
	SHELL_CMD(backspace_mode, &m_sub_backspace_mode,
			SHELL_HELP_BACKSPACE_MODE, NULL),
	SHELL_COND_CMD(CONFIG_SHELL_VT100_COMMANDS, colors, &m_sub_colors,
		       SHELL_HELP_COLORS, NULL),
	SHELL_COND_CMD(CONFIG_SHELL_VT100_COMMANDS, vt100, &m_sub_vt100,
		       SHELL_HELP_VT100, NULL),
	SHELL_CMD(prompt, &m_sub_prompt, SHELL_HELP_PROMPT, NULL),
	SHELL_CMD_ARG(echo, &m_sub_echo, SHELL_HELP_ECHO, cmd_echo, 1, 1),
	SHELL_COND_CMD(CONFIG_SHELL_STATS, stats, &m_sub_shell_stats,
			SHELL_HELP_STATISTICS, NULL),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_resize,
	SHELL_CMD_ARG(default, NULL, SHELL_HELP_RESIZE_DEFAULT,
			cmd_resize_default, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_COND_CMD_REGISTER(CONFIG_SHELL_VT100_COMMANDS, rem, NULL,
				SHELL_HELP_COMMENT, cmd_comment);
SHELL_COND_CMD_ARG_REGISTER(CONFIG_SHELL_VT100_COMMANDS, clear, NULL,
			    SHELL_HELP_CLEAR, cmd_clear, 1, 0);
SHELL_CMD_REGISTER(shell, &m_sub_shell, SHELL_HELP_SHELL, NULL);
SHELL_COND_CMD_ARG_REGISTER(CONFIG_SHELL_HISTORY, history, NULL,
			SHELL_HELP_HISTORY, cmd_history, 1, 0);
SHELL_COND_CMD_ARG_REGISTER(CONFIG_SHELL_CMDS_RESIZE, resize, &m_sub_resize,
			SHELL_HELP_RESIZE, cmd_resize, 1, 1);
SHELL_COND_CMD_ARG_REGISTER(CONFIG_SHELL_CMDS_SELECT, select, NULL,
			    SHELL_HELP_SELECT, cmd_select, 2,
			    SHELL_OPT_ARG_CHECK_SKIP);
SHELL_COND_CMD_ARG_REGISTER(CONFIG_SHELL_CMDS_RETURN_VALUE, retval, NULL,
			    SHELL_HELP_RETVAL, cmd_get_retval, 1, 0);
