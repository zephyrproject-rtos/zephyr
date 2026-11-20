/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 *
 * The "net stun" shell commands, run through the dummy shell backend against
 * the scripted server of server.c.
 */
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "server.h"

static const struct shell *sh;

/* What the last command printed. */
static const char *output;

/* Run a command in this thread; its output is kept, its return code returned. */
static int run(const char *cmd)
{
	size_t len = 0U;
	int rc = 0;

	shell_backend_dummy_clear_output(sh);
	rc = shell_execute_cmd(sh, cmd);
	output = shell_backend_dummy_get_output(sh, &len);
	return rc;
}

#define expect_output(text)                                                                        \
	zexpect_not_null(strstr(output, text), "no '%s' in the output: '%s'", text, output)

#define expect_no_output(text)                                                                     \
	zexpect_is_null(strstr(output, text), "'%s' in the output: '%s'", text, output)

/* Start the scripted server, wait for the dummy shell backend and check the
 * commands start with no server.
 */
static void *suite_setup(void)
{
	server_start();

	sh = shell_backend_dummy_get_ptr();
	WAIT_FOR(shell_ready(sh), 20 * USEC_PER_SEC, k_msleep(1));
	zassert_true(shell_ready(sh), "the dummy shell backend did not start");

#if defined(CONFIG_STUN_SHELL)
	/* Before any command has named a server, there is none. */
	zassert_ok(run("net stun server"));
	zassert_not_null(strstr(output, "No STUN server"), "a server is set: '%s'", output);
#endif
	return NULL;
}

#if defined(CONFIG_STUN_SHELL)

#define OTHER_PORT_STR STRINGIFY(OTHER_PORT)

/* The longest host name "net stun server" keeps. */
#define HOST_MAX_LEN 253

/* Is `text` in what the shell has printed so far? */
static bool printed(const char *text)
{
	struct shell_dummy *dummy = (struct shell_dummy *)sh->iface->ctx;

	dummy->buf[dummy->len] = '\0';
	return strstr(dummy->buf, text) != NULL;
}

/* Reset the scripted server and unset the server of the commands. */
static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	server_reset();
	zassert_ok(run("net stun server \"\""));
}

ZTEST_SUITE(stun_shell, NULL, suite_setup, before_each, NULL, NULL);

/* The address the server saw the last request come from, as the command
 * prints it.
 */
static const char *client_address(int family)
{
	static char text[64];

	snprintf(text, sizeof(text), (family == NET_AF_INET) ? "127.0.0.1:%u" : "[::1]:%u",
		 (unsigned int)atomic_get(&server.last_client_port));
	return text;
}

/* "net stun server" sets the host and the port, and "net stun query" asks there
 * and prints the address the server saw.
 */
ZTEST(stun_shell, test_server_then_query)
{
	/* A server with a port of its own. */
	zassert_ok(run("net stun server " HOST " " OTHER_PORT_STR));
	zassert_ok(run("net stun query"));
	zassert_equal(atomic_get(&server.requests), 1);
	zassert_equal(atomic_get(&server.last_port), OTHER_PORT);
	expect_output(client_address(FAMILY));

	/* And one on the default port. */
	zassert_ok(run("net stun server " HOST));
	zassert_ok(run("net stun query 2000"));
	zassert_equal(atomic_get(&server.requests), 2);
	zassert_equal(atomic_get(&server.last_port), STUN_CLIENT_DEFAULT_PORT);
	expect_output(client_address(FAMILY));

	/* The same over IPv6, where an address is printed in brackets. */
	if (IS_ENABLED(CONFIG_NET_IPV4) && IS_ENABLED(CONFIG_NET_IPV6)) {
		zassert_ok(run("net stun server ::1"));
		zassert_ok(run("net stun query"));
		zassert_equal(atomic_get(&server.requests), 3);
		expect_output(client_address(NET_AF_INET6));
	}
}

/* With no server set the query fails, names the command that sets one and sends
 * nothing.
 */
ZTEST(stun_shell, test_query_without_server)
{
	zassert_true(run("net stun query") != 0);
	expect_output("net stun server");
	zassert_equal(atomic_get(&server.requests), 0);
}

/* "net stun server" with no argument prints the server that is set, or that
 * there is none.
 */
ZTEST(stun_shell, test_server_shows_what_is_set)
{
	char default_port[sizeof("65535")] = "";

	snprintf(default_port, sizeof(default_port), "%u", (unsigned int)STUN_CLIENT_DEFAULT_PORT);

	zassert_ok(run("net stun server"));
	expect_output("No STUN server");

	zassert_ok(run("net stun server " HOST " " OTHER_PORT_STR));
	zassert_ok(run("net stun server"));
	expect_output(HOST);
	expect_output(OTHER_PORT_STR);

	/* Without a port it is the default one. */
	zassert_ok(run("net stun server " HOST));
	zassert_ok(run("net stun server"));
	expect_output(HOST);
	expect_output(default_port);
	expect_no_output(OTHER_PORT_STR);

	/* An empty name takes the server away again. */
	zassert_ok(run("net stun server \"\""));
	zassert_ok(run("net stun server"));
	expect_output("No STUN server");
}

