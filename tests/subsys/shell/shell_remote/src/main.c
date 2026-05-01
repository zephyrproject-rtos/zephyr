/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/tc_util.h"
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/shell/shell_remote.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Wait for command completion */
#define SLEEP_TIME 50

static char *remote_names[8];
static char *remote_name;
static size_t remote_names_count;

/* Parse shell response to tab suggestion to get remote names */
static void get_remote_names(void)
{
	if (!IS_ENABLED(CONFIG_SHELL_REMOTE_MULTI_CLI)) {
		remote_names[remote_names_count] = "";
		remote_names_count++;
		TC_PRINT("Single remote client\n");
		return;
	}

	STRUCT_SECTION_FOREACH(shell_remote, remote) {
		size_t remote_name_len = strlen(remote->name);

		remote_names[remote_names_count] = (char *)malloc(remote_name_len + 2);
		remote_names[remote_names_count][0] = ' ';
		memcpy((void *)&remote_names[remote_names_count][1], remote->name, remote_name_len);
		remote_names[remote_names_count][remote_name_len + 1] = '\0';
		remote_names_count++;
		zassert_true(remote_names_count < ARRAY_SIZE(remote_names),
			     "Remote names count is too large: %d", remote_names_count);
	}

	if (remote_names_count == 0) {
		zassert_true(remote_names_count > 0, "At least one remote name should be found");
	}

	TC_PRINT("remote names:");
	for (int i = 0; i < remote_names_count; i++) {
		TC_PRINT(" %s", remote_names[i]);
	}
	TC_PRINT("\n");
}

static int cmd_ping(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "pong");
	return 0;
}

SHELL_CMD_REGISTER(ping, NULL, "Basic remote shell command", cmd_ping);

static void test_shell_execute_cmd(const char *cmd, int result, const char *expected,
				   size_t expected_len)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int ret;
	size_t len;

	shell_backend_dummy_clear_output(sh);

	ret = shell_execute_cmd(sh, cmd);

	TC_PRINT("shell command executed: \"%s\" returned: %d\n", cmd, ret);
	zassert_true(ret == result, "cmd: %s, got:%d, expected:%d", cmd, ret, result);
	k_msleep(SLEEP_TIME);

	const char *str = shell_backend_dummy_get_output(sh, &len);

	zassert_equal(len, expected_len, "len: %d, expected_len:%d response:%s", len, expected_len,
		      str);
	zassert_mem_equal(str, expected, expected_len, "cmd: %s, got:%s, expected:%s", cmd, str,
			  expected);
	shell_backend_dummy_clear_output(sh);
}

ZTEST(shell_remote, test_local_cmd)
{
#define PING_RESP "\r\npong\r\n"

	const struct shell *sh = shell_backend_dummy_get_ptr();

	zassert_not_null(sh, "dummy shell backend is not available");
	zassert_true(shell_ready(sh), "timed out waiting for dummy shell backend");

	test_shell_execute_cmd("ping", 0, PING_RESP, sizeof(PING_RESP) - 1);
}

ZTEST(shell_remote, test_remote_ping)
{
#define PING_RESP    "\r\npong\r\n"
#define PING_COMMAND "remote_shell%s ping"
	char command[sizeof(PING_COMMAND) + 64];
	int rv;

	rv = snprintf(command, sizeof(command), PING_COMMAND, remote_name);
	zassert_true(rv > 0);

	test_shell_execute_cmd(command, 0, PING_RESP, sizeof(PING_RESP) - 1);
}

ZTEST(shell_remote, test_local_tab)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int rv;
	size_t len;

	shell_backend_dummy_clear_output(sh);
	shell_backend_dummy_clear_input(sh);
	rv = shell_backend_dummy_push_input(sh, "\t", 1);
	k_msleep(SLEEP_TIME);

	const char *str = shell_backend_dummy_get_output(sh, &len);

	/* Expect to find the remote shell command. */
	zassert_not_null(strstr(str, "remote_shell"));

	(void)shell_backend_dummy_push_input(sh, "\r\n", 2);
	k_msleep(SLEEP_TIME);

	shell_backend_dummy_clear_output(sh);
}

static void clear_shell(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	const char ctrl_c = 0x03;

	shell_backend_dummy_clear_input(sh);
	zassert_true(IS_ENABLED(CONFIG_SHELL_METAKEYS), "shell metakeys are not enabled");
	(void)shell_backend_dummy_push_input(sh, &ctrl_c, 1);
	k_msleep(SLEEP_TIME);
	shell_backend_dummy_clear_output(sh);
}

ZTEST(shell_remote, test_remote_tab)
{
#undef TAB_COMMAND
#define TAB_COMMAND "remote_shell%s \t"
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int rv;
	size_t len;
	char command[sizeof(TAB_COMMAND) + 64];

	rv = snprintf(command, sizeof(command), TAB_COMMAND, remote_name);
	zassert_true(rv > 0);

	shell_backend_dummy_clear_output(sh);

	shell_backend_dummy_push_input(sh, command, strlen(command));
	k_msleep(SLEEP_TIME);

	const char *str = shell_backend_dummy_get_output(sh, &len);

	/* Expect to find the remote shell command. */
	zassert_not_null(strstr(str, "ping"));
}

