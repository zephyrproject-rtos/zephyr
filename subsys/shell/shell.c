/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <shell/shell.h>
#include "shell_utils.h"
#include "shell_ops.h"
#include "shell_wildcard.h"
#include "shell_vt100.h"
#include <atomic.h>

/* 2 == 1 char for cmd + 1 char for '\0' */
#if (CONFIG_SHELL_CMD_BUFF_SIZE < 2)
	#error too small CONFIG_SHELL_CMD_BUFF_SIZE
#endif

#if (CONFIG_SHELL_PRINTF_BUFF_SIZE < 1)
	#error too small SHELL_PRINTF_BUFF_SIZE
#endif

#define SHELL_MSG_COMMAND_NOT_FOUND	": command not found"
#define SHELL_MSG_TAB_OVERFLOWED	\
	"Tab function: commands counter overflowed.\r\n"

#define SHELL_INIT_OPTION_PRINTER	(NULL)

/* Initial cursor position is: (1, 1). */
#define SHELL_INITIAL_CURS_POS		(1u)

static int shell_execute(const struct shell *shell);

extern const struct shell_cmd_entry __shell_root_cmds_start[0];
extern const struct shell_cmd_entry __shell_root_cmds_end[0];

static inline const struct shell_cmd_entry *shell_root_cmd_get(u32_t id)
{
	return &__shell_root_cmds_start[id];
}

static inline u32_t shell_root_cmd_count(void)
{
	return ((u8_t *)__shell_root_cmds_end -
			(u8_t *)__shell_root_cmds_start)/
				sizeof(struct shell_cmd_entry);
}

static inline void transport_buffer_flush(const struct shell *shell)
{
	shell_fprintf_buffer_flush(shell->fprintf_ctx);
}

static inline void help_flag_set(const struct shell *shell)
{
	shell->ctx->internal.flags.show_help = 1;
}
static inline void help_flag_clear(const struct shell *shell)
{
	shell->ctx->internal.flags.show_help = 0;
}

/* Function returns true if delete escape code shall be interpreted as
 * backspace.
 */
static inline bool flag_delete_mode_set(const struct shell *shell)
{
	return shell->ctx->internal.flags.mode_delete == 1 ? true : false;
}

static inline bool flag_processing_is_set(const struct shell *shell)
{
	return shell->ctx->internal.flags.processing == 1 ? true : false;
}

static inline void receive_state_change(const struct shell *shell,
					enum shell_receive_state state)
{
	shell->ctx->receive_state = state;
}

static void shell_cmd_buffer_clear(const struct shell *shell)
{
	shell->ctx->cmd_buff[0] = '\0'; /* clear command buffer */
	shell->ctx->cmd_buff_pos = 0;
	shell->ctx->cmd_buff_len = 0;
}

/* Function sends data stream to the shell instance. Each time before the
 * shell_write function is called, it must be ensured that IO buffer of fprintf
 * is flushed to avoid synchronization issues.
 * For that purpose, use function transport_buffer_flush(shell)
 */
static void shell_write(const struct shell *shell, const void *data,
			size_t length)
{
	__ASSERT_NO_MSG(shell && data);

	size_t offset = 0;
	size_t tmp_cnt;

	while (length) {
		int err = shell->iface->api->write(shell->iface,
				&((const u8_t *) data)[offset], length,
				&tmp_cnt);
		(void)err;
		__ASSERT_NO_MSG(err == 0);
		__ASSERT_NO_MSG(length >= tmp_cnt);
		offset += tmp_cnt;
		length -= tmp_cnt;
		if (tmp_cnt == 0 &&
		    (shell->ctx->state != SHELL_STATE_PANIC_MODE_ACTIVE)) {
			/* todo  semaphore pend*/
			if (IS_ENABLED(CONFIG_MULTITHREADING)) {
				k_poll(&shell->ctx->events[SHELL_SIGNAL_TXDONE],
				1, K_FOREVER);
			} else {
				/* Blocking wait in case of bare metal. */
				while (!shell->ctx->internal.flags.tx_rdy) {

				}
				shell->ctx->internal.flags.tx_rdy = 0;
			}
		}
	}
}

/* @brief Function shall be used to search commands.
 *
 * It moves the pointer entry to command of static command structure. If the
 * command cannot be found, the function will set entry to NULL.
 *
 *   @param command	Pointer to command which will be processed (no matter
 *			the root command).
 *   @param lvl		Level of the requested command.
 *   @param idx		Index of the requested command.
 *   @param entry	Pointer which points to subcommand[idx] after function
 *			execution.
 *   @param st_entry	Pointer to the structure where dynamic entry data can be
 *			stored.
 */
static void cmd_get(const struct shell_cmd_entry *command, size_t lvl,
		    size_t idx, const struct shell_static_entry **entry,
		    struct shell_static_entry *d_entry)
{
	__ASSERT_NO_MSG(entry != NULL);
	__ASSERT_NO_MSG(d_entry != NULL);

	if (lvl == SHELL_CMD_ROOT_LVL) {
		if (idx < shell_root_cmd_count()) {
			const struct shell_cmd_entry *cmd;

			cmd = shell_root_cmd_get(idx);
			*entry = cmd->u.entry;
		} else {
			*entry = NULL;
		}
		return;
	}

	if (command == NULL) {
		*entry = NULL;
		return;
	}

	if (command->is_dynamic) {
		command->u.dynamic_get(idx, d_entry);
		*entry = (d_entry->syntax != NULL) ? d_entry : NULL;
	} else {
		*entry = (command->u.entry[idx].syntax != NULL) ?
				&command->u.entry[idx] : NULL;
	}
}

static void vt100_color_set(const struct shell *shell,
			    enum shell_vt100_color color)
{

	if (shell->ctx->vt100_ctx.col.col == color) {
		return;
	}

	shell->ctx->vt100_ctx.col.col = color;

	if (color != SHELL_NORMAL) {

		u8_t cmd[] = SHELL_VT100_COLOR(color - 1);

		shell_raw_fprintf(shell->fprintf_ctx, "%s", cmd);
	} else {
		static const u8_t cmd[] = SHELL_VT100_MODESOFF;

		shell_raw_fprintf(shell->fprintf_ctx, "%s", cmd);
	}
}

