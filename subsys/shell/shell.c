/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <stdlib.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/shell/shell.h>
#if defined(CONFIG_SHELL_BACKEND_DUMMY)
#include <zephyr/shell/shell_dummy.h>
#endif
#include "shell_ops.h"
#include "shell_help.h"
#include "shell_utils.h"
#include "shell_vt100.h"
#include "shell_wildcard.h"

/* 2 == 1 char for cmd + 1 char for '\0' */
#if (CONFIG_SHELL_CMD_BUFF_SIZE < 2)
	#error too small CONFIG_SHELL_CMD_BUFF_SIZE
#endif

#if (CONFIG_SHELL_PRINTF_BUFF_SIZE < 1)
	#error too small SHELL_PRINTF_BUFF_SIZE
#endif

#define SHELL_MSG_CMD_NOT_FOUND		": command not found"
#define SHELL_MSG_BACKEND_NOT_ACTIVE	\
	"WARNING: A print request was detected on not active shell backend.\n"
#define SHELL_MSG_TOO_MANY_ARGS		"Too many arguments in the command.\n"
#define SHELL_INIT_OPTION_PRINTER	(NULL)

#define SHELL_THREAD_PRIORITY \
	COND_CODE_1(CONFIG_SHELL_THREAD_PRIORITY_OVERRIDE, \
			(CONFIG_SHELL_THREAD_PRIORITY), (K_LOWEST_APPLICATION_THREAD_PRIO))

BUILD_ASSERT(SHELL_THREAD_PRIORITY >=
		  K_HIGHEST_APPLICATION_THREAD_PRIO
		&& SHELL_THREAD_PRIORITY <= K_LOWEST_APPLICATION_THREAD_PRIO,
		  "Invalid range for thread priority");

static inline void receive_state_change(const struct shell *sh,
					enum shell_receive_state state)
{
	sh->ctx->receive_state = state;
}

static void cmd_buffer_clear(const struct shell *sh)
{
	sh->ctx->cmd_buff[0] = '\0'; /* clear command buffer */
	sh->ctx->cmd_buff_pos = 0;
	sh->ctx->cmd_buff_len = 0;
}

static void shell_internal_help_print(const struct shell *sh)
{
	if (!IS_ENABLED(CONFIG_SHELL_HELP)) {
		return;
	}

	z_shell_help_cmd_print(sh, &sh->ctx->active_cmd);
	z_shell_help_subcmd_print(sh, &sh->ctx->active_cmd,
				  "Subcommands:\n");
}

/**
 * @brief Prints error message on wrong argument count.
 *	  Optionally, printing help on wrong argument count.
 *
 * @param[in] shell	  Pointer to the shell instance.
 * @param[in] arg_cnt_ok  Flag indicating valid number of arguments.
 *
 * @return 0		  if check passed
 * @return -EINVAL	  if wrong argument count
 */
static int cmd_precheck(const struct shell *sh,
			bool arg_cnt_ok)
{
	if (!arg_cnt_ok) {
		z_shell_fprintf(sh, SHELL_ERROR,
				"%s: wrong parameter count\n",
				sh->ctx->active_cmd.syntax);

		if (IS_ENABLED(CONFIG_SHELL_HELP_ON_WRONG_ARGUMENT_COUNT)) {
			shell_internal_help_print(sh);
		}

		return -EINVAL;
	}

	return 0;
}

static inline void state_set(const struct shell *sh, enum shell_state state)
{
	sh->ctx->state = state;

	if (state == SHELL_STATE_ACTIVE && !sh->ctx->bypass) {
		cmd_buffer_clear(sh);
		if (z_flag_print_noinit_get(sh)) {
			z_shell_fprintf(sh, SHELL_WARNING, "%s",
					SHELL_MSG_BACKEND_NOT_ACTIVE);
			z_flag_print_noinit_set(sh, false);
		}
		z_shell_print_prompt_and_cmd(sh);
	}
}

static inline enum shell_state state_get(const struct shell *sh)
{
	return sh->ctx->state;
}

static inline const struct shell_static_entry *
selected_cmd_get(const struct shell *sh)
{
	if (IS_ENABLED(CONFIG_SHELL_CMDS_SELECT)
	    || (CONFIG_SHELL_CMD_ROOT[0] != 0)) {
		return sh->ctx->selected_cmd;
	}

	return NULL;
}

static void tab_item_print(const struct shell *sh, const char *option,
			   uint16_t longest_option)
{
	static const char *tab = "  ";
	uint16_t columns;
	uint16_t diff;

	/* Function initialization has been requested. */
	if (option == NULL) {
		sh->ctx->vt100_ctx.printed_cmd = 0;
		return;
	}

	longest_option += z_shell_strlen(tab);

	columns = (sh->ctx->vt100_ctx.cons.terminal_wid
			- z_shell_strlen(tab)) / longest_option;
	diff = longest_option - z_shell_strlen(option);

	if (sh->ctx->vt100_ctx.printed_cmd++ % columns == 0U) {
		z_shell_fprintf(sh, SHELL_OPTION, "\n%s%s", tab, option);
	} else {
		z_shell_fprintf(sh, SHELL_OPTION, "%s", option);
	}

	z_shell_op_cursor_horiz_move(sh, diff);
}

static void history_init(const struct shell *sh)
{
	if (!IS_ENABLED(CONFIG_SHELL_HISTORY)) {
		return;
	}

	z_shell_history_init(sh->history);
}

static void history_purge(const struct shell *sh)
{
	if (!IS_ENABLED(CONFIG_SHELL_HISTORY)) {
		return;
	}

	z_shell_history_purge(sh->history);
}

static void history_mode_exit(const struct shell *sh)
{
	if (!IS_ENABLED(CONFIG_SHELL_HISTORY)) {
		return;
	}

	z_flag_history_exit_set(sh, false);
	z_shell_history_mode_exit(sh->history);
}

static void history_put(const struct shell *sh, uint8_t *line, size_t length)
{
	if (!IS_ENABLED(CONFIG_SHELL_HISTORY)) {
		return;
	}

	z_shell_history_put(sh->history, line, length);
}

static void history_handle(const struct shell *sh, bool up)
{
	bool history_mode;
	uint16_t len;

	/*optional feature */
	if (!IS_ENABLED(CONFIG_SHELL_HISTORY)) {
		return;
	}

	/* Checking if history process has been stopped */
	if (z_flag_history_exit_get(sh)) {
		z_flag_history_exit_set(sh, false);
		z_shell_history_mode_exit(sh->history);
	}

	/* Backup command if history is entered */
	if (!z_shell_history_active(sh->history)) {
		if (up) {
			uint16_t cmd_len = z_shell_strlen(sh->ctx->cmd_buff);

			if (cmd_len) {
				strcpy(sh->ctx->temp_buff,
				       sh->ctx->cmd_buff);
			} else {
				sh->ctx->temp_buff[0] = '\0';
			}
		} else {
			/* Pressing 'down' not in history mode has no effect. */
			return;
		}
	}

	/* Start by checking if history is not empty. */
	history_mode = z_shell_history_get(sh->history, up,
					   sh->ctx->cmd_buff, &len);

	/* On exiting history mode print backed up command. */
	if (!history_mode) {
		strcpy(sh->ctx->cmd_buff, sh->ctx->temp_buff);
		len = z_shell_strlen(sh->ctx->cmd_buff);
	}

	z_shell_op_cursor_home_move(sh);
	z_clear_eos(sh);
	z_shell_print_cmd(sh);
	sh->ctx->cmd_buff_pos = len;
	sh->ctx->cmd_buff_len = len;
	z_shell_op_cond_next_line(sh);
}

