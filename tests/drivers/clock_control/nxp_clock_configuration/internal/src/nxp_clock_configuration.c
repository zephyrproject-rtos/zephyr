/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

/* Variable defined at SOC level for system clock frequency */
const struct device *clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_PATH(zephyr_user)));
const clock_control_subsys_t subsys =
	(clock_control_subsys_t)DT_CLOCKS_CELL(DT_PATH(zephyr_user), name);

ZTEST(nxp_clock_config, validate_freq) {
	int ret;
	uint32_t rate;

	ret = clock_control_get_rate(clock_dev, subsys, &rate);
	zassert_equal(ret, 0, "Could not get clock subsys rate");
	/* Validate that the system clock frequency matches what we expect */
	zassert_equal(rate, DT_PROP(DT_PATH(zephyr_user), core_freq));
	TC_PRINT("System Core clock was %d\n", rate);
};

ZTEST_SUITE(nxp_clock_config, NULL, NULL, NULL, NULL, NULL);