static void vt100_bgcolor_set(const struct shell *shell,
			      enum shell_vt100_color bgcolor)
{
	if ((bgcolor == SHELL_NORMAL) ||
	    (shell->ctx->vt100_ctx.col.bgcol == bgcolor)) {
		return;
	}

	/* -1 because default value is first in enum */
	u8_t cmd[] = SHELL_VT100_BGCOLOR(bgcolor - 1);

	shell->ctx->vt100_ctx.col.bgcol = bgcolor;
	shell_raw_fprintf(shell->fprintf_ctx, "%s", cmd);

}

static inline void vt100_colors_store(const struct shell *shell,
				      struct shell_vt100_colors *color)
{
	memcpy(color, &shell->ctx->vt100_ctx.col, sizeof(*color));
}

static void vt100_colors_restore(const struct shell *shell,
				 const struct shell_vt100_colors *color)
{
	vt100_color_set(shell, color->col);
	vt100_bgcolor_set(shell, color->bgcol);
}

static void shell_state_set(const struct shell *shell, enum shell_state state)
{
	shell->ctx->state = state;

	if (state == SHELL_STATE_ACTIVE) {
		shell_cmd_buffer_clear(shell);
		shell_fprintf(shell, SHELL_INFO, "%s", shell->prompt);
	}
}

static void tab_item_print(const struct shell *shell, const char *option,
			   u16_t longest_option)
{
	static const char *tab = "  ";
	u16_t columns;
	u16_t diff;

	/* Function initialization has been requested. */
	if (option == NULL) {
		shell->ctx->vt100_ctx.printed_cmd = 0;
		return;
	}

	longest_option += shell_strlen(tab);

	columns = (shell->ctx->vt100_ctx.cons.terminal_wid
			- shell_strlen(tab)) / longest_option;
	diff = longest_option - shell_strlen(option);

	if (shell->ctx->vt100_ctx.printed_cmd++ % columns == 0) {
		shell_fprintf(shell, SHELL_OPTION, "\r\n%s%s", tab, option);
	} else {
		shell_fprintf(shell, SHELL_OPTION, "%s", option);
	}

	shell_op_cursor_horiz_move(shell, diff);
}

static void history_init(const struct shell *shell)
{
	if (!IS_ENABLED(CONFIG_SHELL_HISTORY)) {
		return;
	}

	shell_history_init(shell->history);
}

static void history_purge(const struct shell *shell)
{
	if (!IS_ENABLED(CONFIG_SHELL_HISTORY)) {
		return;
	}

	shell_history_purge(shell->history);
}

static void history_mode_exit(const struct shell *shell)
{
	if (!IS_ENABLED(CONFIG_SHELL_HISTORY)) {
		return;
	}

	shell_history_mode_exit(shell->history);
}

static void history_put(const struct shell *shell, u8_t *line, size_t length)
{
	if (!IS_ENABLED(CONFIG_SHELL_HISTORY)) {
		return;
	}

	shell_history_put(shell->history, line, length);
}

static void history_handle(const struct shell *shell, bool up)
{
	bool history_mode;
	size_t len;

	/*optional feature */
	if (!IS_ENABLED(CONFIG_SHELL_HISTORY)) {
		return;
	}

	/* Backup command if history is entered */
	if (!shell_history_active(shell->history)) {
		if (up) {
			u16_t cmd_len = shell_strlen(shell->ctx->cmd_buff);

			if (cmd_len) {
				strcpy(shell->ctx->temp_buff,
				       shell->ctx->cmd_buff);
			} else {
				shell->ctx->temp_buff[0] = '\0';
			}
		} else {
			/* Pressing 'down' not in history mode has no effect. */
			return;
		}
	}

	/* Start by checking if history is not empty. */
	history_mode = shell_history_get(shell->history, true,
					 shell->ctx->cmd_buff, &len);

	/* On exiting history mode print backed up command. */
	if (!history_mode) {
		strcpy(shell->ctx->cmd_buff, shell->ctx->temp_buff);
		len = shell_strlen(shell->ctx->cmd_buff);
	}

	if (len) {
		shell_op_cursor_home_move(shell);
		clear_eos(shell);
		shell_fprintf(shell, SHELL_NORMAL, "%s", shell->ctx->cmd_buff);
		shell->ctx->cmd_buff_pos = len;
		shell->ctx->cmd_buff_len = len;
		shell_op_cond_next_line(shell);
	}
}

static const struct shell_static_entry *find_cmd(
					     const struct shell_cmd_entry *cmd,
					     size_t lvl,
					     char *cmd_str,
					     struct shell_static_entry *d_entry)
{
	const struct shell_static_entry *entry = NULL;
	size_t idx = 0;

	do {
		cmd_get(cmd, lvl, idx++, &entry, d_entry);
		if (entry && (strcmp(cmd_str, entry->syntax) == 0)) {
			return entry;
		}
	} while (entry);

	return entry;
}

/** @brief Function for getting last valid command in list of arguments. */
static const struct shell_static_entry *get_last_command(
					     const struct shell *shell,
					     size_t argc,
					     char *argv[],
					     size_t *match_arg,
					     struct shell_static_entry *d_entry)
{
	const struct shell_static_entry *prev_entry = NULL;
	const struct shell_cmd_entry *prev_cmd = NULL;
	const struct shell_static_entry *entry = NULL;
	*match_arg = SHELL_CMD_ROOT_LVL;

	while (*match_arg < argc) {

		if (IS_ENABLED(CONFIG_SHELL_WILDCARD)) {
			/* ignore wildcard argument */
			if (shell_wildcard_character_exist(argv[*match_arg])) {
				(*match_arg)++;
				continue;
			}
		}

		entry = find_cmd(prev_cmd, *match_arg, argv[*match_arg],
				 d_entry);
		if (entry) {
			prev_cmd = entry->subcmd;
			prev_entry = entry;
			(*match_arg)++;
		} else {
			entry = NULL;
			break;
		}
	}

	return entry;
}

static inline u16_t completion_space_get(const struct shell *shell)
{
	u16_t space = (CONFIG_SHELL_CMD_BUFF_SIZE - 1) -
			shell->ctx->cmd_buff_len;
	return space;
}

