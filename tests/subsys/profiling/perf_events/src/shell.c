/*
 * SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its
 * SPDX-FileCopyrightText: affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

static void execute_command(const char *command, int expected_ret, const char *expected_output)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	const char *output;
	size_t output_size;
	int ret;

	shell_backend_dummy_clear_output(sh);
	ret = shell_execute_cmd(sh, command);
	zassert_equal(ret, expected_ret, "%s returned %d, expected %d", command, ret,
		      expected_ret);
	output = shell_backend_dummy_get_output(sh, &output_size);
	if (expected_output != NULL) {
		zassert_not_null(strstr(output, expected_output), "%s: missing '%s' in '%s'",
				 command, expected_output, output);
	}
}

static void *shell_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "Timed out waiting for dummy shell backend");
	return NULL;
}

static void shell_after(void *fixture)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	ARG_UNUSED(fixture);
	/* Stop any session left active by a failed assertion. */
	(void)shell_execute_cmd(sh, "perf stat stop");
	execute_command("perf clear", 0, "Perf buffer cleared");
}

ZTEST(perf_events_shell, test_discovery)
{
	execute_command("perf list", 0, "cpu0 : CPU performance counter provider");
	execute_command("perf list cpu0", 0, "cpu0.cycles : CPU cycle counter");
	execute_command("perf list missing", -ENOENT, "Provider not found: missing");
}

ZTEST(perf_events_shell, test_invalid_arguments)
{
	execute_command("perf stat stop", -EINVAL, NULL);
	execute_command("perf stat start -x cpu0.cycles", -EINVAL, NULL);
	execute_command("perf stat start -e cpu0.missing", -ENOENT,
			"Event not found: cpu0.missing");
	execute_command("perf stat start -e cpu0.cycles -e cpu0.cycles", -EINVAL, NULL);
	execute_command("perf info", 0, "Perf buffer: empty");
	execute_command("perf stat start -e cpu0.cycles", 0, "Perf stat started");
	execute_command("perf stat stop", 0, "cpu0.cycles");
}

ZTEST(perf_events_shell, test_result_retention)
{
	execute_command("perf stat start -e cpu0.cycles", 0, "Perf stat started");
	execute_command("perf stat stop", 0, "Perf buffer type: stat, version: 1");
	execute_command("perf info", 0, "Perf buffer: stat");
	execute_command("perf printbuf", 0, "cpu0.cycles");
	execute_command("perf printbuf", 0, "Perf buffer empty");

	execute_command("perf stat start -e cpu0.cycles", 0, "Perf stat started");
	execute_command("perf stat stop", 0, "cpu0.cycles");
	execute_command("perf clear", 0, "Perf buffer cleared");
	execute_command("perf printbuf", 0, "Perf buffer empty");
}

ZTEST(perf_events_shell, test_active_session)
{
	execute_command("perf stat start -e cpu0.cycles", 0, "Perf stat started");
	execute_command("perf info", 0, "Perf is running");
	execute_command("perf stat start -e cpu0.cycles", -EBUSY, "Perf is running");
	execute_command("perf printbuf", -EINPROGRESS, "Perf is running");
	execute_command("perf clear", -EINPROGRESS, "Perf is running");
	execute_command("perf stat stop", 0, "cpu0.cycles");
	execute_command("perf info", 0, "Perf buffer: stat");
	execute_command("perf stat start -e cpu0.cycles", 0, "Perf stat started");
	execute_command("perf info", 0, "Perf buffer: empty");
	execute_command("perf stat stop", 0, "cpu0.cycles");
}

ZTEST_SUITE(perf_events_shell, NULL, shell_setup, NULL, shell_after, NULL);
