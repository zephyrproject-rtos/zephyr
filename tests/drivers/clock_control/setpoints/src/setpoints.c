/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

const struct device *clock_dev = DEVICE_DT_GET_OR_NULL(DT_CLOCKS_CTLR(DT_PATH(zephyr_user)));

#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), clocks)

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_lpc_syscon)
const clock_control_subsys_t clock_subsys =
	(clock_control_subsys_t)DT_CLOCKS_CELL(DT_PATH(zephyr_user), name);
#else
#error "Unsupported clock controller"
#endif
#else
/*
 * For platforms without a "clocks" property, define this so the test builds,
 * although the test will be skipped at runtime
 */
const clock_control_subsys_t clock_subsys;
#endif

ZTEST(clock_setpoints, test_select_setpoints) {
	int ret;
	uint32_t rate;

	if (!device_is_ready(clock_dev)) {
		TC_PRINT("Setpoint test not supported, skipping\n");
		ztest_test_skip();
	}

	/* Select run setpoint */
	ret = clock_control_setpoint(clock_dev, CLOCK_SETPOINT_RUN);
	if (ret == -ENOSYS) {
		TC_PRINT("Setpoint test not supported, skipping\n");
		ztest_test_skip();
	}

	zassert_equal(ret, 0, "Could not select run setpoint");
	ret = clock_control_get_rate(clock_dev, clock_subsys, &rate);
	zassert_equal(ret, 0, "Could not get run clock subsys rate");
	/* Validate that the system clock frequency matches what we expect */
	zassert_equal(rate, DT_PROP_OR(DT_PATH(zephyr_user), run_freq, 0));
	TC_PRINT("Run setpoint clock was %d\n", rate);

	/* Select idle setpoint */
	ret = clock_control_setpoint(clock_dev, CLOCK_SETPOINT_IDLE);
	zassert_equal(ret, 0, "Could not select idle setpoint");
	ret = clock_control_get_rate(clock_dev, clock_subsys, &rate);
	zassert_equal(ret, 0, "Could not get idle clock subsys rate");
	/* Validate that the system clock frequency matches what we expect */
	zassert_equal(rate, DT_PROP_OR(DT_PATH(zephyr_user), idle_freq, 0));
	TC_PRINT("Idle setpoint clock was %d\n", rate);
};

ZTEST_SUITE(clock_setpoints, NULL, NULL, NULL, NULL, NULL);