/* Prepare arguments and return number of space available for completion. */
static bool shell_tab_prepare(const struct shell *shell,
			      const struct shell_static_entry **cmd,
			      char **argv, size_t *argc,
			      size_t *complete_arg_idx,
			      struct shell_static_entry *d_entry)
{
	u16_t compl_space = completion_space_get(shell);
	size_t search_argc;

	if (compl_space == 0) {
		return false;
	}

	/* If the Tab key is pressed, "history mode" must be terminated because
	 * tab and history handlers are sharing the same array: temp_buff.
	 */
	history_mode_exit(shell);

	/* Copy command from its beginning to cursor position. */
	memcpy(shell->ctx->temp_buff, shell->ctx->cmd_buff,
			shell->ctx->cmd_buff_pos);
	shell->ctx->temp_buff[shell->ctx->cmd_buff_pos] = '\0';

	/* Create argument list. */
	(void)shell_make_argv(argc, argv, shell->ctx->temp_buff,
			      CONFIG_SHELL_ARGC_MAX);

	/* If last command is not completed (followed by space) it is treated
	 * as uncompleted one.
	 */
	int space = isspace((int)shell->ctx->cmd_buff[
						shell->ctx->cmd_buff_pos - 1]);

	/* root command completion */
	if ((*argc == 0) || ((space == 0) && (*argc == 1))) {
		*complete_arg_idx = SHELL_CMD_ROOT_LVL;
		*cmd = NULL;
		return true;
	}

	search_argc = space ? *argc : *argc - 1;

	*cmd = get_last_command(shell, search_argc, argv, complete_arg_idx,
				d_entry);

	/* if search_argc == 0 (empty command line) get_last_command will return
	 * NULL tab is allowed, otherwise not.
	 */
	if ((*cmd == NULL) && (search_argc != 0)) {
		return false;
	}

	return true;
}

static inline bool is_completion_candidate(const char *candidate,
					   const char *str, size_t len)
{
	return (strncmp(candidate, str, len) == 0) ? true : false;
}

static void find_completion_candidates(const struct shell_static_entry *cmd,
				       const char *incompl_cmd,
				       size_t *first_idx, size_t *cnt,
				       u16_t *longest)
{
	size_t incompl_cmd_len = shell_strlen(incompl_cmd);
	const struct shell_static_entry *candidate;
	struct shell_static_entry dynamic_entry;
	bool found = false;
	size_t idx = 0;

	*longest = 0;
	*cnt = 0;

	while (true) {
		cmd_get(cmd ? cmd->subcmd : NULL, cmd ? 1 : 0,
			idx, &candidate, &dynamic_entry);

		if (!candidate) {
			break;
		}

		if (is_completion_candidate(candidate->syntax, incompl_cmd,
				incompl_cmd_len)) {
			size_t slen = strlen(candidate->syntax);

			*longest = (slen > *longest) ? slen : *longest;
			(*cnt)++;

			if (!found) {
				*first_idx = idx;
			}

			found = true;
		}
		idx++;
	}
}

static void autocomplete(const struct shell *shell,
			 const struct shell_static_entry *cmd,
			 const char *arg,
			 size_t subcmd_idx)
{
	const struct shell_static_entry *match;
	size_t arg_len = shell_strlen(arg);
	size_t cmd_len;

	/* shell->ctx->active_cmd can be safely used outside of command context
	 * to save stack
	 */
	cmd_get(cmd ? cmd->subcmd : NULL, cmd ? 1 : 0,
			subcmd_idx, &match, &shell->ctx->active_cmd);
	cmd_len = shell_strlen(match->syntax);

	/* no exact match found */
	if (cmd_len != arg_len) {
		shell_op_completion_insert(shell,
					   match->syntax + arg_len,
					   cmd_len - arg_len);
	}

	/* Next character in the buffer is not 'space'. */
	if (!isspace((int) shell->ctx->cmd_buff[
					shell->ctx->cmd_buff_pos])) {
		if (shell->ctx->internal.flags.insert_mode) {
			shell->ctx->internal.flags.insert_mode = 0;
			shell_op_char_insert(shell, ' ');
			shell->ctx->internal.flags.insert_mode = 1;
		} else {
			shell_op_char_insert(shell, ' ');
		}
	} else {
		/*  case:
		 * | | -> cursor
		 * cons_name $: valid_cmd valid_sub_cmd| |argument  <tab>
		 */
		shell_op_cursor_move(shell, 1);
		/* result:
		 * cons_name $: valid_cmd valid_sub_cmd |a|rgument
		 */
	}
}

static size_t shell_str_common(const char *s1, const char *s2, size_t n)
{
	size_t common = 0;

	while ((n > 0) && (*s1 == *s2) && (*s1 != '\0')) {
		s1++;
		s2++;
		n--;
		common++;
	}

	return common;
}

static void tab_options_print(const struct shell *shell,
			      const struct shell_static_entry *cmd,
			      const char *str, size_t first, size_t cnt,
			      u16_t longest)
{
	size_t str_len = shell_strlen(str);
	const struct shell_static_entry *match;
	size_t idx = first;

	/* Printing all matching commands (options). */
	tab_item_print(shell, SHELL_INIT_OPTION_PRINTER, longest);

	while (cnt) {
		/* shell->ctx->active_cmd can be safely used outside of command
		 * context to save stack
		 */
		cmd_get(cmd ? cmd->subcmd : NULL, cmd ? 1 : 0,
			idx, &match, &shell->ctx->active_cmd);
		idx++;

		if (str && match->syntax &&
		    !is_completion_candidate(match->syntax, str, str_len)) {
			continue;
		}

		tab_item_print(shell, match->syntax, longest);
		cnt--;
	}

	shell_fprintf(shell, SHELL_INFO, "\r\n%s", shell->prompt);
	shell_fprintf(shell, SHELL_NORMAL, "%s", shell->ctx->cmd_buff);

	shell_op_cursor_position_synchronize(shell);
}