ZTEST(shell_remote, test_remote_cmd_tab)
{
#undef TAB_COMMAND
#define TAB_COMMAND "remote_shell%s static_cmd \t"
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int rv;
	size_t len;
	char command[sizeof(TAB_COMMAND) + 64];

	rv = snprintf(command, sizeof(command), TAB_COMMAND, remote_name);
	zassert_true(rv > 0);

	shell_backend_dummy_clear_output(sh);

	shell_backend_dummy_push_input(sh, command, strlen(command));
	k_msleep(SLEEP_TIME);

	const char *str = shell_backend_dummy_get_output(sh, &len);

	/* Expect to find the remote shell command. */
	zassert_not_null(strstr(str, "cmd1"), "Failed for remote: %s", remote_name);
	zassert_not_null(strstr(str, "cmd2"), "Failed for remote: %s", remote_name);
	zassert_not_null(strstr(str, "cmd3"), "Failed for remote: %s", remote_name);
	zassert_not_null(strstr(str, "too_long_string_cmd"), "Failed for remote: %s", remote_name);
}

ZTEST(shell_remote, test_remote_too_long_print_message)
{
#define TOO_LONG_COMMAND "remote_shell%s static_cmd too_long_string_cmd"
	const struct shell *sh = shell_backend_dummy_get_ptr();
	size_t len;
	int rv;
	const char *str;
	char command[sizeof(TOO_LONG_COMMAND) + 64];

	rv = snprintf(command, sizeof(command), TOO_LONG_COMMAND, remote_name);
	zassert_true(rv > 0);

	shell_backend_dummy_clear_output(sh);

	shell_backend_dummy_push_input(sh, command, strlen(command));
	k_msleep(SLEEP_TIME);

	rv = shell_execute_cmd(sh, command);
	zassert_equal(rv, 0);

	str = shell_backend_dummy_get_output(sh, &len);
	zassert_not_null(strstr(str, "Buffer size (128) too small"));
}

ZTEST(shell_remote, test_remote_static_cmd3_error_warn)
{
#define TEST_COMMAND "remote_shell%s static_cmd cmd3"
	const struct shell *sh = shell_backend_dummy_get_ptr();
	char command[sizeof(TEST_COMMAND) + 64];
	size_t len;
	int rv;

	rv = snprintf(command, sizeof(command), TEST_COMMAND, remote_name);
	zassert_true(rv > 0);

	shell_backend_dummy_clear_output(sh);
	rv = shell_execute_cmd(sh, command);
	zassert_equal(rv, 0, "cmd3 shell result");

	const char *str = shell_backend_dummy_get_output(sh, &len);

	zassert_not_null(strstr(str, "remote_static_cmd3_err"), "error output missing: %s", str);
	zassert_not_null(strstr(str, "remote_static_cmd3_warn"), "warning output missing: %s", str);
}

ZTEST(shell_remote, test_remote_dyn_cmd_tab)
{
#undef TAB_COMMAND
#define TAB_COMMAND "remote_shell%s dyn_cmd \t"
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int rv;
	size_t len;
	char command[sizeof(TAB_COMMAND) + 64];

	rv = snprintf(command, sizeof(command), TAB_COMMAND, remote_name);
	zassert_true(rv > 0);

	shell_backend_dummy_clear_output(sh);

	shell_backend_dummy_push_input(sh, command, strlen(command));
	k_msleep(SLEEP_TIME);

	const char *str = shell_backend_dummy_get_output(sh, &len);

	zassert_not_null(strstr(str, "sub_cmd_0"),
			 "completion missing sub_cmd_0: %s for remote: %s", str, remote_name);
	zassert_not_null(strstr(str, "sub_cmd_1"),
			 "completion missing sub_cmd_1: %s for remote: %s", str, remote_name);
	zassert_not_null(strstr(str, "sub_cmd_2"),
			 "completion missing sub_cmd_2: %s for remote: %s", str, remote_name);
}

ZTEST(shell_remote, test_remote_execute_cmd2)
{
#define CMD2_COMMAND "remote_shell%s static_cmd cmd2 argument1"
#define CMD2_RESP    "\r\ncmd2 executed with arg: argument1\r\n"
	char command[sizeof(CMD2_COMMAND) + 64];
	int rv;

	rv = snprintf(command, sizeof(command), CMD2_COMMAND, remote_name);
	zassert_true(rv > 0);

	test_shell_execute_cmd(command, 0, CMD2_RESP, sizeof(CMD2_RESP) - 1);
}

ZTEST(shell_remote, test_remote_dynamic_subcmd)
{
#define DYN_RESP    "\r\nsub_cmd_1\r\n"
#define DYN_COMMAND "remote_shell%s dyn_cmd sub_cmd_1"
	char command[sizeof(DYN_COMMAND) + 64];
	int rv;

	rv = snprintf(command, sizeof(command), DYN_COMMAND, remote_name);
	zassert_true(rv > 0);

	test_shell_execute_cmd(command, 0, DYN_RESP, sizeof(DYN_RESP) - 1);
}

ZTEST(shell_remote, test_remote_print_uint64_t)
{
#define CMD1_COMMAND "remote_shell%s static_cmd cmd1"
#define CMD1_RESP    "\r\ncmd1 executed 12345678901234 1234567890\r\n"
	char command[sizeof(CMD1_COMMAND) + 64];
	int rv;

	rv = snprintf(command, sizeof(command), CMD1_COMMAND, remote_name);
	zassert_true(rv > 0);

	test_shell_execute_cmd(command, 0, CMD1_RESP, sizeof(CMD1_RESP) - 1);
}

static void *remote_shell_setup(void)
{
	static int cli;

	remote_name = remote_names[cli];
	TC_PRINT("Running suite with \"%s\" remote client\n", remote_name);
	cli++;
	return NULL;
}

static void setup(void *arg)
{
	ARG_UNUSED(arg);
	clear_shell();
}

ZTEST_SUITE(shell_remote, NULL, remote_shell_setup, setup, NULL, NULL);

void test_main(void)
{
	get_remote_names();
	ztest_run_all(NULL, false, remote_names_count, 1);
	ztest_verify_all_test_suites_ran();
}
