/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <shell/shell.h>
#include "shell_utils.h"
#include "shell_ops.h"
#include "shell_vt100.h"

#define SHELL_HELP_CLEAR		"Clear screen."
#define SHELL_HELP_BACKSPACE_MODE	"Toggle backspace key mode.\n" \
	"Some terminals are not sending separate escape code for"	 \
	"backspace and delete button. Hence backspace is not working as" \
	"expected. This command can force shell to interpret delete"	 \
	" escape as backspace."
#define SHELL_HELP_BACKSPACE_MODE_BACKSPACE	"Set different escape"	 \
	" code for backspace and delete key."
#define SHELL_HELP_BACKSPACE_MODE_DELETE	"Set the same escape"	 \
	" code for backspace and delete key."

#define SHELL_HELP_COLORS		"Toggle colored syntax."
#define SHELL_HELP_COLORS_OFF		"Disable colored syntax."
#define SHELL_HELP_COLORS_ON		"Enable colored syntax."
#define SHELL_HELP_STATISTICS		"Shell statistics."
#define SHELL_HELP_STATISTICS_SHOW	\
	"Get shell statistics for the Logger module."
#define SHELL_HELP_STATISTICS_RESET	\
	"Reset shell statistics for the Logger module."
#define SHELL_HELP_RESIZE						\
	"Console gets terminal screen size or assumes 80 in case "	\
	"the readout fails. It must be executed after each terminal "	\
	"width change to ensure correct text display."
#define SHELL_HELP_RESIZE_DEFAULT				\
	"Assume 80 chars screen width and send this setting "	\
	"to the terminal."
#define SHELL_HELP_HISTORY	"Command history."
#define SHELL_HELP_ECHO		"Toggle shell echo."
#define SHELL_HELP_ECHO_ON	"Enable shell echo."
#define SHELL_HELP_ECHO_OFF	\
	"Disable shell echo. Arrows and buttons: Backspace, Delete, End, " \
	"Home, Insert are not handled."
#define SHELL_HELP_SHELL		"Useful, not Unix-like shell commands."

#define SHELL_MSG_UNKNOWN_PARAMETER	" unknown parameter: "

#define SHELL_MAX_TERMINAL_SIZE		(250u)

/* 10 == {esc, [, 2, 5, 0, ;, 2, 5, 0, '\0'} */
#define SHELL_CURSOR_POSITION_BUFFER	(10u)