static u16_t common_beginning_find(const struct shell_static_entry *cmd,
				   const char **str,
				   size_t first, size_t cnt)
{
	struct shell_static_entry dynamic_entry;
	const struct shell_static_entry *match;
	u16_t common = UINT16_MAX;


	cmd_get(cmd ? cmd->subcmd : NULL, cmd ? 1 : 0,
		first, &match, &dynamic_entry);

	*str = match->syntax;

	for (size_t idx = first + 1; idx < first + cnt; idx++) {
		struct shell_static_entry dynamic_entry2;
		const struct shell_static_entry *match2;
		int curr_common;

		cmd_get(cmd ? cmd->subcmd : NULL, cmd ? 1 : 0,
			idx, &match2, &dynamic_entry2);

		curr_common = shell_str_common(match->syntax, match2->syntax,
					       UINT16_MAX);
		common = (curr_common < common) ? curr_common : common;
	}

	return common;
}

static void partial_autocomplete(const struct shell *shell,
				 const struct shell_static_entry *cmd,
				 const char *arg,
				 size_t first, size_t cnt)
{
	const char *completion;
	u16_t common = common_beginning_find(cmd, &completion, first, cnt);
	int arg_len = shell_strlen(arg);

	if (common) {
		shell_op_completion_insert(shell, &completion[arg_len],
					   common - arg_len);
	}
}

static void shell_tab_handle(const struct shell *shell)
{
	/* +1 reserved for NULL in function shell_make_argv */
	char *argv[CONFIG_SHELL_ARGC_MAX + 1];
	/* d_entry - placeholder for dynamic command */
	struct shell_static_entry d_entry;
	const struct shell_static_entry *cmd;
	size_t arg_idx;
	u16_t longest;
	size_t first;
	size_t argc;
	size_t cnt;


	bool tab_possible = shell_tab_prepare(shell, &cmd, argv, &argc,
					      &arg_idx, &d_entry);

	if (tab_possible == false) {
		return;
	}

	find_completion_candidates(cmd, argv[arg_idx], &first, &cnt, &longest);

	if (!cnt) {
		/* No candidates to propose. */
		return;
	} else if (cnt == 1) {
		/* Autocompletion.*/
		autocomplete(shell, cmd, argv[arg_idx], first);
	} else {
		tab_options_print(shell, cmd, argv[arg_idx], first, cnt,
				  longest);
		partial_autocomplete(shell, cmd, argv[arg_idx], first, cnt);
	}
}

#define SHELL_ASCII_MAX_CHAR (127u)
static inline int ascii_filter(const char data)
{
	return (u8_t) data > SHELL_ASCII_MAX_CHAR ?
			-EINVAL : 0;
}

static void metakeys_handle(const struct shell *shell, char data)
{
	/* Optional feature */
	if (!IS_ENABLED(CONFIG_SHELL_METAKEYS)) {
		return;
	}

	switch (data) {
	case SHELL_VT100_ASCII_CTRL_A: /* CTRL + A */
		shell_op_cursor_home_move(shell);
		break;

	case SHELL_VT100_ASCII_CTRL_C: /* CTRL + C */
		shell_op_cursor_end_move(shell);
		if (!shell_cursor_in_empty_line(shell)) {
			cursor_next_line_move(shell);
		}
		shell_state_set(shell, SHELL_STATE_ACTIVE);
		break;

	case SHELL_VT100_ASCII_CTRL_E: /* CTRL + E */
		shell_op_cursor_end_move(shell);
		break;

	case SHELL_VT100_ASCII_CTRL_L: /* CTRL + L */
		SHELL_VT100_CMD(shell, SHELL_VT100_CURSORHOME);
		SHELL_VT100_CMD(shell, SHELL_VT100_CLEARSCREEN);
		shell_fprintf(shell, SHELL_INFO, "%s", shell->prompt);
		if (flag_echo_is_set(shell)) {
			shell_fprintf(shell, SHELL_NORMAL, "%s",
				      shell->ctx->cmd_buff);
			shell_op_cursor_position_synchronize(shell);
		}
		break;

	case SHELL_VT100_ASCII_CTRL_U: /* CTRL + U */
		shell_op_cursor_home_move(shell);
		shell_cmd_buffer_clear(shell);
		clear_eos(shell);
		break;

	case SHELL_VT100_ASCII_CTRL_W: /* CTRL + W */
		shell_op_word_remove(shell);
		break;

	default:
		break;
	}
}

static void shell_state_collect(const struct shell *shell)
{
	size_t count = 0;
	char data;

	while (true) {
		(void)shell->iface->api->read(shell->iface, &data,
					      sizeof(data), &count);
		if (count == 0) {
			return;
		}

		if (ascii_filter(data) != 0) {
			continue;
		}

		/* todo pwr_mgmt_feed();*/

		switch (shell->ctx->receive_state) {
		case SHELL_RECEIVE_DEFAULT:
			if ((data == '\r') || (data == '\n')) {
				if (!shell->ctx->cmd_buff_len) {
					history_mode_exit(shell);
					cursor_next_line_move(shell);
				} else {
					/* Command execution */
					(void)shell_execute(shell);
				}

				shell_state_set(shell, SHELL_STATE_ACTIVE);
				return;
			}
			switch (data) {
			case SHELL_VT100_ASCII_ESC: /* ESCAPE */
				receive_state_change(shell, SHELL_RECEIVE_ESC);
				break;

			case '\0':
				break;

			case '\t': /* TAB */
				if (flag_echo_is_set(shell)) {
					shell_tab_handle(shell);
				}
				break;

			case SHELL_VT100_ASCII_BSPACE: /* BACKSPACE */
				if (flag_echo_is_set(shell)) {
					shell_op_char_backspace(shell);
				}
				break;

			case SHELL_VT100_ASCII_DEL: /* DELETE */
				if (flag_echo_is_set(shell)) {
					if (flag_delete_mode_set(shell)) {
						shell_op_char_backspace(shell);

					} else {
						shell_op_char_delete(shell);
					}
				}
				break;

			default:
				if (isprint((int) data)) {
					shell_op_char_insert(shell, data);
				} else {
					metakeys_handle(shell, data);
				}
				break;
			}
			break;

		case SHELL_RECEIVE_ESC:
			if (data == '[') {
				receive_state_change(shell,
						SHELL_RECEIVE_ESC_SEQ);
			} else {
				receive_state_change(shell,
						SHELL_RECEIVE_DEFAULT);
			}
			break;

		case SHELL_RECEIVE_ESC_SEQ:
			receive_state_change(shell, SHELL_RECEIVE_DEFAULT);

			if (!flag_echo_is_set(shell)) {
				return;
			}

			switch (data) {
			case 'A': /* UP arrow */
				history_handle(shell, true);
				break;

			case 'B': /* DOWN arrow */
				history_handle(shell, false);
				break;

			case 'C': /* RIGHT arrow */
				shell_op_right_arrow(shell);
				break;

			case 'D': /* LEFT arrow */
				shell_op_left_arrow(shell);
				break;

			case '4': /* END Button in ESC[n~ mode */
				receive_state_change(shell,
						SHELL_RECEIVE_TILDE_EXP);
				/* fall through */
				/* no break */
			case 'F': /* END Button in VT100 mode */
				shell_op_cursor_end_move(shell);
				break;

			case '1': /* HOME Button in ESC[n~ mode */
				receive_state_change(shell,
						SHELL_RECEIVE_TILDE_EXP);
				/* fall through */
				/* no break */
			case 'H': /* HOME Button in VT100 mode */
				shell_op_cursor_home_move(shell);
				break;

			case '2': /* INSERT Button in ESC[n~ mode */
				receive_state_change(shell,
						SHELL_RECEIVE_TILDE_EXP);
				/* fall through */
				/* no break */
			case 'L': /* INSERT Button in VT100 mode */
				shell->ctx->internal.flags.insert_mode ^= 1;
				break;

			case '3':/* DELETE Button in ESC[n~ mode */
				receive_state_change(shell,
						SHELL_RECEIVE_TILDE_EXP);
				if (flag_echo_is_set(shell)) {
					shell_op_char_delete(shell);
				}
				break;

			default:
				break;
			}
			break;

		case SHELL_RECEIVE_TILDE_EXP:
			receive_state_change(shell, SHELL_RECEIVE_DEFAULT);
			break;

		default:
			receive_state_change(shell, SHELL_RECEIVE_DEFAULT);
			break;
		}
	}
}

