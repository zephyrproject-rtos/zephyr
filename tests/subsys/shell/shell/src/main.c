/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Interactive shell test suite
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>

#define MAX_CMD_SYNTAX_LEN	(11)
static char dynamic_cmd_buffer[][MAX_CMD_SYNTAX_LEN] = {
		"dynamic",
		"command"
};

static void test_shell_execute_cmd(const char *cmd, int result)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int ret;

	ret = shell_execute_cmd(sh, cmd);

	TC_PRINT("shell_execute_cmd(%s): %d\n", cmd, ret);

	zassert_true(ret == result, "cmd: %s, got:%d, expected:%d",
							cmd, ret, result);
}

ZTEST(shell_1cpu, test_cmd_help)
{
	test_shell_execute_cmd("help", 0);
	test_shell_execute_cmd("help -h", 1);
	test_shell_execute_cmd("help --help", 1);
	test_shell_execute_cmd("help dummy", -EINVAL);
	test_shell_execute_cmd("help dummy dummy", -EINVAL);
}

ZTEST(shell, test_cmd_clear)
{
	test_shell_execute_cmd("clear", 0);
	test_shell_execute_cmd("clear -h", 1);
	test_shell_execute_cmd("clear --help", 1);
	test_shell_execute_cmd("clear dummy", -EINVAL);
	test_shell_execute_cmd("clear dummy dummy", -EINVAL);
}

ZTEST(shell, test_cmd_shell)
{
	test_shell_execute_cmd("shell -h", 1);
	test_shell_execute_cmd("shell --help", 1);
	test_shell_execute_cmd("shell dummy", 1);
	test_shell_execute_cmd("shell dummy dummy", 1);

	/* subcommand: backspace_mode */
	test_shell_execute_cmd("shell backspace_mode -h", 1);
	test_shell_execute_cmd("shell backspace_mode --help", 1);
	test_shell_execute_cmd("shell backspace_mode dummy", 1);

	test_shell_execute_cmd("shell backspace_mode backspace", 0);
	test_shell_execute_cmd("shell backspace_mode backspace -h", 1);
	test_shell_execute_cmd("shell backspace_mode backspace --help", 1);
	test_shell_execute_cmd("shell backspace_mode backspace dummy", -EINVAL);
	test_shell_execute_cmd("shell backspace_mode backspace dummy dummy",
				-EINVAL);

	test_shell_execute_cmd("shell backspace_mode delete", 0);
	test_shell_execute_cmd("shell backspace_mode delete -h", 1);
	test_shell_execute_cmd("shell backspace_mode delete --help", 1);
	test_shell_execute_cmd("shell backspace_mode delete dummy", -EINVAL);
	test_shell_execute_cmd("shell backspace_mode delete dummy dummy",
				-EINVAL);

	/* subcommand: colors */
	test_shell_execute_cmd("shell colors -h", 1);
	test_shell_execute_cmd("shell colors --help", 1);
	test_shell_execute_cmd("shell colors dummy", 1);
	test_shell_execute_cmd("shell colors dummy dummy", 1);

	test_shell_execute_cmd("shell colors off", 0);
	test_shell_execute_cmd("shell colors off -h", 1);
	test_shell_execute_cmd("shell colors off --help", 1);
	test_shell_execute_cmd("shell colors off dummy", -EINVAL);
	test_shell_execute_cmd("shell colors off dummy dummy", -EINVAL);

	test_shell_execute_cmd("shell colors on", 0);
	test_shell_execute_cmd("shell colors on -h", 1);
	test_shell_execute_cmd("shell colors on --help", 1);
	test_shell_execute_cmd("shell colors on dummy", -EINVAL);
	test_shell_execute_cmd("shell colors on dummy dummy", -EINVAL);

	/* subcommand: echo */
	test_shell_execute_cmd("shell echo", 0);
	test_shell_execute_cmd("shell echo -h", 1);
	test_shell_execute_cmd("shell echo --help", 1);
	test_shell_execute_cmd("shell echo dummy", -EINVAL);
	test_shell_execute_cmd("shell echo dummy dummy", -EINVAL);

	test_shell_execute_cmd("shell echo off", 0);
	test_shell_execute_cmd("shell echo off -h", 1);
	test_shell_execute_cmd("shell echo off --help", 1);
	test_shell_execute_cmd("shell echo off dummy", -EINVAL);
	test_shell_execute_cmd("shell echo off dummy dummy", -EINVAL);

	test_shell_execute_cmd("shell echo on", 0);
	test_shell_execute_cmd("shell echo on -h", 1);
	test_shell_execute_cmd("shell echo on --help", 1);
	test_shell_execute_cmd("shell echo on dummy", -EINVAL);
	test_shell_execute_cmd("shell echo on dummy dummy", -EINVAL);

	/* subcommand: stats */
	test_shell_execute_cmd("shell stats", 1);
	test_shell_execute_cmd("shell stats -h", 1);
	test_shell_execute_cmd("shell stats --help", 1);
	test_shell_execute_cmd("shell stats dummy", 1);
	test_shell_execute_cmd("shell stats dummy dummy", 1);

	test_shell_execute_cmd("shell stats reset", 0);
	test_shell_execute_cmd("shell stats reset -h", 1);
	test_shell_execute_cmd("shell stats reset --help", 1);
	test_shell_execute_cmd("shell stats reset dummy", -EINVAL);
	test_shell_execute_cmd("shell stats reset dummy dummy", -EINVAL);

	test_shell_execute_cmd("shell stats show", 0);
	test_shell_execute_cmd("shell stats show -h", 1);
	test_shell_execute_cmd("shell stats show --help", 1);
	test_shell_execute_cmd("shell stats show dummy", -EINVAL);
	test_shell_execute_cmd("shell stats show dummy dummy", -EINVAL);
}

