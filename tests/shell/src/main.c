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

static void test_shell_exec(const char *line, int result)
{
	char cmd[80];
	int ret;

	strncpy(cmd, line, sizeof(cmd) - 1);
	cmd[79] = '\0';

	ret = shell_exec(cmd);

	TC_PRINT("shell_exec(%s): %d\n", line, ret);

	zassert_true(ret == result, line);
}

static void test_help(void)
{
	test_shell_exec("help", 0);
	test_shell_exec("help dummy", 0);
	test_shell_exec("help invalid", -EINVAL);
}

static void test_select(void)
{
	test_shell_exec("select", 0);
	test_shell_exec("select dummy", 0);
	test_shell_exec("select invalid", -EINVAL);
}

static void test_exit(void)
{
	test_shell_exec("exit", 0);
}

static void test_module(void)
{
	test_shell_exec("dummy cmd1", 0);
	test_shell_exec("dummy cmd1 arg1", -EINVAL);

	test_shell_exec("dummy cmd2 arg1", 0);
	test_shell_exec("dummy cmd2 arg1 arg2", -EINVAL);

	test_shell_exec("dummy cmd3 arg1 arg2", 0);
	test_shell_exec("dummy cmd3 arg1 arg2 arg3", -EINVAL);

	test_shell_exec("dummy cmd4 arg1 arg2 arg3", -EINVAL);

	shell_register_default_module("dummy");

	test_shell_exec("cmd1", 0);
	test_shell_exec("cmd1 arg1", -EINVAL);

	test_shell_exec("cmd2 arg1", 0);
	test_shell_exec("cmd2 arg1 arg2", -EINVAL);

	test_shell_exec("cmd3 arg1 arg2", 0);
	test_shell_exec("cmd3 arg1 arg2 arg3", -EINVAL);

	test_shell_exec("cmd4 arg1 arg2 arg3", -EINVAL);
}

static int app_cmd_handler(int argc, char *argv[])
{
	return 0;
}

static void test_app_handler(void)
{
	shell_register_app_cmd_handler(app_cmd_handler);

	test_shell_exec("cmd4 arg1 arg2 arg3", 0);
}

static int dummy_cmd(int argc, char *argv[])
{
	if (!strcmp(argv[0], "cmd1")) {
		return argc == 1 ? 0 : -EINVAL;
	}

	if (!strcmp(argv[0], "cmd2")) {
		return argc == 2 ? 0 : -EINVAL;
	}

	if (!strcmp(argv[0], "cmd3")) {
		return argc == 3 ? 0 : -EINVAL;
	}

	return -EINVAL;
}

static const struct shell_cmd dummy_cmds[] = {
	{ "cmd1", dummy_cmd, "" },
	{ "cmd2", dummy_cmd, "<arg1>" },
	{ "cmd3", dummy_cmd, "<arg1> <arg2>" },
	{ NULL, NULL }
};

void test_main(void)
{
	SHELL_REGISTER("dummy", dummy_cmds);

	ztest_test_suite(shell_test_suite,
			ztest_unit_test(test_help),
			ztest_unit_test(test_select),
			ztest_unit_test(test_exit),
			ztest_unit_test(test_module),
			ztest_unit_test(test_app_handler));
	ztest_run_test_suite(shell_test_suite);
}
