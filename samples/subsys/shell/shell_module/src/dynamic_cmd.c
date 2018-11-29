/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <ctype.h>
#include <stdio.h>

#define MAX_CMD_CNT (20u)
#define MAX_CMD_LEN (33u)

/* buffer holding dynamicly created user commands */
static char dynamic_cmd_buffer[MAX_CMD_CNT][MAX_CMD_LEN];
/* commands counter */
static u8_t dynamic_cmd_cnt;

typedef int cmp_t(const void *, const void *);
extern void qsort(void *a, size_t n, size_t es, cmp_t *cmp);

static int cmd_dynamic(const struct shell *shell, size_t argc, char **argv)
{
	if ((argc == 1) || shell_help_requested(shell)) {
		shell_help_print(shell, NULL, 0);
		return 0;
	}

	if (argc > 2) {
		shell_fprintf(shell, SHELL_ERROR,
			      "%s: bad parameter count\r\n", argv[0]);
	} else {
		shell_fprintf(shell, SHELL_ERROR,
			      "%s: please specify subcommand\r\n", argv[0]);
	}

	return -ENOEXEC;
}

/* function required by qsort */
static int string_cmp(const void *p_a, const void *p_b)
{
	return strcmp((const char *)p_a, (const char *)p_b);
}

static int cmd_dynamic_add(const struct shell *shell,
			   size_t argc, char **argv)
{
	u8_t idx;
	u16_t cmd_len;

	if (shell_help_requested(shell)) {
		shell_help_print(shell, NULL, 0);
		return 0;
	}

	if (argc != 2) {
		shell_fprintf(shell, SHELL_ERROR,
			      "%s: bad parameter count\r\n", argv[0]);
		return -ENOEXEC;
	}

	if (dynamic_cmd_cnt >= MAX_CMD_CNT) {
		shell_fprintf(shell, SHELL_ERROR, "command limit reached\r\n");
		return -ENOEXEC;
	}

	cmd_len = strlen(argv[1]);

	if (cmd_len >= MAX_CMD_LEN) {
		shell_fprintf(shell, SHELL_ERROR, "too long command\r\n");
		return -ENOEXEC;
	}

	for (idx = 0U; idx < cmd_len; idx++) {
		if (!isalnum((int)(argv[1][idx]))) {
			shell_fprintf(shell, SHELL_ERROR,
				     "bad command name - please use only"
				     " alphanumerical characters\r\n");
			return -ENOEXEC;
		}
	}

	for (idx = 0U; idx < MAX_CMD_CNT; idx++) {
		if (!strcmp(dynamic_cmd_buffer[idx], argv[1])) {
			shell_fprintf(shell, SHELL_ERROR,
				      "duplicated command\r\n");
			return -ENOEXEC;
		}
	}

	sprintf(dynamic_cmd_buffer[dynamic_cmd_cnt++], "%s", argv[1]);

	qsort(dynamic_cmd_buffer, dynamic_cmd_cnt,
	      sizeof(dynamic_cmd_buffer[0]), string_cmp);

	shell_fprintf(shell, SHELL_NORMAL, "command added successfully\r\n");

	return 0;
}

static int cmd_dynamic_show(const struct shell *shell,
			    size_t argc, char **argv)
{
	if (shell_help_requested(shell)) {
		shell_help_print(shell, NULL, 0);
		return 0;
	}

	if (argc != 1) {
		shell_fprintf(shell, SHELL_ERROR,
			      "%s: bad parameter count\r\n", argv[0]);
		return -ENOEXEC;
	}

	if (dynamic_cmd_cnt == 0) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Please add some commands first.\r\n");
		return -ENOEXEC;
	}

	shell_fprintf(shell, SHELL_NORMAL, "Dynamic command list:\r\n");

	for (u8_t i = 0; i < dynamic_cmd_cnt; i++) {
		shell_fprintf(shell, SHELL_NORMAL,
			      "[%3d] %s\r\n", i, dynamic_cmd_buffer[i]);
	}

	return 0;
}