static inline uint16_t completion_space_get(const struct shell *sh)
{
	uint16_t space = (CONFIG_SHELL_CMD_BUFF_SIZE - 1) -
			sh->ctx->cmd_buff_len;
	return space;
}

/* Prepare arguments and return number of space available for completion. */
static bool tab_prepare(const struct shell *sh,
			const struct shell_static_entry **cmd,
			const char ***argv, size_t *argc,
			size_t *complete_arg_idx,
			struct shell_static_entry *d_entry)
{
	uint16_t compl_space = completion_space_get(sh);
	size_t search_argc;

	if (compl_space == 0U) {
		return false;
	}

	/* Copy command from its beginning to cursor position. */
	memcpy(sh->ctx->temp_buff, sh->ctx->cmd_buff,
			sh->ctx->cmd_buff_pos);
	sh->ctx->temp_buff[sh->ctx->cmd_buff_pos] = '\0';

	/* Create argument list. */
	(void)z_shell_make_argv(argc, *argv, sh->ctx->temp_buff,
				CONFIG_SHELL_ARGC_MAX);

	if (*argc > CONFIG_SHELL_ARGC_MAX) {
		return false;
	}

	/* terminate arguments with NULL */
	(*argv)[*argc] = NULL;

	if ((IS_ENABLED(CONFIG_SHELL_CMDS_SELECT) || (CONFIG_SHELL_CMD_ROOT[0] != 0))
	    && (*argc > 0) &&
	    (strcmp("select", (*argv)[0]) == 0) &&
	    !z_shell_in_select_mode(sh)) {
		*argv = *argv + 1;
		*argc = *argc - 1;
	}

	/* If last command is not completed (followed by space) it is treated
	 * as uncompleted one.
	 */
	int space = isspace((int)sh->ctx->cmd_buff[
						sh->ctx->cmd_buff_pos - 1]);

	/* root command completion */
	if ((*argc == 0) || ((space == 0) && (*argc == 1))) {
		*complete_arg_idx = Z_SHELL_CMD_ROOT_LVL;
		*cmd = selected_cmd_get(sh);
		return true;
	}

	search_argc = space ? *argc : *argc - 1;

	*cmd = z_shell_get_last_command(selected_cmd_get(sh), search_argc,
					*argv, complete_arg_idx,	d_entry,
					false);

	/* if search_argc == 0 (empty command line) shell_get_last_command will
	 * return NULL tab is allowed, otherwise not.
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

static void find_completion_candidates(const struct shell *sh,
				       const struct shell_static_entry *cmd,
				       const char *incompl_cmd,
				       size_t *first_idx, size_t *cnt,
				       uint16_t *longest)
{
	const struct shell_static_entry *candidate;
	struct shell_static_entry dloc;
	size_t incompl_cmd_len;
	size_t idx = 0;

	incompl_cmd_len = z_shell_strlen(incompl_cmd);
	*longest = 0U;
	*cnt = 0;

	while ((candidate = z_shell_cmd_get(cmd, idx, &dloc)) != NULL) {
		bool is_candidate;
		is_candidate = is_completion_candidate(candidate->syntax,
						incompl_cmd, incompl_cmd_len);
		if (is_candidate) {
			*longest = Z_MAX(strlen(candidate->syntax), *longest);
			if (*cnt == 0) {
				*first_idx = idx;
			}
			(*cnt)++;
		}

		idx++;
	}
}

static void autocomplete(const struct shell *sh,
			 const struct shell_static_entry *cmd,
			 const char *arg,
			 size_t subcmd_idx)
{
	const struct shell_static_entry *match;
	uint16_t cmd_len;
	uint16_t arg_len = z_shell_strlen(arg);

	/* sh->ctx->active_cmd can be safely used outside of command context
	 * to save stack
	 */
	match = z_shell_cmd_get(cmd, subcmd_idx, &sh->ctx->active_cmd);
	__ASSERT_NO_MSG(match != NULL);
	cmd_len = z_shell_strlen(match->syntax);

	if (!IS_ENABLED(CONFIG_SHELL_TAB_AUTOCOMPLETION)) {
		/* Add a space if the Tab button is pressed when command is
		 * complete.
		 */
		if (cmd_len == arg_len) {
			z_shell_op_char_insert(sh, ' ');
		}
		return;
	}

	/* no exact match found */
	if (cmd_len != arg_len) {
		z_shell_op_completion_insert(sh,
					     match->syntax + arg_len,
					     cmd_len - arg_len);
	}

	/* Next character in the buffer is not 'space'. */
	if (isspace((int) sh->ctx->cmd_buff[
					sh->ctx->cmd_buff_pos]) == 0) {
		if (z_flag_insert_mode_get(sh)) {
			z_flag_insert_mode_set(sh, false);
			z_shell_op_char_insert(sh, ' ');
			z_flag_insert_mode_set(sh, true);
		} else {
			z_shell_op_char_insert(sh, ' ');
		}
	} else {
		/*  case:
		 * | | -> cursor
		 * cons_name $: valid_cmd valid_sub_cmd| |argument  <tab>
		 */
		z_shell_op_cursor_move(sh, 1);
		/* result:
		 * cons_name $: valid_cmd valid_sub_cmd |a|rgument
		 */
	}
}

static size_t str_common(const char *s1, const char *s2, size_t n)
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

static void tab_options_print(const struct shell *sh,
			      const struct shell_static_entry *cmd,
			      const char *str, size_t first, size_t cnt,
			      uint16_t longest)
{
	const struct shell_static_entry *match;
	size_t str_len = z_shell_strlen(str);
	size_t idx = first;

	/* Printing all matching commands (options). */
	tab_item_print(sh, SHELL_INIT_OPTION_PRINTER, longest);

	while (cnt) {
		/* sh->ctx->active_cmd can be safely used outside of command
		 * context to save stack
		 */
		match = z_shell_cmd_get(cmd, idx, &sh->ctx->active_cmd);
		__ASSERT_NO_MSG(match != NULL);
		idx++;
		if (str && match->syntax &&
		    !is_completion_candidate(match->syntax, str, str_len)) {
			continue;
		}

		tab_item_print(sh, match->syntax, longest);
		cnt--;
	}

	z_cursor_next_line_move(sh);
	z_shell_print_prompt_and_cmd(sh);
}