static void cmd_trim(const struct shell *shell)
{
	shell_buffer_trim(shell->ctx->cmd_buff, &shell->ctx->cmd_buff_len);
	shell->ctx->cmd_buff_pos = shell->ctx->cmd_buff_len;
}

/* Function returning pointer to root command matching requested syntax. */
static const struct shell_cmd_entry *root_cmd_find(const char *syntax)
{
	const size_t cmd_count = shell_root_cmd_count();
	const struct shell_cmd_entry *cmd;

	for (size_t cmd_idx = 0; cmd_idx < cmd_count; ++cmd_idx) {
		cmd = shell_root_cmd_get(cmd_idx);
		if (strcmp(syntax, cmd->u.entry->syntax) == 0) {
			return cmd;
		}
	}

	return NULL;
}

/* Function is analyzing the command buffer to find matching commands. Next, it
 * invokes the  last recognized command which has a handler and passes the rest
 * of command buffer as arguments.
 */
static int shell_execute(const struct shell *shell)
{
	struct shell_static_entry d_entry; /* Memory for dynamic commands. */
	char *argv[CONFIG_SHELL_ARGC_MAX + 1]; /* +1 reserved for NULL */
	const struct shell_static_entry *p_static_entry = NULL;
	const struct shell_cmd_entry *p_cmd = NULL;
	size_t cmd_lvl = SHELL_CMD_ROOT_LVL;
	size_t cmd_with_handler_lvl = 0;
	bool wildcard_found = false;
	int ret_val = 0;
	size_t cmd_idx;
	size_t argc;
	char quote;

	shell_op_cursor_end_move(shell);
	if (!shell_cursor_in_empty_line(shell)) {
		cursor_next_line_move(shell);
	}

	memset(&shell->ctx->active_cmd, 0, sizeof(shell->ctx->active_cmd));

	cmd_trim(shell);

	history_put(shell, shell->ctx->cmd_buff,
		    shell->ctx->cmd_buff_len);

	if (IS_ENABLED(CONFIG_SHELL_WILDCARD)) {
		shell_wildcard_prepare(shell);
	}

	/* create argument list */
	quote = shell_make_argv(&argc, &argv[0], shell->ctx->cmd_buff,
				CONFIG_SHELL_ARGC_MAX);

	if (!argc) {
		return -ENOEXEC;
	}

	if (quote != 0) {
		shell_fprintf(shell, SHELL_ERROR, "not terminated: %c\r\n",
			      quote);
		return -ENOEXEC;
	}

	/*  Searching for a matching root command. */
	p_cmd = root_cmd_find(argv[0]);
	if (p_cmd == NULL) {
		shell_fprintf(shell, SHELL_ERROR, "%s%s\r\n", argv[0],
					SHELL_MSG_COMMAND_NOT_FOUND);
		return -ENOEXEC;
	}

	/* Root command shall be always static. */
	__ASSERT_NO_MSG(p_cmd->is_dynamic == false);

	/* checking if root command has a handler */
	shell->ctx->active_cmd = *p_cmd->u.entry;

	p_cmd = p_cmd->u.entry->subcmd;
	cmd_lvl++;
	cmd_idx = 0;

	/* Below loop is analyzing subcommands of found root command. */
	while (true) {
		if (cmd_lvl >= argc) {
			break;
		}

		if (!strcmp(argv[cmd_lvl], "-h") ||
		    !strcmp(argv[cmd_lvl], "--help")) {
			/* Command called with help option so it makes no sense
			 * to search deeper commands.
			 */
			help_flag_set(shell);
			break;
		}

		if (IS_ENABLED(CONFIG_SHELL_WILDCARD)) {
			enum shell_wildcard_status status;

			status = shell_wildcard_process(shell, p_cmd,
							 argv[cmd_lvl]);
			/* Wildcard character found but there is no matching
			 * command.
			 */
			if (status == SHELL_WILDCARD_CMD_NO_MATCH_FOUND) {
				break;
			}

			/* Wildcard character was not found function can process
			 * argument.
			 */
			if (status != SHELL_WILDCARD_NOT_FOUND) {
				++cmd_lvl;
				wildcard_found = true;
				continue;
			}
		}

		cmd_get(p_cmd, cmd_lvl, cmd_idx++, &p_static_entry, &d_entry);

		if ((cmd_idx == 0) || (p_static_entry == NULL)) {
			break;
		}

		if (strcmp(argv[cmd_lvl], p_static_entry->syntax) == 0) {
			/* checking if command has a handler */
			if (p_static_entry->handler != NULL) {
				if (IS_ENABLED(CONFIG_SHELL_WILDCARD)) {
					if (wildcard_found) {
						shell_op_cursor_end_move(shell);
						shell_op_cond_next_line(shell);

						/* An error occurred, fnmatch
						 * argument cannot be followed
						 * by argument with a handler to
						 * avoid multiple function
						 * calls.
						 */
						shell_fprintf(shell,
							      SHELL_ERROR,
							"Error: requested"
							" multiple function"
							" executions\r\n");
						help_flag_clear(shell);

						return -ENOEXEC;
					}
				}

				shell->ctx->active_cmd = *p_static_entry;
				cmd_with_handler_lvl = cmd_lvl;
			}

			cmd_lvl++;
			cmd_idx = 0;
			p_cmd = p_static_entry->subcmd;
		}
	}

	if (IS_ENABLED(CONFIG_SHELL_WILDCARD)) {
		shell_wildcard_finalize(shell);
		/* cmd_buffer has been overwritten by function finalize function
		 * with all expanded commands. Hence shell_make_argv needs to
		 * be called again.
		 */
		(void)shell_make_argv(&argc, &argv[0],
				      shell->ctx->cmd_buff,
				      CONFIG_SHELL_ARGC_MAX);
	}

	/* Executing the deepest found handler. */
	if (shell->ctx->active_cmd.handler == NULL) {
		if (shell->ctx->active_cmd.help) {
			shell_help_print(shell, NULL, 0);
		} else {
			shell_fprintf(shell, SHELL_ERROR,
				      SHELL_MSG_SPECIFY_SUBCOMMAND);
			ret_val = -ENOEXEC;
		}
	} else {
		ret_val = shell->ctx->active_cmd.handler(shell,
						   argc - cmd_with_handler_lvl,
						   &argv[cmd_with_handler_lvl]);
	}

	help_flag_clear(shell);

	return ret_val;
}