ZTEST(shell, test_cmd_history)
{
	test_shell_execute_cmd("history", 0);
	test_shell_execute_cmd("history -h", 1);
	test_shell_execute_cmd("history --help", 1);
	test_shell_execute_cmd("history dummy", -EINVAL);
	test_shell_execute_cmd("history dummy dummy", -EINVAL);
}

ZTEST(shell, test_cmd_resize)
{
	test_shell_execute_cmd("resize -h", 1);
	test_shell_execute_cmd("resize --help", 1);
	test_shell_execute_cmd("resize dummy", -EINVAL);
	test_shell_execute_cmd("resize dummy dummy", -EINVAL);

	/* subcommand: default */
	test_shell_execute_cmd("resize default", 0);
	test_shell_execute_cmd("resize default -h", 1);
	test_shell_execute_cmd("resize default --help", 1);
	test_shell_execute_cmd("resize default dummy", -EINVAL);
	test_shell_execute_cmd("resize default dummy dummy", -EINVAL);
}

ZTEST(shell, test_shell_module)
{
	test_shell_execute_cmd("test_shell_cmd", 0);
	test_shell_execute_cmd("test_shell_cmd -h", 1);
	test_shell_execute_cmd("test_shell_cmd --help", 1);
	test_shell_execute_cmd("test_shell_cmd dummy", -EINVAL);
	test_shell_execute_cmd("test_shell_cmd dummy dummy", -EINVAL);

	test_shell_execute_cmd("", -ENOEXEC); /* empty command */
	test_shell_execute_cmd("not existing command", -ENOEXEC);
}

/* test wildcard and static subcommands */
ZTEST(shell, test_shell_wildcards_static)
{
	test_shell_execute_cmd("test_wildcard", 0);
	test_shell_execute_cmd("test_wildcard argument_1", 1);
	test_shell_execute_cmd("test_wildcard argument?1", 1);
	test_shell_execute_cmd("test_wildcard argu?ent?1", 1);
	test_shell_execute_cmd("test_wildcard a*1", 1);
	test_shell_execute_cmd("test_wildcard ar?u*1", 1);

	test_shell_execute_cmd("test_wildcard *", 3);
	test_shell_execute_cmd("test_wildcard a*", 2);
}

/* test wildcard and dynamic subcommands */
ZTEST(shell, test_shell_wildcards_dynamic)
{
	test_shell_execute_cmd("test_dynamic", 0);
	test_shell_execute_cmd("test_dynamic d*", 1);
	test_shell_execute_cmd("test_dynamic c*", 1);
	test_shell_execute_cmd("test_dynamic d* c*", 2);
}


static int cmd_test_module(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(argc);

	return 0;
}
SHELL_CMD_ARG_REGISTER(test_shell_cmd, NULL, "help", cmd_test_module, 1, 0);


static int cmd_wildcard(const struct shell *shell, size_t argc, char **argv)
{
	int valid_arguments = 0;
	for (size_t i = 1; i < argc; i++) {
		if (!strcmp("argument_1", argv[i])) {
			valid_arguments++;
			continue;
		}
		if (!strcmp("argument_2", argv[i])) {
			valid_arguments++;
			continue;
		}
		if (!strcmp("dummy", argv[i])) {
			valid_arguments++;
		}
	}

	return valid_arguments;
}

SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_test_shell_cmdl,
	SHELL_CMD(argument_1, NULL, NULL, NULL),
	SHELL_CMD(argument_2, NULL, NULL, NULL),
	SHELL_CMD(dummy, NULL, NULL, NULL),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(test_wildcard, &m_sub_test_shell_cmdl, NULL, cmd_wildcard);