static uint16_t common_beginning_find(const struct shell *sh,
				   const struct shell_static_entry *cmd,
				   const char **str,
				   size_t first, size_t cnt, uint16_t arg_len)
{
	struct shell_static_entry dynamic_entry;
	const struct shell_static_entry *match;
	uint16_t common = UINT16_MAX;
	size_t idx = first + 1;

	__ASSERT_NO_MSG(cnt > 1);

	match = z_shell_cmd_get(cmd, first, &dynamic_entry);
	__ASSERT_NO_MSG(match);
	strncpy(sh->ctx->temp_buff, match->syntax,
			sizeof(sh->ctx->temp_buff) - 1);

	*str = match->syntax;

	while (cnt > 1) {
		struct shell_static_entry dynamic_entry2;
		const struct shell_static_entry *match2;
		int curr_common;

		match2 = z_shell_cmd_get(cmd, idx++, &dynamic_entry2);
		if (match2 == NULL) {
			break;
		}

		curr_common = str_common(sh->ctx->temp_buff, match2->syntax,
					 UINT16_MAX);
		if ((arg_len == 0U) || (curr_common >= arg_len)) {
			--cnt;
			common = (curr_common < common) ? curr_common : common;
		}
	}

	return common;
}

static void partial_autocomplete(const struct shell *sh,
				 const struct shell_static_entry *cmd,
				 const char *arg,
				 size_t first, size_t cnt)
{
	const char *completion;
	uint16_t arg_len = z_shell_strlen(arg);
	uint16_t common = common_beginning_find(sh, cmd, &completion, first,
					     cnt, arg_len);

	if (!IS_ENABLED(CONFIG_SHELL_TAB_AUTOCOMPLETION)) {
		return;
	}

	if (common) {
		z_shell_op_completion_insert(sh, &completion[arg_len],
					     common - arg_len);
	}
}

static int exec_cmd(const struct shell *sh, size_t argc, const char **argv,
		    const struct shell_static_entry *help_entry)
{
	int ret_val = 0;

	if (sh->ctx->active_cmd.handler == NULL) {
		if ((help_entry != NULL) && IS_ENABLED(CONFIG_SHELL_HELP)) {
			if (help_entry->help == NULL) {
				return -ENOEXEC;
			}
			if (help_entry->help != sh->ctx->active_cmd.help) {
				sh->ctx->active_cmd = *help_entry;
			}
			shell_internal_help_print(sh);
			return SHELL_CMD_HELP_PRINTED;
		} else {
			z_shell_fprintf(sh, SHELL_ERROR,
					SHELL_MSG_SPECIFY_SUBCOMMAND);
			return -ENOEXEC;
		}
	}

	if (sh->ctx->active_cmd.args.mandatory) {
		uint32_t mand = sh->ctx->active_cmd.args.mandatory;
		uint8_t opt8 = sh->ctx->active_cmd.args.optional;
		uint32_t opt = (opt8 == SHELL_OPT_ARG_CHECK_SKIP) ?
				UINT16_MAX : opt8;
		const bool in_range = IN_RANGE(argc, mand, mand + opt);

		/* Check if argc is within allowed range */
		ret_val = cmd_precheck(sh, in_range);
	}

	if (!ret_val) {
#if CONFIG_SHELL_GETOPT
		getopt_init();
#endif

		z_flag_cmd_ctx_set(sh, true);
		/* Unlock thread mutex in case command would like to borrow
		 * shell context to other thread to avoid mutex deadlock.
		 */
		k_mutex_unlock(&sh->ctx->wr_mtx);
		ret_val = sh->ctx->active_cmd.handler(sh, argc,
							 (char **)argv);
		/* Bring back mutex to shell thread. */
		k_mutex_lock(&sh->ctx->wr_mtx, K_FOREVER);
		z_flag_cmd_ctx_set(sh, false);
	}

	return ret_val;
}

static void active_cmd_prepare(const struct shell_static_entry *entry,
				struct shell_static_entry *active_cmd,
				struct shell_static_entry *help_entry,
				size_t *lvl, size_t *handler_lvl,
				size_t *args_left)
{
	if (entry->handler) {
		*handler_lvl = *lvl;
		*active_cmd = *entry;
		/* If command is final handler and it has a raw optional argument,
		 * then set remaining arguments to mandatory - 1 so after processing mandatory
		 * args, handler is passed remaining raw string
		 */
		if ((entry->subcmd == NULL)
		    && entry->args.optional == SHELL_OPT_ARG_RAW) {
			*args_left = entry->args.mandatory - 1;
		}
	}
	if (entry->help) {
		*help_entry = *entry;
	}
}

static bool wildcard_check_report(const struct shell *sh, bool found,
				  const struct shell_static_entry *entry)
{
	/* An error occurred, fnmatch  argument cannot be followed by argument
	 * with a handler to avoid multiple function calls.
	 */
	if (IS_ENABLED(CONFIG_SHELL_WILDCARD) && found && entry->handler) {
		z_shell_op_cursor_end_move(sh);
		z_shell_op_cond_next_line(sh);

		z_shell_fprintf(sh, SHELL_ERROR,
			"Error: requested multiple function executions\n");
		return false;
	}

	return true;
}

/* Function is analyzing the command buffer to find matching commands. Next, it
 * invokes the  last recognized command which has a handler and passes the rest
 * of command buffer as arguments.
 *
 * By default command buffer is parsed and spaces are treated by arguments
 * separators. Complex arguments are provided in quotation marks with quotation
 * marks escaped within the argument. Argument parser is removing quotation
 * marks at argument boundary as well as escape characters within the argument.
 * However, it is possible to indicate that command shall treat remaining part
 * of command buffer as the last argument without parsing. This can be used for
 * commands which expects whole command buffer to be passed directly to
 * the command handler without any preprocessing.
 * Because of that feature, command buffer is processed argument by argument and
 * decision on further processing is based on currently processed command.
 */