static void shell_transport_evt_handler(enum shell_transport_evt evt_type,
				      void *context)
{
	struct shell *shell = (struct shell *)context;
	struct k_poll_signal *signal;

	signal = (evt_type == SHELL_TRANSPORT_EVT_RX_RDY) ?
			&shell->ctx->signals[SHELL_SIGNAL_RXRDY] :
			&shell->ctx->signals[SHELL_SIGNAL_TXDONE];
	k_poll_signal(signal, 0);
}

static void shell_current_command_erase(const struct shell *shell)
{
	shell_multiline_data_calc(&shell->ctx->vt100_ctx.cons,
				  shell->ctx->cmd_buff_pos,
				  shell->ctx->cmd_buff_len);
	shell_op_cursor_horiz_move(shell, -shell->ctx->vt100_ctx.cons.cur_x);
	shell_op_cursor_vert_move(shell, shell->ctx->vt100_ctx.cons.cur_y - 1);

	clear_eos(shell);
}

static void shell_current_command_print(const struct shell *shell)
{
	shell_fprintf(shell, SHELL_INFO, "%s", shell->prompt);

	if (flag_echo_is_set(shell)) {
		shell_fprintf(shell, SHELL_NORMAL, "%s", shell->ctx->cmd_buff);
		shell_op_cursor_position_synchronize(shell);
	}
}

static void shell_log_process(const struct shell *shell)
{
	bool processed;
	int signaled;
	int result;

	do {
		shell_current_command_erase(shell);
		processed = shell_log_backend_process(shell->log_backend);
		shell_current_command_print(shell);

		k_poll_signal_check(&shell->ctx->signals[SHELL_SIGNAL_RXRDY],
						    &signaled, &result);

	} while (processed && !signaled);
}

static int shell_instance_init(const struct shell *shell, const void *p_config,
			       bool use_colors)
{
	__ASSERT_NO_MSG(shell);
	__ASSERT_NO_MSG(shell->ctx && shell->iface && shell->prompt);

	int err;

	err = shell->iface->api->init(shell->iface, p_config,
				      shell_transport_evt_handler,
				      (void *) shell);
	if (err != 0) {
		return err;
	}

	history_init(shell);

	memset(shell->ctx, 0, sizeof(*shell->ctx));

	if (IS_ENABLED(CONFIG_SHELL_BACKSPACE_MODE_DELETE)) {
		shell->ctx->internal.flags.mode_delete = 1;
	}

	if (IS_ENABLED(CONFIG_SHELL_STATS)) {
		shell->stats->log_lost_cnt = 0;
	}

	shell->ctx->internal.flags.tx_rdy = 1;
	shell->ctx->internal.flags.echo = CONFIG_SHELL_ECHO_STATUS;
	shell->ctx->state = SHELL_STATE_INITIALIZED;
	shell->ctx->vt100_ctx.cons.terminal_wid = SHELL_DEFAULT_TERMINAL_WIDTH;
	shell->ctx->vt100_ctx.cons.terminal_hei = SHELL_DEFAULT_TERMINAL_HEIGHT;
	shell->ctx->vt100_ctx.cons.name_len = shell_strlen(shell->prompt);
	shell->ctx->internal.flags.use_colors =
					IS_ENABLED(CONFIG_SHELL_VT100_COLORS);

	return 0;
}

static int shell_instance_uninit(const struct shell *shell);

void shell_thread(void *shell_handle, void *dummy1, void *dummy2)
{
	struct shell *shell = (struct shell *)shell_handle;
	int err;
	int i;

	for (i = 0; i < SHELL_SIGNALS; i++) {
		k_poll_signal_init(&shell->ctx->signals[i]);
		k_poll_event_init(&shell->ctx->events[i],
				  K_POLL_TYPE_SIGNAL,
				  K_POLL_MODE_NOTIFY_ONLY,
				  &shell->ctx->signals[i]);
	}

	err = shell_start(shell);
	if (err != 0) {
		return;
	}

	while (true) {
		int signaled;
		int result;

		err = k_poll(shell->ctx->events, SHELL_SIGNALS, K_FOREVER);
		(void)err;

		k_poll_signal_check(&shell->ctx->signals[SHELL_SIGNAL_KILL],
				    &signaled, &result);

		if (signaled) {
			k_poll_signal_reset(
				&shell->ctx->signals[SHELL_SIGNAL_KILL]);
			(void)shell_instance_uninit(shell);

			k_thread_abort(k_current_get());
		}

		k_poll_signal_check(&shell->ctx->signals[SHELL_SIGNAL_LOG_MSG],
						    &signaled, &result);

		if (!signaled) {
			/* Other signals handled together.*/
			k_poll_signal_reset(
				&shell->ctx->signals[SHELL_SIGNAL_RXRDY]);
			k_poll_signal_reset(
				&shell->ctx->signals[SHELL_SIGNAL_TXDONE]);
			shell_process(shell);
		} else if (IS_ENABLED(CONFIG_LOG)) {
			k_poll_signal_reset(
				&shell->ctx->signals[SHELL_SIGNAL_LOG_MSG]);
			/* process log msg */
			shell_log_process(shell);
		}
	}
}

