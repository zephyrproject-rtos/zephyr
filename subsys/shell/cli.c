/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <shell/cli.h>
#include "shell_utils.h"
#include "shell_ops.h"
#include "cli_vt100.h"
#include <assert.h>
#include <atomic.h>
#include <logging/log_backend.h>
#include <logging/log_ctrl.h>
#include <logging/log_output.h>
#include <lib/fnmatch/fnmatch.h>
#include <logging/log.h>

#define LOG_MODULE_NAME shell
LOG_MODULE_REGISTER();

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

#define CLI_DATA_SECTION_ITEM_GET(i)	\
	NRF_SECTION_ITEM_GET(cli_command, shell_cmd_entry_t, (i))
#define CLI_DATA_SECTION_ITEM_COUNT	\
	NRF_SECTION_ITEM_COUNT(cli_command, shell_cmd_entry_t)

#define CLI_SORTED_CMD_PTRS_ITEM_GET(i)	\
	NRF_SECTION_ITEM_GET(cli_sorted_cmd_ptrs, const char *, (i))
#define CLI_SORTED_CMD_PTRS_START_ADDR_GET	\
	NRF_SECTION_START_ADDR(cli_sorted_cmd_ptrs)

#define SHELL_INIT_OPTION_PRINTER	(NULL)

#define SHELL_INITIAL_CURS_POS		(1u)  /* Initial cursor position is: (1, 1). */

#define SHELL_CMD_ROOT_LVL		(0u)

typedef enum {
	WILDCARD_CMD_ADDED,
	WILDCARD_CMD_ADDED_MISSING_SPACE,
	WILDCARD_CMD_NO_MATCH_FOUND
} wildcard_cmd_status_t;

/*static bool shell_log_entry_process(const struct shell *shell, bool skip);*/
static void shell_execute(const struct shell *shell);

extern const struct shell_cmd_entry __shell_root_cmds_start[0];
extern const struct shell_cmd_entry __shell_root_cmds_end[0];

static inline const struct shell_cmd_entry *shell_root_cmd_get(u32_t id)
{
	return &__shell_root_cmds_start[id];
}

static inline u32_t shell_root_cmd_count(void)
{
	return ((void *)__shell_root_cmds_end -
			(void *)__shell_root_cmds_start)/
				sizeof(struct shell_cmd_entry);
}

static inline void transport_buffer_flush(const struct shell *shell) {
	shell_fprintf_buffer_flush(shell->fprintf_ctx);
}

