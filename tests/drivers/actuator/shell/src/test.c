/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/actuator.h>
#include <zephyr/drivers/actuator/fake.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

DEFINE_FFF_GLOBALS;

#define FAKE_ACTUATOR_NODE DT_NODELABEL(actuator_fake)
#define FAKE_ACTUATOR_NAME DEVICE_DT_NAME(FAKE_ACTUATOR_NODE)

static const struct shell *test_sh;
static const struct device *test_dev = DEVICE_DT_GET(FAKE_ACTUATOR_NODE);

static int test_set_setpoint_ok(const struct device *dev, q31_t setpoint)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(setpoint);

	return 0;
}

static int test_set_setpoint_eio(const struct device *dev, q31_t setpoint)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(setpoint);

	return -EIO;
}

static void *test_setup(void)
{
	test_sh = shell_backend_dummy_get_ptr();
	WAIT_FOR(shell_ready(test_sh), 20000, k_msleep(1));
	zassert_true(shell_ready(test_sh), "timed out waiting for dummy shell backend");
	return NULL;
}

ZTEST_SUITE(actuator_shell, NULL, test_setup, NULL, NULL, NULL);

ZTEST(actuator_shell, test_set_setpoint)
{
	int ret;
	const char *out;
	size_t out_size;

	actuator_fake_set_setpoint_fake.custom_fake = test_set_setpoint_ok;

	ret = shell_execute_cmd(test_sh, "actuator set_setpoint " FAKE_ACTUATOR_NAME " 0");
	zassert_ok(ret);
	zassert_equal(actuator_fake_set_setpoint_fake.call_count, 1);
	zassert_equal(actuator_fake_set_setpoint_fake.arg0_val, test_dev);
	zassert_equal(actuator_fake_set_setpoint_fake.arg1_val, 0);

	ret = shell_execute_cmd(test_sh, "actuator set_setpoint " FAKE_ACTUATOR_NAME " -1000");
	zassert_ok(ret);
	zassert_equal(actuator_fake_set_setpoint_fake.call_count, 2);
	zassert_equal(actuator_fake_set_setpoint_fake.arg0_val, test_dev);
	zassert_equal(actuator_fake_set_setpoint_fake.arg1_val, INT32_MIN);

	ret = shell_execute_cmd(test_sh, "actuator set_setpoint " FAKE_ACTUATOR_NAME " 1000");
	zassert_ok(ret);
	zassert_equal(actuator_fake_set_setpoint_fake.call_count, 3);
	zassert_equal(actuator_fake_set_setpoint_fake.arg0_val, test_dev);
	zassert_equal(actuator_fake_set_setpoint_fake.arg1_val, INT32_MAX);

	shell_backend_dummy_clear_output(test_sh);
	ret = shell_execute_cmd(test_sh, "actuator set_setpoint " FAKE_ACTUATOR_NAME " -1001");
	zassert_equal(ret, -EINVAL);
	zassert_equal(actuator_fake_set_setpoint_fake.call_count, 3);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\n-1001 not valid\r\n");

	ret = shell_execute_cmd(test_sh, "actuator set_setpoint " FAKE_ACTUATOR_NAME " 1001");
	zassert_equal(ret, -EINVAL);
	zassert_equal(actuator_fake_set_setpoint_fake.call_count, 3);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\n1001 not valid\r\n");

	actuator_fake_set_setpoint_fake.custom_fake = test_set_setpoint_eio;

	ret = shell_execute_cmd(test_sh, "actuator set_setpoint " FAKE_ACTUATOR_NAME " 0");
	zassert_equal(ret, -EIO);
	zassert_equal(actuator_fake_set_setpoint_fake.call_count, 4);
	zassert_equal(actuator_fake_set_setpoint_fake.arg1_val, 0);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\nfailed to set setpoint\r\n");
}