/* A port that is not a number from 1 to 65535, or an extra argument, is refused
 * and the server stays as it was.
 */
ZTEST(stun_shell, test_server_refuses_a_bad_port)
{
	static const char *const bad[] = {
		"net stun server other.invalid 0",
		"net stun server other.invalid 65536",
		"net stun server other.invalid 70000",
		"net stun server other.invalid 4294967297",
		"net stun server other.invalid 99999999999999999999",
		"net stun server other.invalid -1",
		"net stun server other.invalid abc",
		"net stun server other.invalid 34x",
		"net stun server other.invalid \"\"",
		"net stun server other.invalid 3478 more",
	};

	zassert_ok(run("net stun server " HOST " " OTHER_PORT_STR));

	for (size_t i = 0; i < ARRAY_SIZE(bad); i++) {
		zexpect_true(run(bad[i]) != 0, "'%s' was accepted", bad[i]);

		/* And the server is what it was. */
		zexpect_ok(run("net stun server"));
		expect_output(HOST);
		expect_output(OTHER_PORT_STR);
		expect_no_output("other.invalid");
	}

	zassert_ok(run("net stun query"));
	zassert_equal(atomic_get(&server.last_port), OTHER_PORT);
}

/* A host name longer than the command keeps is refused and the server stays as
 * it was.
 */
ZTEST(stun_shell, test_server_refuses_a_name_too_long)
{
	/* One character more than the command keeps. */
	char cmd[sizeof("net stun server ") + HOST_MAX_LEN + 1] = "";
	size_t at = strlen("net stun server ");

	zassert_ok(run("net stun server " HOST " " OTHER_PORT_STR));

	memset(cmd, 0, sizeof(cmd));
	memcpy(cmd, "net stun server ", at);
	memset(cmd + at, 'a', HOST_MAX_LEN + 1);
	zassert_true(run(cmd) != 0);

	zassert_ok(run("net stun server"));
	expect_output(HOST);
	expect_output(OTHER_PORT_STR);
	expect_no_output("aaaa");

	/* The longest one it keeps is kept whole. */
	cmd[at + HOST_MAX_LEN] = '\0';
	zassert_ok(run(cmd));
	zassert_ok(run("net stun server"));
	expect_output(cmd + at);
}

/* A silent server: the query prints "No answer" after the time given. */
ZTEST(stun_shell, test_query_timeout)
{
	int64_t start = 0;
	int64_t took = 0;

	atomic_set(&server.mode, MODE_SILENT);
	zassert_ok(run("net stun server " HOST));

	start = k_uptime_get();
	zassert_true(run("net stun query 300") != 0);
	took = k_uptime_get() - start;

	expect_output("No answer");
	zassert_true(took >= 300 && took < 2000, "gave up after %lld ms", took);
	zassert_equal(atomic_get(&server.requests), 1);
}

/* A timeout that is not a number of milliseconds, or an extra argument, is
 * refused and nothing is sent.
 */
ZTEST(stun_shell, test_query_refuses_a_bad_timeout)
{
	/* clang-format off */
	static const char *const bad[] = {
		"net stun query abc",
		"net stun query -5",
		"net stun query 12x",
		"net stun query \"\"",
		"net stun query 4294967296",
		"net stun query 99999999999999999999",
		"net stun query 100 200",
	};
	/* clang-format on */

	zassert_ok(run("net stun server " HOST));

	for (size_t i = 0; i < ARRAY_SIZE(bad); i++) {
		zexpect_true(run(bad[i]) != 0, "'%s' was accepted", bad[i]);
	}
	zassert_equal(atomic_get(&server.requests), 0, "nothing was sent");
}

/* An error response and a name that does not resolve each fail the query with a
 * message of their own.
 */
ZTEST(stun_shell, test_query_reports_what_went_wrong)
{
	/* The server answers with an error. */
	atomic_set(&server.mode, MODE_ERROR_500);
	zassert_ok(run("net stun server " HOST));
	zassert_true(run("net stun query") != 0);
	expect_output("answered with an error");
	zassert_equal(atomic_get(&server.requests), 1);

	/* A name nothing resolves. */
	zassert_ok(run("net stun server no-such-host.invalid"));
	zassert_true(run("net stun query") != 0);
	expect_output("Cannot resolve no-such-host.invalid");
	zassert_equal(atomic_get(&server.requests), 1, "nothing more was sent");
}