int shell_init(const struct shell *shell, const void *transport_config,
	       bool use_colors, bool log_backend, u32_t init_log_level)
{
	__ASSERT_NO_MSG(shell);
	int err;

	err = shell_instance_init(shell, transport_config, use_colors);
	if (err != 0) {
		return err;
	}

	if (log_backend) {
		if (IS_ENABLED(CONFIG_LOG)) {
			shell_log_backend_enable(shell->log_backend,
						 (void *)shell, init_log_level);
		}
	}

	k_tid_t tid = k_thread_create(shell->thread,
			      shell->stack, CONFIG_SHELL_STACK_SIZE,
			      shell_thread, (void *)shell, NULL, NULL,
			      CONFIG_SHELL_THREAD_PRIO, 0, K_NO_WAIT);

	k_thread_name_set(tid, "shell");
	return 0;
}

static int shell_instance_uninit(const struct shell *shell)
{
	__ASSERT_NO_MSG(shell);
	__ASSERT_NO_MSG(shell->ctx && shell->iface && shell->prompt);
	int err;

	if (flag_processing_is_set(shell)) {
		return -EBUSY;
	}

	if (IS_ENABLED(CONFIG_LOG)) {
		/* todo purge log queue */
		shell_log_backend_disable(shell->log_backend);
	}

	err = shell->iface->api->uninit(shell->iface);
	if (err != 0) {
		return err;
	}

	history_purge(shell);

	shell->ctx->state = SHELL_STATE_UNINITIALIZED;

	return 0;
}

int shell_uninit(const struct shell *shell)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		/* signal kill message */
		(void)k_poll_signal(&shell->ctx->signals[SHELL_SIGNAL_KILL], 0);

		return 0;
	} else {
		return shell_instance_uninit(shell);
	}
}

int shell_start(const struct shell *shell)
{
	__ASSERT_NO_MSG(shell);
	__ASSERT_NO_MSG(shell->ctx && shell->iface && shell->prompt);
	int err;

	if (shell->ctx->state != SHELL_STATE_INITIALIZED) {
		return -ENOTSUP;
	}

	err = shell->iface->api->enable(shell->iface, false);
	if (err != 0) {
		return err;
	}

	if (IS_ENABLED(CONFIG_SHELL_VT100_COLORS_ENABLED)) {
		vt100_color_set(shell, SHELL_NORMAL);
	}

	shell_raw_fprintf(shell->fprintf_ctx, "\r\n\n");

	shell_state_set(shell, SHELL_STATE_ACTIVE);

	return 0;
}

int shell_stop(const struct shell *shell)
{
	__ASSERT_NO_MSG(shell);

	if ((shell->ctx->state == SHELL_STATE_INITIALIZED) ||
	    (shell->ctx->state == SHELL_STATE_UNINITIALIZED)) {
		return -ENOTSUP;
	}

	shell_state_set(shell, SHELL_STATE_INITIALIZED);

	return 0;
}

void shell_process(const struct shell *shell)
{
	__ASSERT_NO_MSG(shell);

	union shell_internal internal;

	internal.value = 0;
	internal.flags.processing = 1;

	(void)atomic_or((atomic_t *)&shell->ctx->internal.value,
			internal.value);

	switch (shell->ctx->state) {
	case SHELL_STATE_UNINITIALIZED:
	case SHELL_STATE_INITIALIZED:
		/* Console initialized but not started. */
		break;

	case SHELL_STATE_ACTIVE:
		shell_state_collect(shell);
		break;

	default:
		break;
	}

	transport_buffer_flush(shell);

	internal.value = 0xFFFFFFFF;
	internal.flags.processing = 0;
	(void)atomic_and((atomic_t *)&shell->ctx->internal.value,
			 internal.value);
}

/* Function shall be only used by the fprintf module. */
void shell_print_stream(const void *user_ctx, const char *data,
			size_t data_len)
{
	shell_write((const struct shell *) user_ctx, data, data_len);
}

void shell_fprintf(const struct shell *shell, enum shell_vt100_color color,
		   const char *p_fmt, ...)
{
	__ASSERT_NO_MSG(shell);

	va_list args = { 0 };

	va_start(args, p_fmt);

	if (IS_ENABLED(CONFIG_SHELL_VT100_COLORS) &&
	    shell->ctx->internal.flags.use_colors &&
	    (color != shell->ctx->vt100_ctx.col.col)) {
		struct shell_vt100_colors col;

		vt100_colors_store(shell, &col);
		vt100_color_set(shell, color);

		shell_fprintf_fmt(shell->fprintf_ctx, p_fmt, args);

		vt100_colors_restore(shell, &col);
	} else {
		shell_fprintf_fmt(shell->fprintf_ctx, p_fmt, args);
	}

	va_end(args);
}

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

static void help_cmd_print(const struct shell *shell)
{
	static const char cmd_sep[] = " - ";	/* commands separator */

	u16_t field_width = shell_strlen(shell->ctx->active_cmd.syntax) +
							  shell_strlen(cmd_sep);

	shell_fprintf(shell, SHELL_NORMAL, "%s%s",
		      shell->ctx->active_cmd.syntax, cmd_sep);

	formatted_text_print(shell, shell->ctx->active_cmd.help,
			     field_width, false);
}