/* Function reads cursor position from terminal. */
static int cursor_position_get(const struct shell *shell, u16_t *x, u16_t *y)
{
	u16_t buff_idx = 0;
	size_t cnt;
	char c = 0;

	*x = 0;
	*y = 0;

	memset(shell->ctx->temp_buff, 0, sizeof(shell->ctx->temp_buff));

	/* escape code asking terminal about its size */
	static char const cmd_get_terminal_size[] = "\033[6n";

	shell_raw_fprintf(shell->fprintf_ctx, cmd_get_terminal_size);

	/* fprintf buffer needs to be flushed to start sending prepared
	 * escape code to the terminal.
	 */
	shell_fprintf_buffer_flush(shell->fprintf_ctx);

	/* timeout for terminal response = ~1s */
	for (u16_t i = 0; i < 1000; i++) {
		do {
			(void)shell->iface->api->read(shell->iface, &c,
						      sizeof(c), &cnt);
			if (cnt == 0) {
				k_sleep(1);
				break;
			}
			if ((c != SHELL_VT100_ASCII_ESC) &&
			    (shell->ctx->temp_buff[0] !=
					    SHELL_VT100_ASCII_ESC)) {
				continue;
			}

			if (c == 'R') { /* End of response from the terminal. */
				shell->ctx->temp_buff[buff_idx] = '\0';
				if (shell->ctx->temp_buff[1] != '[') {
					shell->ctx->temp_buff[0] = 0;
					return -EIO;
				}

				/* Index start position in the buffer where 'y'
				 * is stored.
				 */
				buff_idx = 2;

				while (shell->ctx->temp_buff[buff_idx] != ';') {
					*y = *y * 10 +
					(shell->ctx->temp_buff[buff_idx++] -
									  '0');
					if (buff_idx >=
						CONFIG_SHELL_CMD_BUFF_SIZE) {
						return -EMSGSIZE;
					}
				}

				if (++buff_idx >= CONFIG_SHELL_CMD_BUFF_SIZE) {
					return -EIO;
				}

				while (shell->ctx->temp_buff[buff_idx]
							     != '\0') {
					*x = *x * 10 +
					(shell->ctx->temp_buff[buff_idx++] -
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

				shell->ctx->temp_buff[0] = 0;

				return 0;
			}

			shell->ctx->temp_buff[buff_idx] = c;

			if (++buff_idx > SHELL_CURSOR_POSITION_BUFFER - 1) {
				shell->ctx->temp_buff[0] = 0;
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
static int terminal_size_get(const struct shell *shell)
{
	u16_t x; /* horizontal position */
	u16_t y; /* vertical position */
	int ret_val = 0;

	cursor_save(shell);

	/* Assumption: terminal width and height < 999. */
	/* Move to last column. */
	shell_op_cursor_vert_move(shell, -SHELL_MAX_TERMINAL_SIZE);
	/* Move to last row. */
	shell_op_cursor_horiz_move(shell, SHELL_MAX_TERMINAL_SIZE);

	if (cursor_position_get(shell, &x, &y) == 0) {
		shell->ctx->vt100_ctx.cons.terminal_wid = x;
		shell->ctx->vt100_ctx.cons.terminal_hei = y;
	} else {
		ret_val = -ENOTSUP;
	}

	cursor_restore(shell);
	return ret_val;
}

static int cmd_clear(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argv);

	int ret = shell_cmd_precheck(shell, (argc == 1), NULL, 0);

	if (ret) {
		return ret;
	}

	SHELL_VT100_CMD(shell, SHELL_VT100_CURSORHOME);
	SHELL_VT100_CMD(shell, SHELL_VT100_CLEARSCREEN);

	return 0;
}

static int cmd_shell(const struct shell *shell, size_t argc, char **argv)
{
	int ret = shell_cmd_precheck(shell, (argc == 2), NULL, 0);

	if (ret) {
		return ret;
	}

	shell_fprintf(shell, SHELL_ERROR, "%s:%s%s\n", argv[0],
		      SHELL_MSG_UNKNOWN_PARAMETER, argv[1]);

	return -EINVAL;
}

static int cmd_bacskpace_mode_backspace(const struct shell *shell, size_t argc,
					char **argv)
{
	ARG_UNUSED(argv);

	int ret = shell_cmd_precheck(shell, (argc == 1), NULL, 0);

	if (ret == 0) {
		shell->ctx->internal.flags.mode_delete = 0;
	}

	return ret;
}

static int cmd_bacskpace_mode_delete(const struct shell *shell, size_t argc,
				      char **argv)
{
	ARG_UNUSED(argv);

	int ret = shell_cmd_precheck(shell, (argc == 1), NULL, 0);

	if (ret == 0) {
		shell->ctx->internal.flags.mode_delete = 1;
	}

	return ret;
}

static int cmd_bacskpace_mode(const struct shell *shell, size_t argc,
			      char **argv)
{
	int ret = shell_cmd_precheck(shell, (argc == 2), NULL, 0);

	if (ret) {
		return ret;
	}

	shell_fprintf(shell, SHELL_ERROR, "%s:%s%s\n", argv[0],
		      SHELL_MSG_UNKNOWN_PARAMETER, argv[1]);

	return -EINVAL;
}

static int cmd_colors_off(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argv);

	int ret = shell_cmd_precheck(shell, (argc == 1), NULL, 0);

	if (ret == 0) {
		shell->ctx->internal.flags.use_colors = 0;
	}

	return ret;
}

static int cmd_colors_on(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argv);

	int ret = shell_cmd_precheck(shell, (argc == 1), NULL, 0);

	if (ret == 0) {
		shell->ctx->internal.flags.use_colors = 1;
	}

	return ret;
}

static int cmd_colors(const struct shell *shell, size_t argc, char **argv)
{
	int ret = shell_cmd_precheck(shell, (argc == 2), NULL, 0);

	if (ret) {
		return ret;
	}

	shell_fprintf(shell, SHELL_ERROR, "%s:%s%s\n", argv[0],
		      SHELL_MSG_UNKNOWN_PARAMETER, argv[1]);

	return -EINVAL;
}

static int cmd_echo_off(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argv);

	int ret = shell_cmd_precheck(shell, (argc == 1), NULL, 0);

	if (ret == 0) {
		shell->ctx->internal.flags.echo = 0;
	}

	return ret;
}

static int cmd_echo_on(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argv);

	int ret = shell_cmd_precheck(shell, (argc == 1), NULL, 0);

	if (ret == 0) {
		shell->ctx->internal.flags.echo = 1;
	}

	return ret;
}

static int cmd_echo(const struct shell *shell, size_t argc, char **argv)
{
	int ret = shell_cmd_precheck(shell, (argc <= 2), NULL, 0);

	if (ret) {
		return ret;
	}

	if (argc == 2) {
		shell_fprintf(shell, SHELL_ERROR, "%s:%s%s\n", argv[0],
			      SHELL_MSG_UNKNOWN_PARAMETER, argv[1]);
		return -EINVAL;
	}

	shell_fprintf(shell, SHELL_NORMAL, "Echo status: %s\n",
		      flag_echo_is_set(shell) ? "on" : "off");

	return 0;
}

static int cmd_help(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_NORMAL,
		"Please press the <Tab> button to see all available commands.\n"
		"You can also use the <Tab> button to prompt or auto-complete"
		" all commands or its subcommands.\n"
		"You can try to call commands with <-h> or <--help> parameter"
		" for more information.\n");

	return 0;
}