static int execute(const struct shell *sh)
{
	struct shell_static_entry dloc; /* Memory for dynamic commands. */
	const char *argv[CONFIG_SHELL_ARGC_MAX + 1] = {0}; /* +1 reserved for NULL */
	const struct shell_static_entry *parent = selected_cmd_get(sh);
	const struct shell_static_entry *entry = NULL;
	struct shell_static_entry help_entry;
	size_t cmd_lvl = 0;
	size_t cmd_with_handler_lvl = 0;
	bool wildcard_found = false;
	size_t argc = 0, args_left = SIZE_MAX;
	char quote;
	const char **argvp;
	char *cmd_buf = sh->ctx->cmd_buff;
	bool has_last_handler = false;

	z_shell_op_cursor_end_move(sh);
	if (!z_shell_cursor_in_empty_line(sh)) {
		z_cursor_next_line_move(sh);
	}

	memset(&sh->ctx->active_cmd, 0, sizeof(sh->ctx->active_cmd));

	if (IS_ENABLED(CONFIG_SHELL_HISTORY)) {
		z_shell_cmd_trim(sh);
		history_put(sh, sh->ctx->cmd_buff,
			    sh->ctx->cmd_buff_len);
	}

	if (IS_ENABLED(CONFIG_SHELL_WILDCARD)) {
		z_shell_wildcard_prepare(sh);
	}

	/* Parent present means we are in select mode. */
	if (parent != NULL) {
		argv[0] = parent->syntax;
		argv[1] = cmd_buf;
		argvp = &argv[1];
		active_cmd_prepare(parent, &sh->ctx->active_cmd, &help_entry,
				   &cmd_lvl, &cmd_with_handler_lvl, &args_left);
		cmd_lvl++;
	} else {
		help_entry.help = NULL;
		argvp = &argv[0];
	}

	/* Below loop is analyzing subcommands of found root command. */
	while ((argc != 1) && (cmd_lvl < CONFIG_SHELL_ARGC_MAX)
		&& args_left > 0) {
		quote = z_shell_make_argv(&argc, argvp, cmd_buf, 2);
		cmd_buf = (char *)argvp[1];

		if (argc == 0) {
			return -ENOEXEC;
		} else if ((argc == 1) && (quote != 0)) {
			z_shell_fprintf(sh, SHELL_ERROR,
					"not terminated: %c\n", quote);
			return -ENOEXEC;
		}

		if (IS_ENABLED(CONFIG_SHELL_HELP) && (cmd_lvl > 0) &&
		    z_shell_help_request(argvp[0])) {
			/* Command called with help option so it makes no sense
			 * to search deeper commands.
			 */
			if (help_entry.help) {
				sh->ctx->active_cmd = help_entry;
				shell_internal_help_print(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			z_shell_fprintf(sh, SHELL_ERROR,
					SHELL_MSG_SPECIFY_SUBCOMMAND);
			return -ENOEXEC;
		}

		if (IS_ENABLED(CONFIG_SHELL_WILDCARD) && (cmd_lvl > 0)) {
			enum shell_wildcard_status status;

			status = z_shell_wildcard_process(sh, entry,
							  argvp[0]);
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

		if (has_last_handler == false) {
			entry = z_shell_find_cmd(parent, argvp[0], &dloc);
		}

		argvp++;
		args_left--;
		if (entry) {
			if (wildcard_check_report(sh, wildcard_found, entry)
				== false) {
				return -ENOEXEC;
			}

			active_cmd_prepare(entry, &sh->ctx->active_cmd,
					  &help_entry, &cmd_lvl,
					  &cmd_with_handler_lvl, &args_left);
			parent = entry;
		} else {
			if (cmd_lvl == 0 &&
				(!z_shell_in_select_mode(sh) ||
				 sh->ctx->selected_cmd->handler == NULL)) {
				z_shell_fprintf(sh, SHELL_ERROR,
						"%s%s\n", argv[0],
						SHELL_MSG_CMD_NOT_FOUND);
			}

			/* last handler found - no need to search commands in
			 * the next iteration.
			 */
			has_last_handler = true;
		}

		if (args_left || (argc == 2)) {
			cmd_lvl++;
		}

	}

	if ((cmd_lvl >= CONFIG_SHELL_ARGC_MAX) && (argc == 2)) {
		/* argc == 2 indicates that when command string was parsed
		 * there was more characters remaining. It means that number of
		 * arguments exceeds the limit.
		 */
		z_shell_fprintf(sh, SHELL_ERROR, "%s\n",
				SHELL_MSG_TOO_MANY_ARGS);
		return -ENOEXEC;
	}

	if (IS_ENABLED(CONFIG_SHELL_WILDCARD) && wildcard_found) {
		z_shell_wildcard_finalize(sh);
		/* cmd_buffer has been overwritten by function finalize function
		 * with all expanded commands. Hence shell_make_argv needs to
		 * be called again.
		 */
		(void)z_shell_make_argv(&cmd_lvl,
					&argv[selected_cmd_get(sh) ? 1 : 0],
					sh->ctx->cmd_buff,
					CONFIG_SHELL_ARGC_MAX);

		if (selected_cmd_get(sh)) {
			/* Apart from what is in the command buffer, there is
			 * a selected command.
			 */
			cmd_lvl++;
		}
	}

	/* If a command was found */
	if (parent != NULL) {
		/* If the found command uses a raw optional argument and
		 * we have a remaining unprocessed non-null string,
		 * then increment command level so handler receives raw string
		 */
		if (parent->args.optional == SHELL_OPT_ARG_RAW && argv[cmd_lvl] != NULL) {
			cmd_lvl++;
		}
	}

	/* Executing the deepest found handler. */
	return exec_cmd(sh, cmd_lvl - cmd_with_handler_lvl,
			&argv[cmd_with_handler_lvl], &help_entry);
}

static void tab_handle(const struct shell *sh)
{
	const char *__argv[CONFIG_SHELL_ARGC_MAX + 1];
	/* d_entry - placeholder for dynamic command */
	struct shell_static_entry d_entry;
	const struct shell_static_entry *cmd;
	const char **argv = __argv;
	size_t first = 0;
	size_t arg_idx;
	uint16_t longest;
	size_t argc;
	size_t cnt;

	bool tab_possible = tab_prepare(sh, &cmd, &argv, &argc, &arg_idx,
					&d_entry);

	if (tab_possible == false) {
		return;
	}

	find_completion_candidates(sh, cmd, argv[arg_idx], &first, &cnt,
				   &longest);

	if (cnt == 1) {
		/* Autocompletion.*/
		autocomplete(sh, cmd, argv[arg_idx], first);
	} else if (cnt > 1) {
		tab_options_print(sh, cmd, argv[arg_idx], first, cnt,
				  longest);
		partial_autocomplete(sh, cmd, argv[arg_idx], first, cnt);
	}
}

static void alt_metakeys_handle(const struct shell *sh, char data)
{
	/* Optional feature */
	if (!IS_ENABLED(CONFIG_SHELL_METAKEYS)) {
		return;
	}
	if (data == SHELL_VT100_ASCII_ALT_B) {
		z_shell_op_cursor_word_move(sh, -1);
	} else if (data == SHELL_VT100_ASCII_ALT_F) {
		z_shell_op_cursor_word_move(sh, 1);
	} else if (data == SHELL_VT100_ASCII_ALT_R &&
		   IS_ENABLED(CONFIG_SHELL_CMDS_SELECT)) {
		if (selected_cmd_get(sh) != NULL) {
			z_shell_cmd_line_erase(sh);
			z_shell_fprintf(sh, SHELL_WARNING,
					"Restored default root commands\n");
			if (CONFIG_SHELL_CMD_ROOT[0]) {
				sh->ctx->selected_cmd = root_cmd_find(CONFIG_SHELL_CMD_ROOT);
			} else {
				sh->ctx->selected_cmd = NULL;
			}
			z_shell_print_prompt_and_cmd(sh);
		}
	}
}

static void ctrl_metakeys_handle(const struct shell *sh, char data)
{
	/* Optional feature */
	if (!IS_ENABLED(CONFIG_SHELL_METAKEYS)) {
		return;
	}

	switch (data) {
	case SHELL_VT100_ASCII_CTRL_A: /* CTRL + A */
		z_shell_op_cursor_home_move(sh);
		break;

	case SHELL_VT100_ASCII_CTRL_B: /* CTRL + B */
		z_shell_op_left_arrow(sh);
		break;

	case SHELL_VT100_ASCII_CTRL_C: /* CTRL + C */
		z_shell_op_cursor_end_move(sh);
		if (!z_shell_cursor_in_empty_line(sh)) {
			z_cursor_next_line_move(sh);
		}
		z_flag_history_exit_set(sh, true);
		state_set(sh, SHELL_STATE_ACTIVE);
		break;

	case SHELL_VT100_ASCII_CTRL_D: /* CTRL + D */
		z_shell_op_char_delete(sh);
		break;

	case SHELL_VT100_ASCII_CTRL_E: /* CTRL + E */
		z_shell_op_cursor_end_move(sh);
		break;

	case SHELL_VT100_ASCII_CTRL_F: /* CTRL + F */
		z_shell_op_right_arrow(sh);
		break;

	case SHELL_VT100_ASCII_CTRL_K: /* CTRL + K */
		z_shell_op_delete_from_cursor(sh);
		break;

	case SHELL_VT100_ASCII_CTRL_L: /* CTRL + L */
		Z_SHELL_VT100_CMD(sh, SHELL_VT100_CURSORHOME);
		Z_SHELL_VT100_CMD(sh, SHELL_VT100_CLEARSCREEN);
		z_shell_print_prompt_and_cmd(sh);
		break;

	case SHELL_VT100_ASCII_CTRL_N: /* CTRL + N */
		history_handle(sh, false);
		break;

	case SHELL_VT100_ASCII_CTRL_P: /* CTRL + P */
		history_handle(sh, true);
		break;

	case SHELL_VT100_ASCII_CTRL_U: /* CTRL + U */
		z_shell_op_cursor_home_move(sh);
		cmd_buffer_clear(sh);
		z_flag_history_exit_set(sh, true);
		z_clear_eos(sh);
		break;

	case SHELL_VT100_ASCII_CTRL_W: /* CTRL + W */
		z_shell_op_word_remove(sh);
		z_flag_history_exit_set(sh, true);
		break;

	default:
		break;
	}
}

/* Functions returns true if new line character shall be processed */
static bool process_nl(const struct shell *sh, uint8_t data)
{
	if ((data != '\r') && (data != '\n')) {
		z_flag_last_nl_set(sh, 0);
		return false;
	}

	if ((z_flag_last_nl_get(sh) == 0U) ||
	    (data == z_flag_last_nl_get(sh))) {
		z_flag_last_nl_set(sh, data);
		return true;
	}

	return false;
}

#define SHELL_ASCII_MAX_CHAR (127u)
static inline int ascii_filter(const char data)
{
	return (uint8_t) data > SHELL_ASCII_MAX_CHAR ? -EINVAL : 0;
}

static void state_collect(const struct shell *sh)
{
	size_t count = 0;
	char data;

	while (true) {
		shell_bypass_cb_t bypass = sh->ctx->bypass;

		if (bypass) {
			uint8_t buf[16];

			(void)sh->iface->api->read(sh->iface, buf,
							sizeof(buf), &count);
			if (count) {
				z_flag_cmd_ctx_set(sh, true);
				bypass(sh, buf, count);
				z_flag_cmd_ctx_set(sh, false);
				/* Check if bypass mode ended. */
				if (!(volatile shell_bypass_cb_t *)sh->ctx->bypass) {
					state_set(sh, SHELL_STATE_ACTIVE);
				} else {
					continue;
				}
			}

			return;
		}

		(void)sh->iface->api->read(sh->iface, &data,
					      sizeof(data), &count);
		if (count == 0) {
			return;
		}

		if (ascii_filter(data) != 0) {
			continue;
		}

		switch (sh->ctx->receive_state) {
		case SHELL_RECEIVE_DEFAULT:
			if (process_nl(sh, data)) {
				if (!sh->ctx->cmd_buff_len) {
					history_mode_exit(sh);
					z_cursor_next_line_move(sh);
				} else {
					/* Command execution */
					(void)execute(sh);
				}
				/* Function responsible for printing prompt
				 * on received NL.
				 */
				state_set(sh, SHELL_STATE_ACTIVE);
				continue;
			}

			switch (data) {
			case SHELL_VT100_ASCII_ESC: /* ESCAPE */
				receive_state_change(sh, SHELL_RECEIVE_ESC);
				break;

			case '\0':
				break;

			case '\t': /* TAB */
				if (z_flag_echo_get(sh) &&
				    IS_ENABLED(CONFIG_SHELL_TAB)) {
					/* If the Tab key is pressed, "history
					 * mode" must be terminated because
					 * tab and history handlers are sharing
					 * the same array: temp_buff.
					 */
					z_flag_history_exit_set(sh, true);
					tab_handle(sh);
				}
				break;

			case SHELL_VT100_ASCII_BSPACE: /* BACKSPACE */
				if (z_flag_echo_get(sh)) {
					z_flag_history_exit_set(sh, true);
					z_shell_op_char_backspace(sh);
				}
				break;

			case SHELL_VT100_ASCII_DEL: /* DELETE */
				if (z_flag_echo_get(sh)) {
					z_flag_history_exit_set(sh, true);
					if (z_flag_mode_delete_get(sh)) {
						z_shell_op_char_backspace(sh);

					} else {
						z_shell_op_char_delete(sh);
					}
				}
				break;

			default:
				if (isprint((int) data) != 0) {
					z_flag_history_exit_set(sh, true);
					z_shell_op_char_insert(sh, data);
				} else if (z_flag_echo_get(sh)) {
					ctrl_metakeys_handle(sh, data);
				}
				break;
			}
			break;

		case SHELL_RECEIVE_ESC:
			if (data == '[') {
				receive_state_change(sh,
						SHELL_RECEIVE_ESC_SEQ);
				break;
			} else if (z_flag_echo_get(sh)) {
				alt_metakeys_handle(sh, data);
			}
			receive_state_change(sh, SHELL_RECEIVE_DEFAULT);
			break;

		case SHELL_RECEIVE_ESC_SEQ:
			receive_state_change(sh, SHELL_RECEIVE_DEFAULT);

			if (!z_flag_echo_get(sh)) {
				continue;
			}

			switch (data) {
			case 'A': /* UP arrow */
				history_handle(sh, true);
				break;

			case 'B': /* DOWN arrow */
				history_handle(sh, false);
				break;

			case 'C': /* RIGHT arrow */
				z_shell_op_right_arrow(sh);
				break;

			case 'D': /* LEFT arrow */
				z_shell_op_left_arrow(sh);
				break;

			case '4': /* END Button in ESC[n~ mode */
				receive_state_change(sh,
						SHELL_RECEIVE_TILDE_EXP);
				__fallthrough;
			case 'F': /* END Button in VT100 mode */
				z_shell_op_cursor_end_move(sh);
				break;

			case '1': /* HOME Button in ESC[n~ mode */
				receive_state_change(sh,
						SHELL_RECEIVE_TILDE_EXP);
				__fallthrough;
			case 'H': /* HOME Button in VT100 mode */
				z_shell_op_cursor_home_move(sh);
				break;

			case '2': /* INSERT Button in ESC[n~ mode */
				receive_state_change(sh,
						SHELL_RECEIVE_TILDE_EXP);
				__fallthrough;
			case 'L': {/* INSERT Button in VT100 mode */
				bool status = z_flag_insert_mode_get(sh);
				z_flag_insert_mode_set(sh, !status);
				break;
			}

			case '3':/* DELETE Button in ESC[n~ mode */
				receive_state_change(sh,
						SHELL_RECEIVE_TILDE_EXP);
				if (z_flag_echo_get(sh)) {
					z_shell_op_char_delete(sh);
				}
				break;

			default:
				break;
			}
			break;

		case SHELL_RECEIVE_TILDE_EXP:
			receive_state_change(sh, SHELL_RECEIVE_DEFAULT);
			break;

		default:
			receive_state_change(sh, SHELL_RECEIVE_DEFAULT);
			break;
		}
	}

	z_transport_buffer_flush(sh);
}

static void transport_evt_handler(enum shell_transport_evt evt_type, void *ctx)
{
	struct shell *sh = (struct shell *)ctx;
	struct k_poll_signal *signal;

	signal = (evt_type == SHELL_TRANSPORT_EVT_RX_RDY) ?
			&sh->ctx->signals[SHELL_SIGNAL_RXRDY] :
			&sh->ctx->signals[SHELL_SIGNAL_TXDONE];
	k_poll_signal_raise(signal, 0);
}

static void shell_log_process(const struct shell *sh)
{
	bool processed = false;
	int signaled = 0;
	int result;

	do {
		if (!IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
			z_shell_cmd_line_erase(sh);

			processed = z_shell_log_backend_process(
					sh->log_backend);
		}

		struct k_poll_signal *signal =
			&sh->ctx->signals[SHELL_SIGNAL_RXRDY];

		z_shell_print_prompt_and_cmd(sh);

		/* Arbitrary delay added to ensure that prompt is
		 * readable and can be used to enter further commands.
		 */
		if (sh->ctx->cmd_buff_len) {
			k_sleep(K_MSEC(15));
		}

		k_poll_signal_check(signal, &signaled, &result);

	} while (processed && !signaled);
}

static int instance_init(const struct shell *sh,
			 const void *transport_config,
			 struct shell_backend_config_flags cfg_flags)
{
	__ASSERT_NO_MSG((sh->shell_flag == SHELL_FLAG_CRLF_DEFAULT) ||
			(sh->shell_flag == SHELL_FLAG_OLF_CRLF));

	memset(sh->ctx, 0, sizeof(*sh->ctx));
	sh->ctx->prompt = sh->default_prompt;
	if (CONFIG_SHELL_CMD_ROOT[0]) {
		sh->ctx->selected_cmd = root_cmd_find(CONFIG_SHELL_CMD_ROOT);
	}

	history_init(sh);

	k_mutex_init(&sh->ctx->wr_mtx);

	for (int i = 0; i < SHELL_SIGNALS; i++) {
		k_poll_signal_init(&sh->ctx->signals[i]);
		k_poll_event_init(&sh->ctx->events[i],
				  K_POLL_TYPE_SIGNAL,
				  K_POLL_MODE_NOTIFY_ONLY,
				  &sh->ctx->signals[i]);
	}

	if (IS_ENABLED(CONFIG_SHELL_STATS)) {
		sh->stats->log_lost_cnt = 0;
	}

	z_flag_tx_rdy_set(sh, true);

	sh->ctx->vt100_ctx.cons.terminal_wid =
					CONFIG_SHELL_DEFAULT_TERMINAL_WIDTH;
	sh->ctx->vt100_ctx.cons.terminal_hei =
					CONFIG_SHELL_DEFAULT_TERMINAL_HEIGHT;
	sh->ctx->vt100_ctx.cons.name_len = z_shell_strlen(sh->ctx->prompt);

	/* Configure backend according to enabled shell features and backend
	 * specific settings.
	 */
	cfg_flags.obscure     &= IS_ENABLED(CONFIG_SHELL_START_OBSCURED);
	cfg_flags.use_colors  &= IS_ENABLED(CONFIG_SHELL_VT100_COLORS);
	cfg_flags.use_vt100   &= IS_ENABLED(CONFIG_SHELL_VT100_COMMANDS);
	cfg_flags.echo        &= IS_ENABLED(CONFIG_SHELL_ECHO_STATUS);
	cfg_flags.mode_delete &= IS_ENABLED(CONFIG_SHELL_BACKSPACE_MODE_DELETE);
	sh->ctx->cfg.flags = cfg_flags;

	int ret = sh->iface->api->init(sh->iface, transport_config,
				       transport_evt_handler,
				       (void *)sh);
	if (ret == 0) {
		state_set(sh, SHELL_STATE_INITIALIZED);
	}

	return ret;
}

static int instance_uninit(const struct shell *sh)
{
	__ASSERT_NO_MSG(sh);
	__ASSERT_NO_MSG(sh->ctx && sh->iface);

	int err;

	if (z_flag_processing_get(sh)) {
		return -EBUSY;
	}

	if (IS_ENABLED(CONFIG_SHELL_LOG_BACKEND)) {
		/* todo purge log queue */
		z_shell_log_backend_disable(sh->log_backend);
	}

	err = sh->iface->api->uninit(sh->iface);
	if (err != 0) {
		return err;
	}

	history_purge(sh);
	state_set(sh, SHELL_STATE_UNINITIALIZED);

	return 0;
}

typedef void (*shell_signal_handler_t)(const struct shell *sh);

static void shell_signal_handle(const struct shell *sh,
				enum shell_signal sig_idx,
				shell_signal_handler_t handler)
{
	struct k_poll_signal *signal = &sh->ctx->signals[sig_idx];
	int set;
	int res;

	k_poll_signal_check(signal, &set, &res);

	if (set) {
		k_poll_signal_reset(signal);
		handler(sh);
	}
}

static void kill_handler(const struct shell *sh)
{
	int err = instance_uninit(sh);

	if (sh->ctx->uninit_cb) {
		sh->ctx->uninit_cb(sh, err);
	}

	sh->ctx->tid = NULL;
	k_thread_abort(k_current_get());
}

void shell_thread(void *shell_handle, void *arg_log_backend,
		  void *arg_log_level)
{
	struct shell *sh = shell_handle;
	bool log_backend = (bool)arg_log_backend;
	uint32_t log_level = POINTER_TO_UINT(arg_log_level);
	int err;

	err = sh->iface->api->enable(sh->iface, false);
	if (err != 0) {
		return;
	}

	if (IS_ENABLED(CONFIG_SHELL_LOG_BACKEND) && log_backend
	    && !IS_ENABLED(CONFIG_SHELL_START_OBSCURED)) {
		z_shell_log_backend_enable(sh->log_backend, (void *)sh,
					   log_level);
	}

	if (IS_ENABLED(CONFIG_SHELL_AUTOSTART)) {
		/* Enable shell and print prompt. */
		err = shell_start(sh);
		if (err != 0) {
			return;
		}
	}

	while (true) {
		/* waiting for all signals except SHELL_SIGNAL_TXDONE */
		err = k_poll(sh->ctx->events, SHELL_SIGNAL_TXDONE,
			     K_FOREVER);

		if (err != 0) {
			k_mutex_lock(&sh->ctx->wr_mtx, K_FOREVER);
			z_shell_fprintf(sh, SHELL_ERROR,
					"Shell thread error: %d", err);
			k_mutex_unlock(&sh->ctx->wr_mtx);
			return;
		}

		k_mutex_lock(&sh->ctx->wr_mtx, K_FOREVER);

		shell_signal_handle(sh, SHELL_SIGNAL_KILL, kill_handler);
		shell_signal_handle(sh, SHELL_SIGNAL_RXRDY, shell_process);
		if (IS_ENABLED(CONFIG_SHELL_LOG_BACKEND)) {
			shell_signal_handle(sh, SHELL_SIGNAL_LOG_MSG,
					    shell_log_process);
		}

		if (sh->iface->api->update) {
			sh->iface->api->update(sh->iface);
		}

		k_mutex_unlock(&sh->ctx->wr_mtx);
	}
}

int shell_init(const struct shell *sh, const void *transport_config,
	       struct shell_backend_config_flags cfg_flags,
	       bool log_backend, uint32_t init_log_level)
{
	__ASSERT_NO_MSG(sh);
	__ASSERT_NO_MSG(sh->ctx && sh->iface && sh->default_prompt);

	if (sh->ctx->tid) {
		return -EALREADY;
	}

	int err = instance_init(sh, transport_config, cfg_flags);

	if (err != 0) {
		return err;
	}

	k_tid_t tid = k_thread_create(sh->thread,
				  sh->stack, CONFIG_SHELL_STACK_SIZE,
				  shell_thread, (void *)sh, (void *)log_backend,
				  UINT_TO_POINTER(init_log_level),
				  SHELL_THREAD_PRIORITY, 0, K_NO_WAIT);

	sh->ctx->tid = tid;
	k_thread_name_set(tid, sh->thread_name);

	return 0;
}

void shell_uninit(const struct shell *sh, shell_uninit_cb_t cb)
{
	__ASSERT_NO_MSG(sh);

	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct k_poll_signal *signal =
				&sh->ctx->signals[SHELL_SIGNAL_KILL];

		sh->ctx->uninit_cb = cb;
		/* signal kill message */
		(void)k_poll_signal_raise(signal, 0);

		return;
	}

	int err = instance_uninit(sh);

	if (cb) {
		cb(sh, err);
	} else {
		__ASSERT_NO_MSG(0);
	}
}

int shell_start(const struct shell *sh)
{
	__ASSERT_NO_MSG(sh);
	__ASSERT_NO_MSG(sh->ctx && sh->iface && sh->default_prompt);

	if (state_get(sh) != SHELL_STATE_INITIALIZED) {
		return -ENOTSUP;
	}

	k_mutex_lock(&sh->ctx->wr_mtx, K_FOREVER);

	if (IS_ENABLED(CONFIG_SHELL_VT100_COLORS)) {
		z_shell_vt100_color_set(sh, SHELL_NORMAL);
	}

	if (z_shell_strlen(sh->default_prompt) > 0) {
		z_shell_raw_fprintf(sh->fprintf_ctx, "\n\n");
	}
	state_set(sh, SHELL_STATE_ACTIVE);

	k_mutex_unlock(&sh->ctx->wr_mtx);

	return 0;
}

int shell_stop(const struct shell *sh)
{
	__ASSERT_NO_MSG(sh);
	__ASSERT_NO_MSG(sh->ctx);

	enum shell_state state = state_get(sh);

	if ((state == SHELL_STATE_INITIALIZED) ||
	    (state == SHELL_STATE_UNINITIALIZED)) {
		return -ENOTSUP;
	}

	state_set(sh, SHELL_STATE_INITIALIZED);

	return 0;
}

void shell_process(const struct shell *sh)
{
	__ASSERT_NO_MSG(sh);
	__ASSERT_NO_MSG(sh->ctx);

	/* atomically set the processing flag */
	z_flag_processing_set(sh, true);

	switch (sh->ctx->state) {
	case SHELL_STATE_UNINITIALIZED:
	case SHELL_STATE_INITIALIZED:
		/* Console initialized but not started. */
		break;

	case SHELL_STATE_ACTIVE:
		state_collect(sh);
		break;
	default:
		break;
	}

	/* atomically clear the processing flag */
	z_flag_processing_set(sh, false);
}

/* This function mustn't be used from shell context to avoid deadlock.
 * However it can be used in shell command handlers.
 */
void shell_vfprintf(const struct shell *sh, enum shell_vt100_color color,
		   const char *fmt, va_list args)
{
	__ASSERT_NO_MSG(sh);
	__ASSERT(!k_is_in_isr(), "Thread context required.");
	__ASSERT_NO_MSG(sh->ctx);
	__ASSERT_NO_MSG(z_flag_cmd_ctx_get(sh) ||
			(k_current_get() != sh->ctx->tid));
	__ASSERT_NO_MSG(sh->fprintf_ctx);
	__ASSERT_NO_MSG(fmt);

	/* Sending a message to a non-active shell leads to a dead lock. */
	if (state_get(sh) != SHELL_STATE_ACTIVE) {
		z_flag_print_noinit_set(sh, true);
		return;
	}

	k_mutex_lock(&sh->ctx->wr_mtx, K_FOREVER);
	if (!z_flag_cmd_ctx_get(sh) && !sh->ctx->bypass) {
		z_shell_cmd_line_erase(sh);
	}
	z_shell_vfprintf(sh, color, fmt, args);
	if (!z_flag_cmd_ctx_get(sh) && !sh->ctx->bypass) {
		z_shell_print_prompt_and_cmd(sh);
	}
	z_transport_buffer_flush(sh);
	k_mutex_unlock(&sh->ctx->wr_mtx);
}

/* This function mustn't be used from shell context to avoid deadlock.
 * However it can be used in shell command handlers.
 */
void shell_fprintf(const struct shell *sh, enum shell_vt100_color color,
		   const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	shell_vfprintf(sh, color, fmt, args);
	va_end(args);
}

void shell_hexdump_line(const struct shell *sh, unsigned int offset,
			const uint8_t *data, size_t len)
{
	__ASSERT_NO_MSG(sh);

	int i;

	shell_fprintf(sh, SHELL_NORMAL, "%08X: ", offset);

	for (i = 0; i < SHELL_HEXDUMP_BYTES_IN_LINE; i++) {
		if (i > 0 && !(i % 8)) {
			shell_fprintf(sh, SHELL_NORMAL, " ");
		}

		if (i < len) {
			shell_fprintf(sh, SHELL_NORMAL, "%02x ",
				      data[i] & 0xFF);
		} else {
			shell_fprintf(sh, SHELL_NORMAL, "   ");
		}
	}

	shell_fprintf(sh, SHELL_NORMAL, "|");

	for (i = 0; i < SHELL_HEXDUMP_BYTES_IN_LINE; i++) {
		if (i > 0 && !(i % 8)) {
			shell_fprintf(sh, SHELL_NORMAL, " ");
		}

		if (i < len) {
			char c = data[i];

			shell_fprintf(sh, SHELL_NORMAL, "%c",
				      isprint((int)c) != 0 ? c : '.');
		} else {
			shell_fprintf(sh, SHELL_NORMAL, " ");
		}
	}

	shell_print(sh, "|");
}

void shell_hexdump(const struct shell *sh, const uint8_t *data, size_t len)
{
	__ASSERT_NO_MSG(sh);

	const uint8_t *p = data;
	size_t line_len;

	while (len) {
		line_len = MIN(len, SHELL_HEXDUMP_BYTES_IN_LINE);

		shell_hexdump_line(sh, p - data, p, line_len);

		len -= line_len;
		p += line_len;
	}
}

int shell_prompt_change(const struct shell *sh, const char *prompt)
{
	__ASSERT_NO_MSG(sh);

	if (prompt == NULL) {
		return -EINVAL;
	}
	sh->ctx->prompt = prompt;
	sh->ctx->vt100_ctx.cons.name_len = z_shell_strlen(prompt);

	return 0;
}

void shell_help(const struct shell *sh)
{
	k_mutex_lock(&sh->ctx->wr_mtx, K_FOREVER);
	shell_internal_help_print(sh);
	k_mutex_unlock(&sh->ctx->wr_mtx);
}

int shell_execute_cmd(const struct shell *sh, const char *cmd)
{
	uint16_t cmd_len = z_shell_strlen(cmd);
	int ret_val;

	if (cmd == NULL) {
		return -ENOEXEC;
	}

	if (cmd_len > (CONFIG_SHELL_CMD_BUFF_SIZE - 1)) {
		return -ENOMEM;
	}

	if (sh == NULL) {
#if defined(CONFIG_SHELL_BACKEND_DUMMY)
		sh = shell_backend_dummy_get_ptr();
#else
		return -EINVAL;
#endif
	}

	__ASSERT(!z_flag_cmd_ctx_get(sh), "Function cannot be called"
					  " from command context");

	strcpy(sh->ctx->cmd_buff, cmd);
	sh->ctx->cmd_buff_len = cmd_len;
	sh->ctx->cmd_buff_pos = cmd_len;

	k_mutex_lock(&sh->ctx->wr_mtx, K_FOREVER);
	ret_val = execute(sh);
	k_mutex_unlock(&sh->ctx->wr_mtx);

	cmd_buffer_clear(sh);

	return ret_val;
}

int shell_insert_mode_set(const struct shell *sh, bool val)
{
	if (sh == NULL) {
		return -EINVAL;
	}

	return (int)z_flag_insert_mode_set(sh, val);
}

int shell_use_colors_set(const struct shell *sh, bool val)
{
	if (sh == NULL) {
		return -EINVAL;
	}

	return (int)z_flag_use_colors_set(sh, val);
}

int shell_use_vt100_set(const struct shell *sh, bool val)
{
	if (sh == NULL) {
		return -EINVAL;
	}

	return (int)z_flag_use_vt100_set(sh, val);
}

int shell_echo_set(const struct shell *sh, bool val)
{
	if (sh == NULL) {
		return -EINVAL;
	}

	return (int)z_flag_echo_set(sh, val);
}

int shell_obscure_set(const struct shell *sh, bool val)
{
	if (sh == NULL) {
		return -EINVAL;
	}

	return (int)z_flag_obscure_set(sh, val);
}

int shell_mode_delete_set(const struct shell *sh, bool val)
{
	if (sh == NULL) {
		return -EINVAL;
	}

	return (int)z_flag_mode_delete_set(sh, val);
}

void shell_set_bypass(const struct shell *sh, shell_bypass_cb_t bypass)
{
	__ASSERT_NO_MSG(sh);

	sh->ctx->bypass = bypass;

	if (bypass == NULL) {
		cmd_buffer_clear(sh);
	}
}

bool shell_ready(const struct shell *sh)
{
	__ASSERT_NO_MSG(sh);

	return state_get(sh) ==	SHELL_STATE_ACTIVE;
}

static int cmd_help(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_SHELL_TAB)
	shell_print(sh, "Please press the <Tab> button to see all available "
			   "commands.");
#endif

#if defined(CONFIG_SHELL_TAB_AUTOCOMPLETION)
	shell_print(sh,
		"You can also use the <Tab> button to prompt or auto-complete"
		" all commands or its subcommands.");
#endif

#if defined(CONFIG_SHELL_HELP)
	shell_print(sh,
		"You can try to call commands with <-h> or <--help> parameter"
		" for more information.");
#endif

#if defined(CONFIG_SHELL_METAKEYS)
	shell_print(sh,
		"\nShell supports following meta-keys:\n"
		"  Ctrl + (a key from: abcdefklnpuw)\n"
		"  Alt  + (a key from: bf)\n"
		"Please refer to shell documentation for more details.");
#endif

	if (IS_ENABLED(CONFIG_SHELL_HELP)) {
		/* For NULL argument function will print all root commands */
		z_shell_help_subcmd_print(sh, NULL,
					 "\nAvailable commands:\n");
	} else {
		const struct shell_static_entry *entry;
		size_t idx = 0;

		shell_print(sh, "\nAvailable commands:");
		while ((entry = z_shell_cmd_get(NULL, idx++, NULL)) != NULL) {
			shell_print(sh, "  %s", entry->syntax);
		}
	}

	return 0;
}

SHELL_CMD_ARG_REGISTER(help, NULL, "Prints the help message.", cmd_help, 1, 0);
