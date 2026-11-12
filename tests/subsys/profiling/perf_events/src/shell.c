/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

#include "internal.h"

static const char *execute_command_with_timeout(const char *command, int expected_ret,
					       const char *expected_output, k_timeout_t timeout)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	k_timepoint_t end = sys_timepoint_calc(timeout);
	const char *output;
	size_t output_size;
	int ret;

	for (;;) {
		shell_backend_dummy_clear_output(sh);
		ret = shell_execute_cmd(sh, command);
		if (ret != -EINPROGRESS || sys_timepoint_expired(end)) {
			break;
		}
		/* The session can be released before the recording work returns. */
		k_msleep(1);
	}
	zassert_equal(ret, expected_ret, "%s returned %d, expected %d", command, ret, expected_ret);
	output = shell_backend_dummy_get_output(sh, &output_size);
	if (expected_output != NULL) {
		zassert_not_null(strstr(output, expected_output), "%s: missing '%s' in '%s'",
				 command, expected_output, output);
	}
	return output;
}

static const char *execute_command(const char *command, int expected_ret,
				   const char *expected_output)
{
	return execute_command_with_timeout(command, expected_ret, expected_output, K_NO_WAIT);
}

static void check_stat_result(const char *command)
{
	const char *header = "Perf buffer type: stat, version: 1";
	const char *expected_name = "cpu0.CPU_CYCLES";
	const char *output = execute_command(command, 0, header);
	const char *result = strstr(output, header) + strlen(header);
	const char *name;
	char *end;

	errno = 0;
	(void)strtoull(result, &end, 10);
	zassert_true(end != result && errno == 0, "Invalid stat result: %s", output);
	name = end + strspn(end, " \t\r\n");
	zassert_equal(strcspn(name, " \t\r\n"), strlen(expected_name));
	zassert_mem_equal(name, expected_name, strlen(expected_name));
}

static void check_info(bool retained_result)
{
	if (retained_result) {
		execute_command("perf info", 0, "Perf buffer: stat");
	} else if (IS_ENABLED(CONFIG_PROFILING_PERF)) {
		execute_command("perf info", 0, "Perf buf: 0/");
	} else {
		execute_command("perf info", 0, "Perf buffer: empty");
	}
}

static void check_empty_buffer(void)
{
	const char *expected = IS_ENABLED(CONFIG_PROFILING_PERF) ?
		"Perf buf length 0" : "Perf buffer empty";

	execute_command("perf printbuf", 0, expected);
}

static void wait_for_session_stop(void)
{
	zassert_true(WAIT_FOR(!z_perf_session_is_active(), 2 * USEC_PER_SEC, k_msleep(1)),
		     "Perf session did not stop");
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
	/* Stop stat sessions and let real recordings finish after a failed assertion. */
	(void)shell_execute_cmd(sh, "perf stat stop");
	if (IS_ENABLED(CONFIG_PROFILING_PERF)) {
		wait_for_session_stop();
	} else {
		z_perf_session_release(PERF_SESSION_RECORD);
	}
	execute_command_with_timeout("perf clear", 0, "Perf buffer cleared", K_SECONDS(2));
}

ZTEST(perf_events_shell, test_discovery)
{
	execute_command("perf list", 0, "cpu0 : CPU performance counters");
	execute_command("perf list cpu0", 0, "cpu0.CPU_CYCLES : CPU_CYCLES");
	execute_command("perf list missing", -ENOENT, "Provider not found: missing");
}

ZTEST(perf_events_shell, test_invalid_arguments)
{
	execute_command("perf stat stop", -EINVAL, NULL);
	execute_command("perf stat start -x cpu0.CPU_CYCLES", -EINVAL, NULL);
	execute_command("perf stat start -e cpu0.missing", -ENOENT,
			"Event not found: cpu0.missing");
	execute_command("perf stat start -e cpu0.CPU_CYCLES -e cpu0.CPU_CYCLES", -EINVAL, NULL);
	check_info(false);
	execute_command("perf stat start -e cpu0.CPU_CYCLES", 0, "Perf stat started");
	execute_command("perf stat stop", 0, "cpu0.CPU_CYCLES");
}

