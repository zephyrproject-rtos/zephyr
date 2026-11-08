/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

#include "fake_meter.h"

static const char *run(const char *cmd, int expected)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	size_t size;

	shell_backend_dummy_clear_output(sh);
	zassert_equal(shell_execute_cmd(sh, cmd), expected, "%s", cmd);

	return shell_backend_dummy_get_output(sh, &size);
}

static void assert_contains(const char *out, const char *text)
{
	zassert_not_null(strstr(out, text), "\"%s\" not found in:\n%s", text, out);
}

ZTEST(energy_meter_shell, test_info)
{
	const char *out = run("energy_meter info fake_meter", 0);

	assert_contains(out, "phases: 3");
	assert_contains(out, "voltage current active_power power_factor");
	assert_contains(out, "frequency");
}

ZTEST(energy_meter_shell, test_get_prints_table)
{
	const char *out = run("energy_meter get fake_meter", 0);

	/* Header, per-phase values, unsupported cells and negative values */
	assert_contains(out, "total");
	assert_contains(out, "voltage [V]");
	assert_contains(out, "231.000");
	assert_contains(out, "-1150.000");
	assert_contains(out, "50.010");
	assert_contains(out, "-");
}

ZTEST(energy_meter_shell, test_watch)
{
	const char *out = run("energy_meter watch fake_meter 1 2", 0);
	const char *second = strstr(out, "frequency");

	zassert_not_null(second);
	zassert_not_null(strstr(second + 1, "frequency"), "expected two tables");

	run("energy_meter watch fake_meter 0", -EINVAL);
	run("energy_meter watch fake_meter 2147483648", -EINVAL);
}

ZTEST(energy_meter_shell, test_get_failure)
{
	assert_contains(run("energy_meter get fake_meter_faulty", -EIO), "Failed to read");
}

ZTEST(energy_meter_shell, test_read)
{
	assert_contains(run("energy_meter read fake_meter voltage b", 0), "231.000 V");
	assert_contains(run("energy_meter read fake_meter frequency", 0), "50.010 Hz");
	assert_contains(run("energy_meter read fake_meter 16", 0), "4660 (0x1234)");

	run("energy_meter read fake_meter voltage x", -EINVAL);
	run("energy_meter read fake_meter bogus", -EINVAL);
	run("energy_meter read fake_meter temperature", -ENOTSUP);
}

ZTEST(energy_meter_shell, test_reset_energy)
{
	fake_meter_add_energy(2000000);
	run("energy_meter reset_energy fake_meter", 0);
	assert_contains(run("energy_meter read fake_meter active_energy_import", 0), "0.000 Wh");

	run("energy_meter reset_energy fake_meter_minimal", -ENOSYS);
}

ZTEST(energy_meter_shell, test_rejects_other_devices)
{
	run("energy_meter get nonexistent", -ENODEV);
}

static void *energy_meter_shell_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "dummy shell backend not ready");

	return NULL;
}

ZTEST_SUITE(energy_meter_shell, NULL, energy_meter_shell_setup, NULL, NULL, NULL);