static int cmd_history(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argv);

	size_t i = 0;
	size_t len;
	int ret;

	if (!IS_ENABLED(CONFIG_SHELL_HISTORY)) {
		shell_fprintf(shell, SHELL_ERROR, "Command not supported.\n");
		return -ENOEXEC;
	}

	ret = shell_cmd_precheck(shell, (argc == 1), NULL, 0);

	if (ret) {
		return ret;
	}

	while (1) {
		shell_history_get(shell->history, true,
				  shell->ctx->temp_buff, &len);

		if (len) {
			shell_fprintf(shell, SHELL_NORMAL, "[%3d] %s\n",
				      i++, shell->ctx->temp_buff);

		} else {
			break;
		}
	}

	shell->ctx->temp_buff[0] = '\0';

	return 0;
}

static int cmd_shell_stats_show(const struct shell *shell, size_t argc,
				char **argv)
{
	ARG_UNUSED(argv);

	if (!IS_ENABLED(CONFIG_SHELL_STATS)) {
		shell_fprintf(shell, SHELL_ERROR, "Command not supported.\n");
		return -ENOEXEC;
	}

	int ret = shell_cmd_precheck(shell, (argc == 1), NULL, 0);

	if (ret == 0) {
		shell_fprintf(shell, SHELL_NORMAL, "Lost logs: %u\n",
			      shell->stats->log_lost_cnt);
	}

	return ret;
}

static int cmd_shell_stats_reset(const struct shell *shell,
				 size_t argc, char **argv)
{
	ARG_UNUSED(argv);

	if (!IS_ENABLED(CONFIG_SHELL_STATS)) {
		shell_fprintf(shell, SHELL_ERROR, "Command not supported.\n");
		return -ENOEXEC;
	}

	int ret = shell_cmd_precheck(shell, (argc == 1), NULL, 0);

	if (ret == 0) {
		shell->stats->log_lost_cnt = 0;
	}

	return ret;
}

static int cmd_shell_stats(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	if (shell_help_requested(shell)) {
		shell_help_print(shell, NULL, 0);
		return 1;
	} else if (argc == 1) {
		shell_help_print(shell, NULL, 0);
	} else {
		shell_fprintf(shell, SHELL_ERROR, "%s:%s%s\n", argv[0],
			      SHELL_MSG_UNKNOWN_PARAMETER, argv[1]);
	}

	return -EINVAL;
}

static int cmd_resize_default(const struct shell *shell,
			      size_t argc, char **argv)
{
	ARG_UNUSED(argv);

	int ret = shell_cmd_precheck(shell, (argc == 1), NULL, 0);

	if (ret == 0) {
		SHELL_VT100_CMD(shell, SHELL_VT100_SETCOL_80);
		shell->ctx->vt100_ctx.cons.terminal_wid =
						   SHELL_DEFAULT_TERMINAL_WIDTH;
		shell->ctx->vt100_ctx.cons.terminal_hei =
						  SHELL_DEFAULT_TERMINAL_HEIGHT;
	}

	return ret;
}