static int cmd_dynamic(const struct shell *shell, size_t argc, char **argv)
{
	int valid_arguments = 0;

	for (size_t i = 1; i < argc; i++) {
		if (!strcmp(dynamic_cmd_buffer[0], argv[i])) {
			valid_arguments++;
			continue;
		}
		if (!strcmp(dynamic_cmd_buffer[1], argv[i])) {
			valid_arguments++;
		}
	}


	return valid_arguments;
}
/* dynamic command creation */
static void dynamic_cmd_get(size_t idx, struct shell_static_entry *entry)
{
	if (idx < ARRAY_SIZE(dynamic_cmd_buffer)) {
		/* m_dynamic_cmd_buffer must be sorted alphabetically to ensure
		 * correct CLI completion
		 */
		entry->syntax = dynamic_cmd_buffer[idx];
		entry->handler  = NULL;
		entry->subcmd = NULL;
		entry->help = NULL;
	} else {
		/* if there are no more dynamic commands available syntax
		 * must be set to NULL.
		 */
		entry->syntax = NULL;
	}
}

SHELL_DYNAMIC_CMD_CREATE(m_sub_test_dynamic, dynamic_cmd_get);
SHELL_CMD_REGISTER(test_dynamic, &m_sub_test_dynamic, NULL, cmd_dynamic);

static void unselect_cmd(void)
{
	/* Unselecting command <shell color> */
	const struct shell *shell = shell_backend_dummy_get_ptr();

	shell->ctx->selected_cmd = NULL;
}

ZTEST(shell, test_cmd_select)
{
	unselect_cmd();
	test_shell_execute_cmd("select -h", 1);
	test_shell_execute_cmd("select clear", -EINVAL);
	test_shell_execute_cmd("off", -ENOEXEC);
	test_shell_execute_cmd("on", -ENOEXEC);
	test_shell_execute_cmd("select shell colors", 0);
	test_shell_execute_cmd("off", 0);
	test_shell_execute_cmd("on", 0);
	unselect_cmd();
	test_shell_execute_cmd("off", -ENOEXEC);
	test_shell_execute_cmd("on", -ENOEXEC);
}

ZTEST(shell, test_set_root_cmd)
{
	int err;

	test_shell_execute_cmd("shell colors on", 0);
	err = shell_set_root_cmd("__shell__");
	zassert_equal(err, -EINVAL, "Unexpected error %d", err);

	err = shell_set_root_cmd("shell");
	zassert_equal(err, 0, "Unexpected error %d", err);

	test_shell_execute_cmd("shell colors", 1);
	test_shell_execute_cmd("colors on", 0);

	err = shell_set_root_cmd(NULL);
	zassert_equal(err, 0, "Unexpected error %d", err);

	test_shell_execute_cmd("colors", -ENOEXEC);
	test_shell_execute_cmd("shell colors on", 0);
}

ZTEST(shell, test_shell_fprintf)
{
	static const char expect[] = "testing 1 2 3";
	const struct shell *shell;
	const char *buf;
	size_t size;

	shell = shell_backend_dummy_get_ptr();
	zassert_not_null(shell, "Failed to get shell");

	/* Clear the output buffer */
	shell_backend_dummy_clear_output(shell);

	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, "testing %d %s %c",
		      1, "2", '3');
	buf = shell_backend_dummy_get_output(shell, &size);
	zassert_true(size >= sizeof(expect), "Expected size > %u, got %d",
		     sizeof(expect), size);

	/*
	 * There are prompts and various ANSI characters in the output, so just
	 * check that the string is in there somewhere.
	 */
	zassert_true(strstr(buf, expect),
		     "Expected string to contain '%s', got '%s'", expect, buf);
}

#define RAW_ARG "aaa \"\" bbb"
#define CMD_MAND_1_OPT_RAW_NAME cmd_mand_1_opt_raw

static int cmd_mand_1_opt_raw_handler(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 2) {
		if (strcmp(argv[0], STRINGIFY(CMD_MAND_1_OPT_RAW_NAME))) {
			return -1;
		}
		if (strcmp(argv[1], RAW_ARG)) {
			return -1;
		}
	} else if (argc > 2) {
		return -1;
	}

	return 0;
}

SHELL_CMD_ARG_REGISTER(CMD_MAND_1_OPT_RAW_NAME, NULL, NULL, cmd_mand_1_opt_raw_handler, 1,
		       SHELL_OPT_ARG_RAW);

ZTEST(shell, test_cmd_mand_1_opt_raw)
{
	test_shell_execute_cmd("cmd_mand_1_opt_raw aaa \"\" bbb", 0);
	test_shell_execute_cmd("cmd_mand_1_opt_raw", 0);
	test_shell_execute_cmd("select cmd_mand_1_opt_raw", 0);
	test_shell_execute_cmd("aaa \"\" bbb", 0);
	shell_set_root_cmd(NULL);
}