static int cmd_dynamic_execute(const struct shell *shell,
			       size_t argc, char **argv)
{
	if (shell_help_requested(shell)) {
		shell_help_print(shell, NULL, 0);
		return 0;
	}

	if (argc != 2) {
		shell_fprintf(shell, SHELL_ERROR,
			      "%s: bad parameter count\r\n", argv[0]);
		return -ENOEXEC;
	}

	for (u8_t idx = 0; idx <  dynamic_cmd_cnt; idx++) {
		if (!strcmp(dynamic_cmd_buffer[idx], argv[1])) {
			shell_fprintf(shell, SHELL_NORMAL,
				      "dynamic command: %s\r\n", argv[1]);
			return 0;
		}
	}

	shell_fprintf(shell, SHELL_ERROR,
		      "%s: uknown parameter: %s\r\n", argv[0], argv[1]);

	return -ENOEXEC;
}

static int cmd_dynamic_remove(const struct shell *shell, size_t argc,
			      char **argv)
{
	if ((argc == 1) || shell_help_requested(shell)) {
		shell_help_print(shell, NULL, 0);
		return 0;
	}

	if (argc != 2) {
		shell_fprintf(shell, SHELL_ERROR,
			      "%s: bad parameter count\r\n", argv[0]);
		return -ENOEXEC;
	}

	for (u8_t idx = 0; idx <  dynamic_cmd_cnt; idx++) {
		if (!strcmp(dynamic_cmd_buffer[idx], argv[1])) {
			if (idx == MAX_CMD_CNT - 1) {
				dynamic_cmd_buffer[idx][0] = '\0';
			} else {
				memmove(dynamic_cmd_buffer[idx],
					dynamic_cmd_buffer[idx + 1],
					sizeof(dynamic_cmd_buffer[idx]) *
					(dynamic_cmd_cnt - idx));
			}

			--dynamic_cmd_cnt;
			shell_fprintf(shell, SHELL_NORMAL,
				      "command removed successfully\r\n");
			return 0;
		}
	}
	shell_fprintf(shell, SHELL_ERROR,
		      "did not find command: %s\r\n", argv[1]);

	return -ENOEXEC;
}

/* dynamic command creation */
static void dynamic_cmd_get(size_t idx, struct shell_static_entry *entry)
{
	if (idx < dynamic_cmd_cnt) {
		/* m_dynamic_cmd_buffer must be sorted alphabetically to ensure
		 * correct CLI completion
		 */
		entry->syntax = dynamic_cmd_buffer[idx];
		entry->handler  = NULL;
		entry->subcmd = NULL;
		entry->help = "Show dynamic command name.";
	} else {
		/* if there are no more dynamic commands available syntax
		 * must be set to NULL.
		 */
		entry->syntax = NULL;
	}
}

SHELL_CREATE_DYNAMIC_CMD(m_sub_dynamic_set, dynamic_cmd_get);
SHELL_CREATE_STATIC_SUBCMD_SET(m_sub_dynamic)
{
	SHELL_CMD(add, NULL,
		"Add a new dynamic command.\nExample usage: [ dynamic add test "
		"] will add a dynamic command 'test'.\nIn this example, command"
		" name length is limited to 32 chars. You can add up to 20"
		" commands. Commands are automatically sorted to ensure correct"
		" shell completion.",
		cmd_dynamic_add),
	SHELL_CMD(execute, &m_sub_dynamic_set,
		"Execute a command.", cmd_dynamic_execute),
	SHELL_CMD(remove, &m_sub_dynamic_set,
		"Remove a command.", cmd_dynamic_remove),
	SHELL_CMD(show, NULL,
		"Show all added dynamic commands.", cmd_dynamic_show),
	SHELL_SUBCMD_SET_END
};

SHELL_CMD_REGISTER(dynamic, &m_sub_dynamic,
		   "Demonstrate dynamic command usage.", cmd_dynamic);
