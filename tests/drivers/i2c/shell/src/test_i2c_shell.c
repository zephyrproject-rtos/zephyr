/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <string.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/fff.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>


ZTEST(i2c_shell, test_i2c_shell_scan_by_nodename)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	/* Scan bus 0 using the node name */
	err = shell_execute_cmd(sh, "i2c scan i2c@100");
	if (IS_ENABLED(CONFIG_I2C_SHELL_CONTROLLER_LIST)) {
		zassert_not_equal(err, 0, "i2c scan by nodenme succeeded");
	} else {
		zassert_ok(err, "i2c scan by nodename failed");
	}

	/* Scan bus 1 using the node name */
	err = shell_execute_cmd(sh, "i2c scan i2c@200");
	if (IS_ENABLED(CONFIG_I2C_SHELL_CONTROLLER_LIST)) {
		zassert_not_equal(err, 0, "i2c scan by nodenme succeeded");
	} else {
		zassert_ok(err, "i2c scan by nodename failed");
	}
}

ZTEST(i2c_shell, test_i2c_shell_scan_by_friendly_name)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	/* Scan bus 0 using the friendly name */
	err = shell_execute_cmd(sh, "i2c scan I2C_BUS_PRIMARY");
	if (IS_ENABLED(CONFIG_I2C_SHELL_CONTROLLER_LIST)) {
		zassert_ok(err, "i2c scan by friendly name failed");
	} else {
		zassert_not_equal(err, 0, "i2c scan by friendly name succeeded");
	}

	err = shell_execute_cmd(sh, "i2c scan I2C_BUS_SECONDARY");
	zassert_not_equal(err, 0, "i2c scan of invalid friendly name succeeded");
}

static void *i2c_shell_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	/* Wait for the initialization of the shell dummy backend. */
	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "timed out waiting for dummy shell backend");
	return NULL;
}

ZTEST_SUITE(i2c_shell, NULL, i2c_shell_setup, NULL, NULL, NULL);
