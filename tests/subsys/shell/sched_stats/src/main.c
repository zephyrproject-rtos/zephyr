/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ztest for the CONFIG_SCHED_STATS_SHELL "sched" shell commands
 * (cpus / threads / top). The commands are driven through the dummy shell
 * backend so the suite runs on native_sim in CI. The cases deliberately cover
 * the behaviours that are easy to get wrong: that "sched top" terminates (it
 * runs on the shell thread and cannot be interrupted), that "sched threads -d"
 * has its own measurement window, and that the STK% column is only present when
 * the stack-usage information is actually available.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>

static const struct shell *sh;

static const char *run(const char *cmd, int *ret)
{
	size_t size = 0;

	shell_backend_dummy_clear_output(sh);
	*ret = shell_execute_cmd(sh, cmd);
	return shell_backend_dummy_get_output(sh, &size);
}

static void *setup(void)
{
	sh = shell_backend_dummy_get_ptr();
	zassert_not_null(sh, "no dummy shell backend");

	/* Give the shell thread time to initialise before injecting cmds. */
	WAIT_FOR(shell_ready(sh), 200000, k_msleep(10));
	zassert_true(shell_ready(sh), "timed out waiting for dummy shell backend");
	return NULL;
}

ZTEST(sched_stats, test_cpus)
{
	int ret;
	const char *out = run("sched cpus", &ret);

	TC_PRINT("--- sched cpus ---\n%s\n", out);
	zassert_ok(ret, "sched cpus returned %d", ret);
	zassert_not_null(strstr(out, "CPU"), "no CPU header");
	zassert_not_null(strstr(out, "busy"), "no busy column");
	zassert_not_null(strstr(out, "idle"), "no idle column");
}

ZTEST(sched_stats, test_cpus_delta_interval)
{
	int ret;
	const char *out = run("sched cpus -d -i 20 -n 2", &ret);

	TC_PRINT("--- sched cpus -d -i 20 -n 2 ---\n%s\n", out);
	zassert_ok(ret, "sched cpus -d returned %d", ret);
	zassert_not_null(strstr(out, "CPU"), "no CPU header");
	zassert_not_null(strstr(out, "sample 2"), "second sample missing");
}

ZTEST(sched_stats, test_threads)
{
	int ret;
	const char *out = run("sched threads", &ret);

	TC_PRINT("--- sched threads ---\n%s\n", out);
	zassert_ok(ret, "sched threads returned %d", ret);
	zassert_not_null(strstr(out, "NAME"), "no NAME header");
	zassert_not_null(strstr(out, "PRIO"), "no PRIO header");
	zassert_not_null(strstr(out, "CPU%"), "no CPU%% column");

	/* STK% must appear only when the stack-usage info is actually
	 * compiled in - otherwise the column is omitted rather than
	 * silently reporting 0.
	 */
	if (IS_ENABLED(CONFIG_INIT_STACKS) && IS_ENABLED(CONFIG_THREAD_STACK_INFO)) {
		zassert_not_null(strstr(out, "STK"), "STK column missing but available");
	} else {
		zassert_is_null(strstr(out, "STK"), "STK column present but unavailable");
	}
}

/*
 * "sched threads -d" must produce deltas from its own window, without relying
 * on "sched cpus" having run first. Establish a baseline with a plain
 * "sched threads", then a delta invocation must print the delta table.
 */
ZTEST(sched_stats, test_threads_delta_self_window)
{
	int ret;
	const char *out;

	(void)run("sched threads", &ret);
	zassert_ok(ret, "baseline sched threads returned %d", ret);

	out = run("sched threads -d", &ret);
	TC_PRINT("--- sched threads -d ---\n%s\n", out);
	zassert_ok(ret, "sched threads -d returned %d", ret);
	zassert_not_null(strstr(out, "dCPU"), "no dCPU delta header");
}

/* First-ever delta with no baseline should record one and say so, not divide
 * by a bogus window.
 */
ZTEST(sched_stats, test_threads_delta_interval)
{
	int ret;
	const char *out = run("sched threads -d -i 20 -n 3", &ret);

	TC_PRINT("--- sched threads -d -i 20 -n 3 ---\n%s\n", out);
	zassert_ok(ret, "sched threads -d -i returned %d", ret);
	zassert_not_null(strstr(out, "dCPU"), "no dCPU delta header");
}

/*
 * "sched top" runs on the shell thread and cannot be interrupted, so it must
 * terminate on its own. With the old "run until interrupted" behaviour this
 * command would never return and the test would hang until the CI timeout.
 * Use the default (unbounded-looking) invocation with only a fast interval.
 */
ZTEST(sched_stats, test_top_terminates)
{
	int ret;
	/* Default repeat count (10). Reaching the final "10/10" banner proves
	 * the command ran the whole bounded sequence and returned instead of
	 * looping forever. (Only the tail of the output is asserted because the
	 * dummy backend ring buffer keeps the most recent bytes.)
	 */
	const char *out = run("sched top -i 5", &ret);

	TC_PRINT("--- sched top -i 5 (tail) ---\n%s\n", out);
	zassert_ok(ret, "sched top returned %d", ret);
	zassert_not_null(strstr(out, "10/10"), "did not reach the bounded default count");
}

ZTEST(sched_stats, test_top_bounded_count)
{
	int ret;
	const char *out = run("sched top -i 5 -n 2", &ret);

	TC_PRINT("--- sched top -i 5 -n 2 ---\n%s\n", out);
	zassert_ok(ret, "sched top -n 2 returned %d", ret);
	zassert_not_null(strstr(out, "2/2"), "did not stop at requested count");
}

ZTEST(sched_stats, test_invalid_options)
{
	int ret;

	(void)run("sched cpus -x", &ret);
	zassert_not_equal(ret, 0, "unknown option accepted");

	(void)run("sched cpus -c 99999", &ret);
	zassert_not_equal(ret, 0, "out-of-range CPU index accepted");

	(void)run("sched cpus -n 0", &ret);
	zassert_not_equal(ret, 0, "zero repeat count accepted");
}

ZTEST_SUITE(sched_stats, NULL, setup, NULL, NULL, NULL);