static void help_item_print(const struct shell *shell, const char *item_name,
			    u16_t item_name_width, const char *item_help)
{
	static const u8_t tabulator[] = "  ";
	const u16_t offset = 2 * strlen(tabulator) + item_name_width + 1;

	if (item_name == NULL) {
		return;
	}

	if (!IS_ENABLED(CONFIG_NEWLIB_LIBC) && !IS_ENABLED(CONFIG_ARCH_POSIX)) {
		/* print option name */
		shell_fprintf(shell, SHELL_NORMAL, "%s%-*s%s:",
			      tabulator,
			      item_name_width, item_name,
			      tabulator);
	} else {
		u16_t tmp = item_name_width - strlen(item_name);
		char space = ' ';

		shell_fprintf(shell, SHELL_NORMAL, "%s%s", tabulator,
			      item_name);
		for (u16_t i = 0; i < tmp; i++) {
			shell_write(shell, &space, 1);
		}
		shell_fprintf(shell, SHELL_NORMAL, "%s:", tabulator);
	}

	if (item_help == NULL) {
		cursor_next_line_move(shell);
		return;
	}
	/* print option help */
	formatted_text_print(shell, item_help, offset, false);
}

static void help_options_print(const struct shell *shell,
			       const struct shell_getopt_option *opt,
			       size_t opt_cnt)
{
	static const char opt_sep[] = ", "; /* options separator */
	static const char help_opt[] = "-h, --help";
	u16_t longest_name = shell_strlen(help_opt);

	shell_fprintf(shell, SHELL_NORMAL, "Options:\r\n");

	if ((opt == NULL) || (opt_cnt == 0)) {
		help_item_print(shell, help_opt, longest_name,
				"Show command help.");
		return;
	}

	/* Looking for the longest option string. */
	for (size_t i = 0; i < opt_cnt; ++i) {
		u16_t len = shell_strlen(opt[i].optname_short) +
						 shell_strlen(opt[i].optname) +
						 shell_strlen(opt_sep);

		longest_name = len > longest_name ? len : longest_name;
	}

	/* help option is printed first */
	help_item_print(shell, help_opt, longest_name, "Show command help.");

	/* prepare a buffer to compose a string:
	 *	option name short - option separator - option name
	 */
	memset(shell->ctx->temp_buff, 0, longest_name + 1);

	/* Formating and printing all available options (except -h, --help). */
	for (size_t i = 0; i < opt_cnt; ++i) {
		if (opt[i].optname_short) {
			strcpy(shell->ctx->temp_buff, opt[i].optname_short);
		}
		if (opt[i].optname) {
			if (*shell->ctx->temp_buff) {
				strcat(shell->ctx->temp_buff, opt_sep);
				strcat(shell->ctx->temp_buff, opt[i].optname);
			} else {
				strcpy(shell->ctx->temp_buff, opt[i].optname);
			}
		}
		help_item_print(shell, shell->ctx->temp_buff, longest_name,
				opt[i].optname_help);
	}
}

/* Function is printing command help, its subcommands name and subcommands
 * help string.
 */
static void help_subcmd_print(const struct shell *shell)
{
	const struct shell_static_entry *entry = NULL;
	struct shell_static_entry static_entry;
	u16_t longest_syntax = 0;
	size_t cmd_idx = 0;

	/* Checking if there are any subcommands available. */
	if (!shell->ctx->active_cmd.subcmd) {
		return;
	}

	/* Searching for the longest subcommand to print. */
	do {
		cmd_get(shell->ctx->active_cmd.subcmd, !SHELL_CMD_ROOT_LVL,
			cmd_idx++, &entry, &static_entry);

		if (!entry) {
			break;
		}

		u16_t len = shell_strlen(entry->syntax);

		longest_syntax = longest_syntax > len ? longest_syntax : len;
	} while (cmd_idx != 0); /* too many commands */

	if (cmd_idx == 1) {
		return;
	}

	shell_fprintf(shell, SHELL_NORMAL, "Subcommands:\r\n");

	/* Printing subcommands and help string (if exists). */
	cmd_idx = 0;

	while (true) {
		cmd_get(shell->ctx->active_cmd.subcmd, !SHELL_CMD_ROOT_LVL,
			cmd_idx++, &entry, &static_entry);

		if (entry == NULL) {
			break;
		}

		help_item_print(shell, entry->syntax, longest_syntax,
				entry->help);
	}
}

void shell_help_print(const struct shell *shell,
		      const struct shell_getopt_option *opt, size_t opt_len)
{
	__ASSERT_NO_MSG(shell);

	if (!IS_ENABLED(CONFIG_SHELL_HELP)) {
		return;
	}

	help_cmd_print(shell);
	help_options_print(shell, opt, opt_len);
	help_subcmd_print(shell);
}

int shell_prompt_change(const struct shell *shell, char *prompt)
{

	size_t len = shell_strlen(prompt);

	__ASSERT_NO_MSG(shell);
	__ASSERT_NO_MSG(prompt);

	if (len <= CONFIG_SHELL_PROMPT_LENGTH) {
		memcpy(shell->prompt, prompt, len + 1); /* +1 for '\0' */
		return 0;
	}
	return -1;
}

bool shell_cmd_precheck(const struct shell *shell,
			bool arg_cnt_ok,
			const struct shell_getopt_option *opt,
			size_t opt_len)
{
	if (shell_help_requested(shell)) {
		shell_help_print(shell, opt, opt_len);
		return false;
	}

	if (!arg_cnt_ok) {
		shell_fprintf(shell, SHELL_ERROR,
			      "%s: wrong parameter count\r\n",
			      shell->ctx->active_cmd.syntax);

		if (IS_ENABLED(SHELL_HELP_ON_WRONG_ARGUMENT_COUNT)) {
			shell_help_print(shell, opt, opt_len);
		}

		return false;
	}

	return true;
}

int shell_execute_cmd(const struct shell *shell, const char *cmd)
{
	u16_t cmd_len = shell_strlen(cmd);

	if ((cmd == NULL) || (shell == NULL)) {
		return -ENOEXEC;
	}

	if (cmd_len > (CONFIG_SHELL_CMD_BUFF_SIZE - 1)) {
		return -ENOEXEC;
	}

	strcpy(shell->ctx->cmd_buff, cmd);
	shell->ctx->cmd_buff_len = cmd_len;

	return shell_execute(shell);
}