#define CMD_MAND_2_OPT_RAW_NAME cmd_mand_2_opt_raw

static int cmd_mand_2_opt_raw_handler(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2 || argc > 3) {
		return -1;
	}

	if (strcmp(argv[0], STRINGIFY(CMD_MAND_2_OPT_RAW_NAME))) {
		return -1;
	}

	if (argc >= 2 && strcmp(argv[1], "mandatory")) {
		return -1;
	}

	if (argc == 3 && strcmp(argv[2], RAW_ARG)) {
		return -1;
	}

	return 0;
}

SHELL_CMD_ARG_REGISTER(CMD_MAND_2_OPT_RAW_NAME, NULL, NULL, cmd_mand_2_opt_raw_handler, 2,
		       SHELL_OPT_ARG_RAW);

ZTEST(shell, test_mand_2_opt_raw)
{
	test_shell_execute_cmd("cmd_mand_2_opt_raw", -EINVAL);
	test_shell_execute_cmd("cmd_mand_2_opt_raw mandatory", 0);
	test_shell_execute_cmd("cmd_mand_2_opt_raw mandatory aaa \"\" bbb", 0);
	test_shell_execute_cmd("select cmd_mand_2_opt_raw", 0);
	test_shell_execute_cmd("", -ENOEXEC);
	test_shell_execute_cmd("mandatory", 0);
	test_shell_execute_cmd("mandatory aaa \"\" bbb", 0);
	shell_set_root_cmd(NULL);
}

static int cmd_dummy(const struct shell *shell, size_t argc, char **argv)
{
	return 0;
}

SHELL_CMD_REGISTER(dummy, NULL, NULL, cmd_dummy);

ZTEST(shell, test_max_argc)
{
	BUILD_ASSERT(CONFIG_SHELL_ARGC_MAX == 20,
		     "Unexpected test configuration.");

	test_shell_execute_cmd("dummy 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19", 0);
	test_shell_execute_cmd("dummy 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15"
			       " 16 17 18 19 20",
			       -ENOEXEC);
}

static int cmd_handler_dict_1(const struct shell *sh, size_t argc, char **argv, void *data)
{
	int n = (intptr_t)data;

	return n;
}

static int cmd_handler_dict_2(const struct shell *sh, size_t argc, char **argv, void *data)
{
	int n = (intptr_t)data;

	return n + n;
}

SHELL_SUBCMD_DICT_SET_CREATE(dict1, cmd_handler_dict_1, (one, 1, "one"), (two, 2, "two"));
SHELL_SUBCMD_DICT_SET_CREATE(dict2, cmd_handler_dict_2, (one, 1, "one"), (two, 2, "two"));

SHELL_CMD_REGISTER(dict1, &dict1, NULL, NULL);
SHELL_CMD_REGISTER(dict2, &dict2, NULL, NULL);

ZTEST(shell, test_cmd_dict)
{
	test_shell_execute_cmd("dict1 one", 1);
	test_shell_execute_cmd("dict1 two", 2);

	test_shell_execute_cmd("dict2 one", 2);
	test_shell_execute_cmd("dict2 two", 4);
}

SHELL_SUBCMD_SET_CREATE(sub_section_cmd, (section_cmd));

static int cmd1_handler(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return 10;
}

/* Create a set of subcommands for "section_cmd cm1". */
SHELL_SUBCMD_SET_CREATE(sub_section_cmd1, (section_cmd, cmd1));

/* Add command to the set. Subcommand set is identify by parent shell command. */
SHELL_SUBCMD_ADD((section_cmd), cmd1, &sub_section_cmd1, "help for cmd1", cmd1_handler, 1, 0);

SHELL_CMD_REGISTER(section_cmd, &sub_section_cmd,
		   "Demo command using section for subcommand registration", NULL);

ZTEST(shell, test_section_cmd)
{
	test_shell_execute_cmd("section_cmd", SHELL_CMD_HELP_PRINTED);
	test_shell_execute_cmd("section_cmd cmd1", 10);
	test_shell_execute_cmd("section_cmd cmd2", 20);
	test_shell_execute_cmd("section_cmd cmd1 sub_cmd1", 11);
	test_shell_execute_cmd("section_cmd cmd1 sub_cmd2", -EINVAL);
}

static void *shell_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	/* Wait for the initialization of the shell dummy backend. */
	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "timed out waiting for dummy shell backend");

	return NULL;
}

ZTEST_SUITE(shell_1cpu, NULL, shell_setup, ztest_simple_1cpu_before,
			ztest_simple_1cpu_after, NULL);

ZTEST_SUITE(shell, NULL, shell_setup, NULL, NULL, NULL);