static inline void flag_help_set(const struct shell *shell)
{
	shell->ctx->internal.flags.show_help = 1;
}
static inline void flag_help_clear(const struct shell *shell)
{
	shell->ctx->internal.flags.show_help = 0;
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

static void cli_cmd_buffer_clear(const struct shell *shell)
{
	shell->ctx->cmd_buff[0] = '\0'; /* clear command buffer */
	shell->ctx->cmd_buff_pos = 0;
	shell->ctx->cmd_buff_len = 0;
}

/* Function sends data stream to the CLI instance. Each time before the cli_write function is called,
 * it must be ensured that IO buffer of fprintf is flushed to avoid synchronization issues.
 * For that purpose, use function transport_buffer_flush(shell) */
static void shell_write(const struct shell *shell, const void *data,
			size_t length)
{
	assert(shell && data);

	size_t offset = 0;
	size_t tmp_cnt;

	while (length) {
		int err = shell->iface->api->write(shell->iface,
				&((uint8_t const *) data)[offset], length,
				&tmp_cnt);
		(void)err;
		assert(err == 0);
		assert(length >= tmp_cnt);
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
		    struct shell_static_entry *st_entry) {
	assert(entry != NULL);
	assert(st_entry != NULL);

	if (lvl == SHELL_CMD_ROOT_LVL) {
		if (idx < shell_root_cmd_count()) {
			const struct shell_cmd_entry *cmd;

			cmd = shell_root_cmd_get(idx);
			*entry = cmd->u.entry;
			return;
		} else {
			*entry = NULL;
			return;
		}
	}

	if (command == NULL) {
		*entry = NULL;
		return;
	}

	if (command->is_dynamic) {
		command->u.dynamic_get(idx, st_entry);
		*entry = (st_entry->syntax != NULL) ? st_entry : NULL;
	} else {
		*entry = (command->u.entry[idx].syntax != NULL) ?
				&command->u.entry[idx] : NULL;
	}
}

static void vt100_color_set(const struct shell *shell,
			    enum shell_vt100_color color)
{

	if (shell->ctx->vt100_ctx.col.col == color)
	{
		return;
	}

	shell->ctx->vt100_ctx.col.col = color;

	if (color != SHELL_DEFAULT) {

		u8_t cmd[] = SHELL_VT100_COLOR(color - 1);

		shell_raw_fprintf(shell->fprintf_ctx, "%s", cmd);
	}
	else {
		static u8_t const cmd[] = SHELL_VT100_MODESOFF;

		shell_raw_fprintf(shell->fprintf_ctx, "%s", cmd);
	}
}

static void vt100_bgcolor_set(const struct shell *shell,
			      enum shell_vt100_color bgcolor)
{
	if ((bgcolor == SHELL_DEFAULT) ||
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
				 struct shell_vt100_colors const *color)
{
	vt100_color_set(shell, color->col);
	vt100_bgcolor_set(shell, color->bgcol);
}

static void shell_state_set(const struct shell *shell, enum shell_state state)
{
	shell->ctx->state = state;

	if (state == SHELL_STATE_ACTIVE) {
		cli_cmd_buffer_clear(shell);
		shell_fprintf(shell, SHELL_INFO, "%s", shell->name);
	}
}

static void option_print(const struct shell *shell, char const * option,
			 u16_t longest_option) {
	static char const * tab = "  ";
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
	size_t len;
	bool history_mode;

	/*optional feature */
	if (!IS_ENABLED(CONFIG_SHELL_HISTORY)) {
		return;
	}

	/* Backup command if history is entered */
	if(!shell_history_active(shell->history)) {
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
					      char *cmd_str)
{
	const struct shell_static_entry *entry = NULL;
	struct shell_static_entry d_entry;
	size_t idx = 0;

	do {
		cmd_get(cmd, lvl, idx++, &entry, &d_entry);
		if (entry && (strcmp(cmd_str, entry->syntax) == 0)) {
			LOG_INF("match %s %s", cmd_str, entry->syntax);
			return entry;
		}
	} while (entry);

	return entry;
}

/** @brief Function for getting last valid command in list of arguments. */
static const struct shell_static_entry *get_last_command(
						const struct shell *shell,
						size_t argc,
						char * argv[],
						size_t *match_arg,
						bool with_handler)
{
	const struct shell_cmd_entry *prev_cmd = NULL;
	const struct shell_static_entry *entry = NULL;
	const struct shell_static_entry *prev_entry = NULL;
	*match_arg = SHELL_CMD_ROOT_LVL;

	while (*match_arg < argc) {
		entry = find_cmd(prev_cmd, *match_arg, argv[*match_arg]);
		if (entry) {
			prev_cmd = entry->subcmd;
			prev_entry = entry;
			(*match_arg)++;
		} else {
			entry = prev_entry;
			break;
		}
	}

	return entry;
}

/* Prepare arguments and return number of space available for completion. */
static u16_t shell_tab_prepare(const struct shell *shell,
			      char **argv, size_t * argc,
			      const struct shell_static_entry **complete_cmd,
			      size_t *complete_arg_idx)
{
	size_t search_argc;
	u16_t compl_len = (CONFIG_SHELL_CMD_BUFF_SIZE - 1)
				- shell->ctx->cmd_buff_len;

	if (!compl_len) {
		return compl_len;
	}


	/* If the Tab key is pressed, "history mode" must be terminated because tab and history handlers
	 are sharing the same array: temp_buff. */
	history_mode_exit(shell);

	/* Copy command from its beginning to cursor position. */
	memcpy(shell->ctx->temp_buff, shell->ctx->cmd_buff,
			shell->ctx->cmd_buff_pos);
	shell->ctx->temp_buff[shell->ctx->cmd_buff_pos] = '\0';

	/* Create argument list. */
	(void)shell_make_argv(argc, argv, shell->ctx->temp_buff,
			      CONFIG_SHELL_ARGC_MAX);

	/* If last command is not completed (followed by space) it is treated
	 * as incompleted one.
	 */
	search_argc = isspace((int)shell->ctx->cmd_buff[
					shell->ctx->cmd_buff_pos - 1]) ?
					*argc : *argc - 1;

	*complete_cmd = get_last_command(shell, search_argc, argv,
					 complete_arg_idx, false);

	return compl_len;
}

static bool is_completion_candidate(const char *candidate,
				    const char *str, size_t len)
{
	return (strncmp(candidate, str, len) == 0) ? true : false;
}

static void find_completion_candidates(const struct shell_static_entry *cmd,
				       const char *incompl_cmd,
				       size_t *first_idx, size_t *cnt,
				       u16_t *longest)
{
	struct shell_static_entry dynamic_entry;
	const struct shell_static_entry *candidate;
	size_t idx = 0;
	bool found = false;
	size_t incompl_cmd_len = shell_strlen(incompl_cmd);

	*longest = 0;
	*cnt = 0;

	while (true) {
		cmd_get(cmd ? cmd->subcmd : NULL, cmd ? 1 : 0,
			idx, &candidate, &dynamic_entry);

		if (!candidate) {
			break;
		}

		if (is_completion_candidate(candidate->syntax, incompl_cmd, incompl_cmd_len)) {
			size_t slen = strlen(candidate->syntax);

			*longest = (slen > *longest) ? slen : *longest;
			(*cnt)++;

			if (!found) {
				*first_idx = idx;
			}

			found = true;
		} else {
			if (found) {
				break;
			}
		}
		idx++;
	}
}

static void autocomplete(const struct shell *shell,
			 const struct shell_static_entry *cmd,
			 const char *arg,
			 size_t subcmd_idx)
{
	struct shell_static_entry dynamic_entry;
	const struct shell_static_entry *match;
	size_t arg_len = shell_strlen(arg);
	size_t cmd_len;

	cmd_get(cmd ? cmd->subcmd : NULL, cmd ? 1 : 0,
			subcmd_idx, &match, &dynamic_entry);
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
		 | | -> cursor
		 cons_name $: valid_cmd valid_sub_cmd| |argument  <tab>
		 */
		shell_op_cursor_move(shell, 1);
		/* result:
		 cons_name $: valid_cmd valid_sub_cmd |a|rgument
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

static void options_print(const struct shell *shell,
			  const struct shell_static_entry *cmd,
			  size_t first, size_t cnt, u16_t longest)
{
	struct shell_static_entry dynamic_entry;
	const struct shell_static_entry *match;
	size_t idx = first;

	/* Printing all matching commands (options). */
	option_print(shell, SHELL_INIT_OPTION_PRINTER, longest);

	while (cnt) {
		cmd_get(cmd ? cmd->subcmd : NULL, cmd ? 1 : 0,
			idx, &match, &dynamic_entry);
		option_print(shell, match->syntax, longest);
		cnt--;
		idx++;
	}

	shell_fprintf(shell, SHELL_INFO, "\r\n%s", shell->name);
	shell_fprintf(shell, SHELL_NORMAL, "%s", shell->ctx->cmd_buff);

	shell_op_cursor_position_synchronize(shell);
}

static u16_t common_begining_find(const struct shell_static_entry *cmd,
				  const char **str,
				  size_t first, size_t cnt)
{
	struct shell_static_entry dynamic_entry;
	const struct shell_static_entry *match;
	u16_t common = UINT16_MAX;


	cmd_get(cmd ? cmd->subcmd : NULL, cmd ? 1 : 0,
			first, &match, &dynamic_entry);
	*str = match->syntax;

	for (size_t idx = first+1; idx < cnt; idx++) {
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
	u16_t common = common_begining_find(cmd, &completion, first, cnt);
	int arg_len = shell_strlen(arg);

	if (common) {
		shell_op_completion_insert(shell, &completion[arg_len],
					   common - arg_len);
	}
}

static void shell_tab_handle(const struct shell *shell)
{
	size_t arg_idx;
	u16_t compl_len;
	size_t first;
	size_t cnt;
	u16_t longest;
	const struct shell_static_entry *cmd;
	size_t argc;
	/* +1 reserved for NULL in function shell_make_argv */
	char * argv[CONFIG_SHELL_ARGC_MAX + 1];

	compl_len = shell_tab_prepare(shell, argv, &argc, &cmd, &arg_idx);

	if (!compl_len) {
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
		options_print(shell, cmd, first, cnt, longest);
		partial_autocomplete(shell, cmd, argv[arg_idx], first, cnt);
	}
}

#define SHELL_ASCII_MAX_CHAR (127u)
static inline int ascii_filter(char const data) {
	return (uint8_t) data > SHELL_ASCII_MAX_CHAR ?
			-EINVAL : 0;
}

static void metakeys_handle(const struct shell *shell, char data)
{
	/* Optional feature */
	if (!IS_ENABLED(CONFIG_SHELL_METAKEYS)) {
		return;
	}

	switch(data) {
	case SHELL_VT100_ASCII_CTRL_A: /* CTRL + A */
		shell_op_cursor_home_move(shell);
		break;

	case SHELL_VT100_ASCII_CTRL_C: /* CTRL + C */
		shell_op_cursor_end_move(shell);
		shell_op_cond_next_line(shell);
		shell_state_set(shell, SHELL_STATE_ACTIVE);
		break;

	case SHELL_VT100_ASCII_CTRL_E: /* CTRL + E */
		shell_op_cursor_end_move(shell);
		break;

	case SHELL_VT100_ASCII_CTRL_L: /* CTRL + L */
		SHELL_VT100_CMD(shell, SHELL_VT100_CURSORHOME);
		SHELL_VT100_CMD(shell, SHELL_VT100_CLEARSCREEN);
		shell_fprintf(shell, SHELL_INFO, "%s", shell->name);
		if (flag_echo_is_set(shell))
		{
			shell_fprintf(shell, SHELL_NORMAL, "%s",
				      shell->ctx->cmd_buff);
			shell_op_cursor_position_synchronize(shell);
		}
		break;

	case SHELL_VT100_ASCII_CTRL_U: /* CTRL + U */
		shell_op_cursor_home_move(shell);
		cli_cmd_buffer_clear(shell);
		clear_eos(shell);
		break;

	case SHELL_VT100_ASCII_CTRL_W: /* CTRL + W */
		shell_op_word_remove(shell);
		break;
	default:
		break;
	}
}

static void cli_state_collect(const struct shell *shell) {
	size_t count = 0;
	char data;

	while (1) {
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
			if (data == shell->newline_char) {
				if (!shell->ctx->cmd_buff_len) {
					history_mode_exit(shell);
					cursor_next_line_move(shell);
				} else {
					/* Command execution */
					shell_execute(shell);
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
					shell_op_char_delete(shell);
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
			case 'F': /* END Button in VT100 mode */
				shell_op_cursor_end_move(shell);
				break;
			case '1': /* HOME Button in ESC[n~ mode */
				receive_state_change(shell,
						SHELL_RECEIVE_TILDE_EXP);
				/* fall through */
			case 'H': /* HOME Button in VT100 mode */
				shell_op_cursor_home_move(shell);
				break;
			case '2': /* INSERT Button in ESC[n~ mode */
				receive_state_change(shell,
						SHELL_RECEIVE_TILDE_EXP);
				/* fall through */
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

/**
 * @internal @brief Function for searching and adding commands matching to wildcard pattern.
 *
 * This function is internal to shell module and shall be not called directly.
 *
 * @param[in/out] shell		 Pointer to the CLI instance.
 * @param[in]	  cmd		 Pointer to command which will be processed
 * @param[in]	  cmd_lvl	 Command level in the command tree.
 * @param[in]	  pattern	 Pointer to wildcard pattern.
 * @param[out]	  counter	 Number of found and added commands.
 *
 * @retval WILDCARD_CMD_ADDED	All matching commands added to the buffer.
 * @retval WILDCARD_CMD_ADDED_MISSING_SPACE  Not all matching commands added
 *					     because CONFIG_SHELL_CMD_BUFF_SIZE
 *					     is too small.
 * @retval WILDCARD_CMD_NO_MATCH_FOUND No matching command found.
 */
static wildcard_cmd_status_t commands_expand(const struct shell *shell,
					     const struct shell_cmd_entry *cmd,
					     size_t cmd_lvl, char *pattern,
					     size_t *counter)
{
	size_t cmd_idx = 0;
	size_t cnt = 0;
	bool success = false;
	struct shell_static_entry static_entry;
	struct shell_static_entry const * p_static_entry = NULL;
	wildcard_cmd_status_t ret_val = WILDCARD_CMD_NO_MATCH_FOUND;

	do {
		cmd_get(cmd, cmd_lvl, cmd_idx++, &p_static_entry, &static_entry);

		if (!p_static_entry) {
			break;
		}

		if (0 == fnmatch(pattern, p_static_entry->syntax, 0)) {
			int err;

			err = shell_command_add(shell->ctx->temp_buff,
						&shell->ctx->cmd_tmp_buff_len,
						p_static_entry->syntax,
						pattern);
			if (err) {
				shell_fprintf(shell, SHELL_WARNING,
					      "Command buffer is not expanded "
					      "with matching wildcard pattern"
					       " (err %d).\r\n", err);
				break;
			}

			cnt++;
		}

	} while(cmd_idx);

	if (cnt > 0)
	{
		*counter = cnt;
		shell_pattern_remove(shell->ctx->temp_buff,
				     &shell->ctx->cmd_tmp_buff_len, pattern);
		ret_val = success ?
			WILDCARD_CMD_ADDED :WILDCARD_CMD_ADDED_MISSING_SPACE;
	}

	return ret_val;
}

/* Function is analyzing the command buffer to find matching commands. Next, it
 * invokes the  last recognized command which has a handler and passes the rest
 * of command buffer as arguments. */
static void shell_execute(const struct shell *shell)
{
	char quote;
	size_t argc;
	/* +1 reserved for NULL added by function shell_make_argv */
	char * argv[CONFIG_SHELL_ARGC_MAX + 1];

	size_t cmd_idx; /* currently analyzed command in cmd_level */
	/* currently analyzed command level */
	size_t cmd_lvl = SHELL_CMD_ROOT_LVL;
	/* last command level for which a handler has been found */
	size_t cmd_handler_lvl = 0;
	/* last command index for which a handler has been found */
	size_t cmd_handler_idx = 0;
	size_t commands_expanded = 0;
	struct shell_cmd_entry const * p_cmd = NULL;

	cmd_trim(shell);

	history_put(shell, shell->ctx->cmd_buff,
		    shell->ctx->cmd_buff_len);

	/* Wildcard can be correctly handled under following conditions:
	 - wildcard command does not have a handler
	 - wildcard command is on the deepest commands level
	 - other commands on the same level as wildcard command shall also not
	   have a handler

	 Algorithm:
	 1. Command buffer is copied to Temp buffer.
	 2. Algorithm goes through Command buffer to find handlers and
	    subcommands.
	 3. If algorithm will find a wildcard character it switches to Temp
	    buffer.
	 4. In the Temp buffer command with found wildcard character is changed
	    into matching command(s).
	 5. Algorithm switch back to Command buffer and analyzes next command.
	 6. When all arguments are analyzed from Command buffer, Temp buffer is
	    copied to Command buffer.
	 7. Last found handler is executed with all arguments in the Command
	    buffer.
	 */

	memset(shell->ctx->temp_buff, 0, sizeof(shell->ctx->temp_buff));
	memcpy(shell->ctx->temp_buff,
			shell->ctx->cmd_buff,
			shell->ctx->cmd_buff_len);

	/* Function shell_spaces_trim must be used instead of shell_make_argv. At this
	 * point it is important to keep temp_buff as a one string. It will
	 * allow to find wildcard commands easily with strstr function.
	 */
	shell_spaces_trim(shell->ctx->temp_buff);

	/* +1 for EOS*/
	shell->ctx->cmd_tmp_buff_len = shell_strlen(shell->ctx->temp_buff) + 1;

	shell_op_cursor_end_move(shell);
	cursor_next_line_move(shell);

	/* create argument list */
	quote = shell_make_argv(&argc, &argv[0], shell->ctx->cmd_buff,
				CONFIG_SHELL_ARGC_MAX);

	if (!argc) {
		cursor_next_line_move(shell);
		return;
	}

	if (quote != 0) {
		shell_fprintf(shell, SHELL_ERROR, "not terminated: %c\r\n",
			      quote);
		return;
	}

	/*  Searching for a matching root command. */
	for (cmd_idx = 0; cmd_idx <= shell_root_cmd_count(); ++cmd_idx) {
		if (cmd_idx >= shell_root_cmd_count()) {
			shell_fprintf(shell,
			SHELL_ERROR, "%s%s\r\n", argv[0],
			SHELL_MSG_COMMAND_NOT_FOUND);
			return;
		}

		p_cmd = shell_root_cmd_get(cmd_idx);
		if (strcmp(argv[cmd_lvl], p_cmd->u.entry->syntax)) {
			continue;
		}
		break;
	}

	/* Root command shall be always static. */
	assert(p_cmd->is_dynamic == false);

	/* Pointer to the deepest command level with a handler. */
	struct shell_cmd_entry const * p_cmd_low_level_entry = NULL;

	/* Memory reserved for dynamic commands. */
	struct shell_static_entry static_entry;
	struct shell_static_entry const * p_static_entry = NULL;

	shell_cmd_handler handler_cmd_lvl_0 = p_cmd->u.entry->handler;
	if (handler_cmd_lvl_0 != NULL) {
		shell->ctx->current_stcmd = p_cmd->u.entry;
	}

	p_cmd = p_cmd->u.entry->subcmd;
	cmd_lvl++;
	cmd_idx = 0;

	while (1) {
		if (cmd_lvl >= argc) {
			break;
		}

		if (!strcmp(argv[cmd_lvl], "-h") ||
		    !strcmp(argv[cmd_lvl], "--help")) {
			/* Command called with help option so it makes no sense
			 * to search deeper commands.
			 */
			flag_help_set(shell);
			break;
		}

		if (IS_ENABLED(CONFIG_SHELL_WILDCARD)) {
			/* Wildcard character is found */
			if (wildcard_character_exist(argv[cmd_lvl]))
			{
				size_t counter = 0;
				wildcard_cmd_status_t status;

				/* Function will search commands tree for
				 * commands matching wildcard pattern stored in
				 * argv[cmd_lvl]. If match is found wildcard
				 * pattern will be replaced by matching commands
				 * in temp_buffer. If there is no space to add
				 * all matching commands function will add as
				 * many as possible. Next it will continue to
				 * search for next wildcard pattern and it will
				 * try to add matching commands. */
				status = commands_expand(shell, p_cmd, cmd_lvl,
							 argv[cmd_lvl],
							 &counter);
				if (status == WILDCARD_CMD_NO_MATCH_FOUND) {
					break;
				}

				commands_expanded += counter;
				cmd_lvl++;
				continue;
			}
		}

		cmd_get(p_cmd, cmd_lvl, cmd_idx++, &p_static_entry,
			&static_entry);

		if ((cmd_idx == 0) || (p_static_entry == NULL)) {
			break;
		}

		if (strcmp(argv[cmd_lvl], p_static_entry->syntax) == 0) {
			/* checking if command has a handler */
			if (p_static_entry->handler != NULL) {
				if (IS_ENABLED(CONFIG_SHELL_WILDCARD)) {
					if (commands_expanded > 0) {
						shell_op_cursor_end_move(shell);
						shell_op_cond_next_line(shell);

						/* An error occured, fnmatch argument cannot be followed by argument
						 * with a handler to avoid multiple function calls. */
						shell_fprintf(shell,SHELL_ERROR,
							      "Error: requested multiple function executions\r\n");
						flag_help_clear(shell);

						return;
					}
				}

				/* Storing p_st_cmd->handler is not feasible for dynamic commands. Data will be
				 * invalid with the next loop iteration. */
				cmd_handler_lvl = cmd_lvl;
				cmd_handler_idx = cmd_idx - 1;
				p_cmd_low_level_entry = p_cmd;
			}

			cmd_lvl++;
			cmd_idx = 0;
			p_cmd = p_static_entry->subcmd;
		}
	}

	if (IS_ENABLED(CONFIG_SHELL_WILDCARD)) {
		if (commands_expanded > 0)
		{
			/* Copy temp_buff to cmd_buff */
			memcpy(shell->ctx->cmd_buff,
					shell->ctx->temp_buff,
					shell->ctx->cmd_tmp_buff_len);
			shell->ctx->cmd_buff_len = shell->ctx->cmd_tmp_buff_len;

			/* calling make_arg function again because cmd_buffer
			 * has additional commands.
			 */
			(void)shell_make_argv(&argc, &argv[0],
					      shell->ctx->cmd_buff,
					      CONFIG_SHELL_ARGC_MAX);
		}
	}

	/* Executing the deepest found handler. */
	if (p_cmd_low_level_entry) {
		cmd_get(p_cmd_low_level_entry, cmd_handler_lvl, cmd_handler_idx,
			&p_static_entry, &static_entry);

		shell->ctx->current_stcmd = p_static_entry;

		shell->ctx->current_stcmd->handler(shell,
				argc - cmd_handler_lvl, &argv[cmd_handler_lvl]);
	} else if (handler_cmd_lvl_0) {
		handler_cmd_lvl_0(shell, argc, &argv[0]);
	} else {
		shell_fprintf(shell, SHELL_ERROR, SHELL_MSG_SPECIFY_SUBCOMMAND);
	}

	flag_help_clear(shell);
}

static void shell_transport_evt_handler(enum shell_transport_evt evt_type,
				      void * context)
{
	struct shell *shell = (struct shell *)context;
	struct k_poll_signal *signal;

	signal = (evt_type == SHELL_TRANSPORT_EVT_RX_RDY) ?
			&shell->ctx->signals[SHELL_SIGNAL_RXRDY] :
			&shell->ctx->signals[SHELL_SIGNAL_TXDONE];
	k_poll_signal(signal, 0);
}

static int shell_instance_init(const struct shell *shell, void const * p_config,
			       bool use_colors) {
	assert(shell);
	assert(shell->ctx && shell->iface && shell->name);
	assert((shell->newline_char == '\n') || (shell->newline_char == '\r'));

	int err;

	err = shell->iface->api->init(shell->iface, p_config,
				      shell_transport_evt_handler,
				      (void *) shell);
	if (err != 0) {
		return err;
	}

	history_init(shell);

	memset(shell->ctx, 0, sizeof(*shell->ctx));

	shell->ctx->internal.flags.tx_rdy = 1;
	shell->ctx->internal.flags.echo = CONFIG_SHELL_ECHO_STATUS;
	shell->ctx->state = SHELL_STATE_INITIALIZED;
	shell->ctx->vt100_ctx.cons.terminal_wid = SHELL_DEFAULT_TERMINAL_WIDTH;
	shell->ctx->vt100_ctx.cons.terminal_hei = SHELL_DEFAULT_TERMINAL_HEIGHT;
	shell->ctx->vt100_ctx.cons.name_len = shell_strlen(shell->name);
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

	while (1)
	{
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
		} else {
			/* Other signals handled together.*/
			k_poll_signal_reset(
				&shell->ctx->signals[SHELL_SIGNAL_RXRDY]);
			k_poll_signal_reset(
				&shell->ctx->signals[SHELL_SIGNAL_TXDONE]);
			shell_process(shell);
		}
	}
}

int shell_init(const struct shell * shell, void const * transport_config,
	       bool use_colors, bool log_backend, u32_t init_log_level)
{
	assert(shell);
	int err;

	err = shell_instance_init(shell, transport_config, use_colors);
	if (err != 0) {
		return err;
	}

	if (log_backend)
	{
		if (IS_ENABLED(CONFIG_LOG)) {
			log_backend_enable(shell->log_backend,
					   (void *)shell, init_log_level);
		}
	}

	(void)k_thread_create(shell->thread, shell->stack,
			      CONFIG_SHELL_STACK_SIZE, shell_thread,
			      (void *)shell, NULL, NULL,
			      CONFIG_SHELL_THREAD_PRIO, 0, K_NO_WAIT);

	return 0;
}

static int shell_instance_uninit(const struct shell *shell)
{
	assert(shell);
	assert(shell->ctx && shell->iface && shell->name);
	int err;

	if (flag_processing_is_set(shell)) {
		return -EBUSY;
	}

	if (IS_ENABLED(CONFIG_LOG)) {
		/* todo purge log queue */
		log_backend_disable(shell->log_backend);
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
	assert(shell);
	assert(shell->ctx && shell->iface && shell->name);
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
		vt100_bgcolor_set(shell, SHELL_VT100_COLOR_BLACK);
	}

	shell_raw_fprintf(shell->fprintf_ctx, "\r\n\n");

	shell_state_set(shell, SHELL_STATE_ACTIVE);

	return 0;
}

int shell_stop(const struct shell *shell)
{
	assert(shell);

	if ((shell->ctx->state == SHELL_STATE_INITIALIZED) ||
	    (shell->ctx->state == SHELL_STATE_UNINITIALIZED)) {
		return -ENOTSUP;
	}

	shell_state_set(shell, SHELL_STATE_INITIALIZED);

	return 0;
}

void shell_process(const struct shell *shell) {
	assert(shell);

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
	case SHELL_STATE_ACTIVE: {
		bool log_processed;

		cli_state_collect(shell);
		log_processed = false; /* todo cli_log_entry_process(shell, false);*/

		if (log_processed) {
			shell_fprintf(shell, SHELL_INFO, "%s", shell->name);
			if (flag_echo_is_set(shell)) {
				shell_fprintf(shell, SHELL_NORMAL, "%s",
					      shell->ctx->cmd_buff);
				shell_op_cursor_position_synchronize(shell);
			}
		}
		break;
	}
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
void shell_print_stream(void const * user_ctx, char const * data,
			size_t data_len)
{
	shell_write((const struct shell *) user_ctx, data, data_len);
}

void shell_fprintf(const struct shell * shell, enum shell_vt100_color color,
		   char const * p_fmt, ...)
{
	assert(shell);

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
 *   shell			   Pointer to CLI instance.
 *   p_str			   Pointer to string to be printed.
 *   terminal_offset	 Requested left margin.
 *   offset_first_line   Add margin to the first printed line.
 */
static void format_offset_string_print(const struct shell *shell,
				       char const * str,
				       size_t terminal_offset,
				       bool offset_first_line)
{
	size_t length;
	size_t offset = 0;

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

	while (1) {
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
		} else {
			/* String is longer than terminal line so text needs to
			 * divide in the way to not divide words.
			 */
			length = shell->ctx->vt100_ctx.cons.terminal_wid
					- terminal_offset;

			while (1) {
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
			 * before calling cli_write.
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
	}
	cursor_next_line_move(shell);
}

void shell_help_print(const struct shell * shell,
		      struct shell_getopt_option const *opt, size_t opt_len) {
	assert(shell);

	if (!IS_ENABLED(CONFIG_SHELL_HELP)) {
		return;
	}

	static u8_t const tab_len = 2;
	static char const opt_sep[] = ", "; /* options separator */
	static char const help[] = "-h, --help";
	static char const cmd_sep[] = " - "; /* command separator */
	u16_t field_width = 0;
	u16_t longest_string = shell_strlen(help) - shell_strlen(opt_sep);

	/* Printing help string for command. */
	shell_fprintf(shell, SHELL_NORMAL, "%s%s",
		      shell->ctx->current_stcmd->syntax, cmd_sep);

	field_width = shell_strlen(shell->ctx->current_stcmd->syntax)
			+ shell_strlen(cmd_sep);
	format_offset_string_print(shell, shell->ctx->current_stcmd->help,
			field_width, false);

	shell_fprintf(shell, SHELL_NORMAL, "Options:\r\n");

	/* Looking for the longest option string. */
	if ((opt_len > 0) && (opt != NULL)) {
		for (size_t i = 0; i < opt_len; ++i) {
			if (shell_strlen(opt[i].optname_short)
					+ shell_strlen(opt[i].optname)
					> longest_string) {
				longest_string =
					shell_strlen(opt[i].optname_short) +
						shell_strlen(opt[i].optname);
			}
		}
	}

	longest_string += shell_strlen(opt_sep) + tab_len;

	shell_fprintf(shell,
	SHELL_NORMAL, "  %-*s:", longest_string, help);

	/* Print help string for options (only -h and --help).
	 * tab_len + 1 == "  " and ':' from: "  %-*s:"
	 */
	field_width = longest_string + tab_len + 1;
	format_offset_string_print(shell, "Show command help.", field_width,
				   false);

	/* Formating and printing all available options (except -h, --help). */
	if (opt) {
		for (size_t i = 0; i < opt_len; ++i) {
			if (opt[i].optname_short && opt[i].optname) {
				shell_fprintf(shell, SHELL_NORMAL,
					      "  %s%s%s", opt[i].optname_short,
					      opt_sep, opt[i].optname);
				field_width = longest_string + tab_len;
				shell_op_cursor_horiz_move(shell, field_width
						- (shell_strlen(opt[i].optname_short)
						+ shell_strlen(opt[i].optname)
										+ tab_len
										+ shell_strlen(
												opt_sep)));
				shell_putc(shell, ':');
				++field_width; /* incrementing because char ':' was already printed above */
			} else if (opt[i].optname_short) {
				shell_fprintf(shell, SHELL_NORMAL, "  %-*s:",
					      longest_string,
					      opt[i].optname_short);
				/* tab_len + 1 == "  " and ':' from: "  %-*s:" */
				field_width = longest_string + tab_len + 1;
			} else if (opt[i].optname) {
				shell_fprintf(shell, SHELL_NORMAL, "  %-*s:",
						longest_string, opt[i].optname);
				/* tab_len + 1 == "  " and ':' from: "  %-*s:" */
				field_width = longest_string + tab_len + 1;
			} else {
				/* Do nothing. */
			}

			if (opt[i].optname_help) {
				format_offset_string_print(shell,
						opt[i].optname_help,
						field_width, false);
			} else {
				cursor_next_line_move(shell);
			}
		}
	}

	/* Checking if there are any subcommands avilable. */
	if (!shell->ctx->current_stcmd->subcmd) {
		return;
	}

	/* Printing formatted help of one level deeper subcommands. */
	struct shell_static_entry static_entry;
	struct shell_cmd_entry const *cmd = shell->ctx->current_stcmd->subcmd;
	struct shell_static_entry const *st_cmd = NULL;
	size_t cmd_idx = 0;

	field_width = 0;
	longest_string = 0;

	/* Searching for the longest subcommand to print. */
	while (1) {
		cmd_get(cmd, !SHELL_CMD_ROOT_LVL, cmd_idx++, &st_cmd,
				&static_entry);

		if (!st_cmd) {
			break;
		}

		if (shell_strlen(st_cmd->syntax) > longest_string) {
			longest_string = shell_strlen(st_cmd->syntax);
		}
	}

	/* Checking if there are dynamic subcommands. */
	if (cmd_idx == 1) {
		/* No dynamic subcommands available. */
		return;
	}

	shell_fprintf(shell, SHELL_NORMAL, "Subcommands:\r\n");

	/* Printing subcommands and help string (if exists). */
	cmd_idx = 0;

	while (1) {
		cmd_get(cmd, !SHELL_CMD_ROOT_LVL, cmd_idx++, &st_cmd,
				&static_entry);

		if (st_cmd == NULL) {
			break;
		}

		field_width = longest_string + tab_len;
		shell_fprintf(shell, SHELL_NORMAL, "  %-*s:", field_width,
				st_cmd->syntax);
		/* tab_len + 1 == "  " and ':' from: "  %-*s:" */
		field_width += tab_len + 1;

		if (st_cmd->help) {
			format_offset_string_print(shell, st_cmd->help,
						   field_width, false);
		} else {
			cursor_next_line_move(shell);
		}
	}
}
#if 0
#if SHELL_LOG_BACKEND && NRF_MODULE_ENABLED(NRF_LOG)

#define SHELL_LOG_MSG_OVERFLOW_MSK ((uint32_t)7)
static bool cli_log_entry_process(const struct shell *shell, bool skip) {
	nrf_log_entry_t entry;

#if NRF_MODULE_ENABLED(SHELL_STATISTICS)
	bool print_msg = false;
#endif

	if (nrf_queue_pop(
			((shell_log_backend_t *) shell->p_log_backend->ctx)->p_queue,
			&entry) != NRF_SUCCESS) {
		return false;
	}

	if (skip) {
		nrf_memobj_put(entry);
#if NRF_MODULE_ENABLED(SHELL_STATISTICS)
		++shell->ctx->statistics.log_lost_cnt;
		if ((shell->ctx->statistics.log_lost_cnt & SHELL_LOG_MSG_OVERFLOW_MSK) == 1) {
			/* Set flag to print a message after clearing the currently entered command. */
			print_msg = true;
		} else
#endif
		{
			return true;
		}
	}
	{
		/* Erasing the currently displayed command and console name. */
		struct shell_multiline_cons *cons = &shell->ctx->vt100_ctx.cons;

		shell_multiline_data_calc(cons, shell->ctx->cmd_buff_pos,
					  shell->ctx->cmd_buff_len);

		if (cons->cur_y > SHELL_INITIAL_CURS_POS) {
			cursor_up_move(shell,
					cons->cur_y - SHELL_INITIAL_CURS_POS);
		}

		if (cons->cur_x > SHELL_INITIAL_CURS_POS) {
			cursor_left_move(shell,
					cons->cur_x - SHELL_INITIAL_CURS_POS);
		}
		clear_eos(shell);
	}

#if NRF_MODULE_ENABLED(SHELL_STATISTICS)
	if (print_msg) {
		/* Print the requested string and exit function. */
		shell_fprintf(shell, SHELL_ERROR, "Lost logs - increase log backend queue size.\r\n");

		return true;
	}
#endif

	/* Printing logs from the queue. */
	do {
		nrf_log_header_t header;
		size_t memobj_offset = 0;
		nrf_log_str_formatter_entry_params_t params;

		nrf_memobj_read(entry, &header, HEADER_SIZE * sizeof(uint32_t),
				memobj_offset);
		memobj_offset = HEADER_SIZE * sizeof(uint32_t);

		params.timestamp = header.timestamp;
		params.module_id = header.module_id;
		params.dropped = header.dropped;
		params.use_colors = NRF_LOG_USES_COLORS; /* Color will be provided by the console application. */

		if (header.base.generic.type == HEADER_TYPE_STD) {
			char const * p_log_str =
					(char const *) ((uint32_t) header.base.std.addr);
			params.severity =
					(nrf_log_severity_t) header.base.std.severity;
			uint32_t nargs = header.base.std.nargs;
			uint32_t args[6];

			nrf_memobj_read(entry, args, nargs * sizeof(uint32_t),
					memobj_offset);
			nrf_log_std_entry_process(p_log_str, args, nargs,
					&params, shell->p_fprintf_ctx);
		} else if (header.base.generic.type == HEADER_TYPE_HEXDUMP) {
			uint32_t data_len;
			uint8_t data_buf[8];
			uint32_t chunk_len;

			data_len = header.base.hexdump.len;
			params.severity =
					(nrf_log_severity_t) header.base.hexdump.severity;

			do {
				chunk_len = sizeof(data_buf) > data_len ?
						data_len : sizeof(data_buf);
				nrf_memobj_read(entry, data_buf, chunk_len,
						memobj_offset);
				memobj_offset += chunk_len;
				data_len -= chunk_len;
				nrf_log_hexdump_entry_process(data_buf,
						chunk_len, &params,
						shell->p_fprintf_ctx);
			} while (data_len > 0);
		}

		nrf_memobj_put(entry);
	} while (nrf_queue_pop(
			((shell_log_backend_t *) shell->p_log_backend->ctx)->p_queue,
			&entry) == NRF_SUCCESS);
	return true;
}

static void nrf_log_backend_cli_put(nrf_log_backend_t const * p_backend,
		nrf_log_entry_t * p_msg) {
	shell_log_backend_t * p_backend_cli =
			(shell_log_backend_t *) p_backend->ctx;
	shell_t const *shell = p_backend_cli->shell;

	//If panic mode cannot be handled, stop handling new requests.
	if (shell->ctx->state != SHELL_STATE_PANIC_MODE_INACTIVE) {
		bool panic_mode = (shell->ctx->state
				== SHELL_STATE_PANIC_MODE_ACTIVE);
		//If there is no place for a new log entry, remove the oldest one.
		ret_code_t err_code = nrf_queue_push(p_backend_cli->p_queue,
				&p_msg);
		while (err_code != NRF_SUCCESS) {
			(void) cli_log_entry_process(shell,
					panic_mode ? false : true);

			err_code = nrf_queue_push(p_backend_cli->p_queue,
					&p_msg);
		}
		nrf_memobj_get(p_msg);

		if (panic_mode) {
			(void) cli_log_entry_process(shell, false);
		}
#if NRF_MODULE_ENABLED(SHELL_USES_TASK_MANAGER)
		else {
			task_events_set((task_id_t)((uint32_t)p_backend_cli->p_context & 0x000000FF),
					SHELL_LOG_PENDING_TASK_EVT);
		}
#endif
	}
}

static void nrf_log_backend_cli_flush(nrf_log_backend_t const * p_backend) {
	shell_log_backend_t * p_backend_cli =
			(shell_log_backend_t *) p_backend->ctx;
	shell_t const * shell = p_backend_cli->shell;
	nrf_log_entry_t * p_msg;

	if (nrf_queue_pop(p_backend_cli->p_queue, &p_msg) == NRF_SUCCESS) {
		(void) cli_log_entry_process(shell, false);
	}
	UNUSED_PARAMETER(p_backend);
}

static void nrf_log_backend_cli_panic_set(nrf_log_backend_t const * p_backend) {
	shell_log_backend_t * p_backend_cli =
			(shell_log_backend_t *) p_backend->ctx;
	shell_t const *shell = p_backend_cli->shell;

	if (shell->p_iface->p_api->enable(shell->p_iface, true)
			== NRF_SUCCESS) {
		shell->ctx->state = SHELL_STATE_PANIC_MODE_ACTIVE;
	} else {
		shell->ctx->state = SHELL_STATE_PANIC_MODE_INACTIVE;
	}
}

const nrf_log_backend_api_t nrf_log_backend_cli_api = { .put =
		nrf_log_backend_cli_put, .flush = nrf_log_backend_cli_flush,
		.panic_set = nrf_log_backend_cli_panic_set, };
#else
static bool cli_log_entry_process(const struct shell *shell, bool skip)
{
	UNUSED_PARAMETER(shell);
	UNUSED_PARAMETER(skip);
	return false;
}
#endif // SHELL_LOG_BACKEND
#endif
const struct log_backend_api log_backend_shell_api;





