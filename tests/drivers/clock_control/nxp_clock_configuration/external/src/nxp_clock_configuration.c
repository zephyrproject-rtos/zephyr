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
#if DT_HAS_COMPAT_STATUS_OKAY(nxp_lpc_syscon)
const clock_control_subsys_t clock_subsys =
	(clock_control_subsys_t)DT_CLOCKS_CELL(DT_PATH(zephyr_user), name);
#else
const clock_control_subsys_t clock_subsys;
#endif

ZTEST(nxp_clock_config, test_validate_freq) {
	int ret;
	uint32_t rate;

	ret = clock_control_get_rate(clock_dev, clock_subsys, &rate);
	zassert_equal(ret, 0, "Could not get clock subsys rate");
	/* Validate that the system clock frequency matches what we expect */
	zassert_equal(rate, DT_PROP(DT_PATH(zephyr_user), core_freq));
	TC_PRINT("System Core clock was %d\n", rate);
};

ZTEST_SUITE(nxp_clock_config, NULL, NULL, NULL, NULL, NULL);