/* The query typed into the shell runs in the shell thread and leaves part of
 * its stack unused.
 */
ZTEST(stun_shell, test_query_on_the_shell_thread)
{
	static const char cmd[] = "net stun query 2000\n";
	const char *expected = NULL;
	size_t unused = 0;
	size_t len = 0U;

	/* Type the command in for the shell thread to run, then read its unused stack. */
	zassert_ok(run("net stun server " HOST));
	shell_backend_dummy_clear_output(sh);
	zassert_ok(shell_backend_dummy_push_input(sh, cmd, strlen(cmd)));

	WAIT_FOR(atomic_get(&server.requests) == 1 && atomic_get(&server.last_client_port) != 0,
		 5 * USEC_PER_SEC, k_msleep(1));
	zassert_equal(atomic_get(&server.requests), 1, "the command did not run");
	expected = client_address(FAMILY);
	WAIT_FOR(printed(expected), 5 * USEC_PER_SEC, k_msleep(1));

	/* Wait until the shell thread has left the command. */
	WAIT_FOR(sh->ctx->ctx.flags.cmd_ctx == 0U, 5 * USEC_PER_SEC, k_msleep(1));
	zassert_true(sh->ctx->ctx.flags.cmd_ctx == 0U, "the command did not return");

	output = shell_backend_dummy_get_output(sh, &len);
	expect_output(expected);

	zassert_ok(k_thread_stack_space_get(sh->thread, &unused));
	TC_PRINT("shell thread: %u of %u bytes of stack never used\n", (unsigned int)unused,
		 (unsigned int)CONFIG_SHELL_STACK_SIZE);
	zassert_true(unused >= 256U, "%u bytes of the shell stack left", (unsigned int)unused);
}

SHELL_DUMMY_DEFINE(other_transport);
SHELL_DEFINE(other_shell, "other:~$ ", &other_transport, 256, 0, SHELL_FLAG_OLF_CRLF);

/* While a query runs in the shell thread, the commands of another shell are
 * refused and the server stays as it was.
 */
ZTEST(stun_shell, test_commands_refused_while_a_query_runs)
{
	static const struct shell_backend_config_flags flags = SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;
	static const char cmd[] = "net stun query 1000\n";
	static bool started;
	size_t len = 0U;

	if (!started) {
		zassert_ok(shell_init(&other_shell, NULL, flags, false, 0));
		WAIT_FOR(shell_ready(&other_shell), 5 * USEC_PER_SEC, k_msleep(1));
		started = true;
	}

	atomic_set(&server.mode, MODE_SILENT);
	zassert_ok(run("net stun server " HOST " " OTHER_PORT_STR));
	zassert_ok(shell_backend_dummy_push_input(sh, cmd, strlen(cmd)));

	WAIT_FOR(atomic_get(&server.requests) == 1, 5 * USEC_PER_SEC, k_msleep(1));
	zassert_equal(atomic_get(&server.requests), 1, "the query did not start");

	shell_backend_dummy_clear_output(&other_shell);
	zexpect_equal(shell_execute_cmd(&other_shell, "net stun server other.invalid"), -EBUSY);
	zexpect_equal(shell_execute_cmd(&other_shell, "net stun query"), -EBUSY);
	output = shell_backend_dummy_get_output(&other_shell, &len);
	expect_output("A STUN query is running");
	zexpect_equal(atomic_get(&server.requests), 1, "a second query was sent");

	WAIT_FOR(sh->ctx->ctx.flags.cmd_ctx == 0U, 5 * USEC_PER_SEC, k_msleep(1));
	zassert_true(sh->ctx->ctx.flags.cmd_ctx == 0U, "the query did not return");

	zassert_ok(shell_execute_cmd(&other_shell, "net stun server"));
	output = shell_backend_dummy_get_output(&other_shell, &len);
	expect_output(HOST);
	expect_output(OTHER_PORT_STR);
	expect_no_output("other.invalid");
}

#else /* CONFIG_STUN_SHELL */

ZTEST_SUITE(stun_shell, NULL, suite_setup, NULL, NULL, NULL);

/* Without CONFIG_STUN_SHELL both commands print the options that enable them. */
ZTEST(stun_shell, test_disabled_commands_say_how_to_enable_them)
{
	server_reset();

	zassert_ok(run("net stun server " HOST));
	expect_output("CONFIG_STUN_SHELL");

	zassert_ok(run("net stun query"));
	expect_output("CONFIG_STUN_SHELL");
	zassert_equal(atomic_get(&server.requests), 0);
}

#endif /* CONFIG_STUN_SHELL */
