/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Interactive shell test suite
 *
 */

#include <zephyr.h>
#include <ztest.h>

#include <shell/shell.h>
#include <shell/shell_dummy.h>

#define MAX_CMD_SYNTAX_LEN	(11)
static char dynamic_cmd_buffer[][MAX_CMD_SYNTAX_LEN] = {
		"dynamic",
		"command"
};

static void test_shell_execute_cmd(const char *cmd, int result)
{
	int ret;

	ret = shell_execute_cmd(NULL, cmd);

	TC_PRINT("shell_execute_cmd(%s): %d\n", cmd, ret);

	zassert_true(ret == result, "cmd: %s, got:%d, expected:%d",
							cmd, ret, result);
}

static void test_cmd_help(void)
{
	test_shell_execute_cmd("help", 0);
	test_shell_execute_cmd("help -h", 1);
	test_shell_execute_cmd("help --help", 1);
	test_shell_execute_cmd("help dummy", -EINVAL);
	test_shell_execute_cmd("help dummy dummy", -EINVAL);
}

static void test_cmd_clear(void)
{
	test_shell_execute_cmd("clear", 0);
	test_shell_execute_cmd("clear -h", 1);
	test_shell_execute_cmd("clear --help", 1);
	test_shell_execute_cmd("clear dummy", -EINVAL);
	test_shell_execute_cmd("clear dummy dummy", -EINVAL);
}

static void test_cmd_shell(void)
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

static void test_cmd_history(void)
{
	test_shell_execute_cmd("history", 0);
	test_shell_execute_cmd("history -h", 1);
	test_shell_execute_cmd("history --help", 1);
	test_shell_execute_cmd("history dummy", -EINVAL);
	test_shell_execute_cmd("history dummy dummy", -EINVAL);
}

static void test_cmd_resize(void)
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

static void test_shell_module(void)
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
static void test_shell_wildcards_static(void)
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
static void test_shell_wildcards_dynamic(void)
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

static void test_cmd_select(void)
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

static void test_set_root_cmd(void)
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

static void test_shell_fprintf(void)
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
#define CMD_NAME test_cmd_raw_arg

static int cmd_raw_arg(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 2) {
		if (strcmp(argv[0], STRINGIFY(CMD_NAME))) {
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

SHELL_CMD_ARG_REGISTER(CMD_NAME, NULL, NULL, cmd_raw_arg, 1, SHELL_OPT_ARG_RAW);

static void test_raw_arg(void)
{
	test_shell_execute_cmd("test_cmd_raw_arg aaa \"\" bbb", 0);
	test_shell_execute_cmd("test_cmd_raw_arg", 0);
	test_shell_execute_cmd("select test_cmd_raw_arg", 0);
	test_shell_execute_cmd("aaa \"\" bbb", 0);
}

void test_main(void)
{
	ztest_test_suite(shell_test_suite,
			ztest_1cpu_unit_test(test_cmd_help),
			ztest_unit_test(test_cmd_clear),
			ztest_unit_test(test_cmd_shell),
			ztest_unit_test(test_cmd_history),
			ztest_unit_test(test_cmd_select),
			ztest_unit_test(test_cmd_resize),
			ztest_unit_test(test_shell_module),
			ztest_unit_test(test_shell_wildcards_static),
			ztest_unit_test(test_shell_wildcards_dynamic),
			ztest_unit_test(test_shell_fprintf),
			ztest_unit_test(test_set_root_cmd),
			ztest_unit_test(test_raw_arg)
			);

	/* Let the shell backend initialize. */
	k_msleep(20);

	ztest_run_test_suite(shell_test_suite);
}