ZTEST(perf_events_shell, test_result_retention)
{
	execute_command("perf stat start -e cpu0.CPU_CYCLES", 0, "Perf stat started");
	check_stat_result("perf stat stop");
	check_info(true);
	check_stat_result("perf printbuf");
	check_info(false);
	check_empty_buffer();

	execute_command("perf stat start -e cpu0.CPU_CYCLES", 0, "Perf stat started");
	execute_command("perf stat stop", 0, "cpu0.CPU_CYCLES");
	execute_command("perf clear", 0, "Perf buffer cleared");
	check_info(false);
	check_empty_buffer();
}

ZTEST(perf_events_shell, test_active_session)
{
	execute_command("perf stat start -e cpu0.CPU_CYCLES", 0, "Perf stat started");
	execute_command("perf info", 0, "Perf is running");
	execute_command("perf stat start -e cpu0.CPU_CYCLES", -EBUSY, "Perf is running");
	execute_command("perf printbuf", -EINPROGRESS, "Perf is running");
	execute_command("perf clear", -EINPROGRESS, "Perf is running");
	execute_command("perf stat stop", 0, "cpu0.CPU_CYCLES");
	check_info(true);
	execute_command("perf stat start -e cpu0.CPU_CYCLES", 0, "Perf stat started");
	check_info(false);
	execute_command("perf stat stop", 0, "cpu0.CPU_CYCLES");
}

ZTEST(perf_events_shell, test_record_excludes_stat)
{
	if (IS_ENABLED(CONFIG_PROFILING_PERF)) {
		execute_command("perf record 200 10", 0, "Enabled perf");
	} else {
		zassert_ok(z_perf_session_claim(PERF_SESSION_RECORD));
	}
	execute_command("perf stat start -e cpu0.CPU_CYCLES", -EBUSY, "Perf is running");
	if (IS_ENABLED(CONFIG_PROFILING_PERF)) {
		wait_for_session_stop();
	} else {
		z_perf_session_release(PERF_SESSION_RECORD);
	}
	execute_command("perf stat start -e cpu0.CPU_CYCLES", 0, "Perf stat started");
	execute_command("perf stat stop", 0, "cpu0.CPU_CYCLES");
}

#if defined(CONFIG_PROFILING_PERF)
ZTEST(perf_events_shell, test_stat_excludes_record)
{
	execute_command("perf stat start -e cpu0.CPU_CYCLES", 0, "Perf stat started");
	execute_command("perf record 200 10", -EINPROGRESS, "Perf is running");
	check_stat_result("perf stat stop");
}

ZTEST(perf_events_shell, test_record_replaces_stat_result)
{
	const char *header = "Perf buf length ";
	const char *output;
	const char *result;
	char *end;
	uint64_t length;

	execute_command("perf stat start -e cpu0.CPU_CYCLES", 0, "Perf stat started");
	check_stat_result("perf stat stop");
	execute_command("perf record 200 10", 0, "Enabled perf");
	zassert_false(z_perf_stat_shell_has_result(), "Recording retained the old stat result");
	wait_for_session_stop();
	output = execute_command("perf info", 0, "Perf buf: ");
	zassert_is_null(strstr(output, "Perf buffer: stat"));
	output = execute_command_with_timeout("perf printbuf", 0, header, K_SECONDS(2));
	zassert_is_null(strstr(output, "Perf buffer type: stat"));
	result = strstr(output, header) + strlen(header);
	errno = 0;
	length = strtoull(result, &end, 10);
	zassert_true(end != result && errno == 0, "Invalid recording result: %s", output);
	zassert_true(length > 0U, "Recording buffer is empty");
	check_empty_buffer();
}
#endif

ZTEST_SUITE(perf_events_shell, NULL, shell_setup, NULL, shell_after, NULL);