static int cmd_resize(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	if (!IS_ENABLED(CONFIG_SHELL_CMDS_RESIZE)) {
		shell_fprintf(shell, SHELL_ERROR, "Command not supported.\n");
		return -ENOEXEC;
	}

	err = shell_cmd_precheck(shell, (argc <= 2), NULL, 0);
	if (err) {
		return err;
	}

	if (argc != 1) {
		shell_fprintf(shell, SHELL_ERROR, "%s:%s%s\n", argv[0],
			      SHELL_MSG_UNKNOWN_PARAMETER, argv[1]);
		return -EINVAL;
	}

	err = terminal_size_get(shell);
	if (err != 0) {
		shell->ctx->vt100_ctx.cons.terminal_wid =
				SHELL_DEFAULT_TERMINAL_WIDTH;
		shell->ctx->vt100_ctx.cons.terminal_hei =
				SHELL_DEFAULT_TERMINAL_HEIGHT;
		shell_fprintf(shell, SHELL_WARNING,
			      "No response from the terminal, assumed 80x24 "
			      "screen size\n");
		return -ENOEXEC;
	}

	return 0;
}

SHELL_CREATE_STATIC_SUBCMD_SET(m_sub_colors)
{
	SHELL_CMD(off, NULL, SHELL_HELP_COLORS_OFF, cmd_colors_off),
	SHELL_CMD(on, NULL, SHELL_HELP_COLORS_ON, cmd_colors_on),
	SHELL_SUBCMD_SET_END
};

SHELL_CREATE_STATIC_SUBCMD_SET(m_sub_echo)
{
	SHELL_CMD(off, NULL, SHELL_HELP_ECHO_OFF, cmd_echo_off),
	SHELL_CMD(on, NULL, SHELL_HELP_ECHO_ON, cmd_echo_on),
	SHELL_SUBCMD_SET_END
};

SHELL_CREATE_STATIC_SUBCMD_SET(m_sub_shell_stats)
{
	SHELL_CMD(reset, NULL, SHELL_HELP_STATISTICS_RESET,
							 cmd_shell_stats_reset),
	SHELL_CMD(show, NULL, SHELL_HELP_STATISTICS_SHOW, cmd_shell_stats_show),
	SHELL_SUBCMD_SET_END
};

SHELL_CREATE_STATIC_SUBCMD_SET(m_sub_backspace_mode)
{
	SHELL_CMD(backspace, NULL, SHELL_HELP_BACKSPACE_MODE_BACKSPACE,
						  cmd_bacskpace_mode_backspace),
	SHELL_CMD(delete, NULL, SHELL_HELP_BACKSPACE_MODE_DELETE,
						     cmd_bacskpace_mode_delete),
	SHELL_SUBCMD_SET_END
};

SHELL_CREATE_STATIC_SUBCMD_SET(m_sub_shell)
{
	SHELL_CMD(backspace_mode, &m_sub_backspace_mode,
			SHELL_HELP_BACKSPACE_MODE, cmd_bacskpace_mode),
	SHELL_CMD(colors, &m_sub_colors, SHELL_HELP_COLORS, cmd_colors),
	SHELL_CMD(echo, &m_sub_echo, SHELL_HELP_ECHO, cmd_echo),
	SHELL_CMD(stats, &m_sub_shell_stats, SHELL_HELP_STATISTICS,
		  cmd_shell_stats),
	SHELL_SUBCMD_SET_END
};

SHELL_CREATE_STATIC_SUBCMD_SET(m_sub_resize)
{
	SHELL_CMD(default, NULL, SHELL_HELP_RESIZE_DEFAULT, cmd_resize_default),
	SHELL_SUBCMD_SET_END
};

SHELL_CMD_REGISTER(clear, NULL, SHELL_HELP_CLEAR, cmd_clear);
SHELL_CMD_REGISTER(shell, &m_sub_shell, SHELL_HELP_SHELL, cmd_shell);
SHELL_CMD_REGISTER(help, NULL, NULL, cmd_help);
SHELL_CMD_REGISTER(history, NULL, SHELL_HELP_HISTORY, cmd_history);
SHELL_CMD_REGISTER(resize, &m_sub_resize, SHELL_HELP_RESIZE, cmd_resize);
