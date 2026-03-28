/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/firmware/tisci/tisci.h>

ZTEST(tisci, test_tisci_api)
{
	const struct device *dmsc = DEVICE_DT_GET(DT_NODELABEL(dmsc));

	zassert_not_null(dmsc, "Unable to get dev");
	zassert_true(device_is_ready(dmsc), "DMSC device not ready");

	struct tisci_version_info ver;
	int ret = tisci_cmd_get_revision(dmsc, &ver);

	zassert_ok(ret, "Failed to get TISCI revision");

	uint64_t freq = 0;

	ret = tisci_cmd_clk_get_freq(dmsc, 152, 0, &freq);

	zassert_ok(ret, "Failed to get clock freq");

	uint64_t min_freq = 96000000, target_freq = 96000000, max_freq = 96000000;

	ret = tisci_cmd_clk_set_freq(dmsc, 152, 0, min_freq, target_freq, max_freq);

	zassert_ok(ret, "Failed to set clock freq");

	freq = 0;
	ret = tisci_cmd_clk_get_freq(dmsc, 152, 0, &freq);

	zassert_ok(ret, "Failed to get clock freq after set");
	zassert_equal(freq, target_freq, "Clock freq after set does not match target");

	ret = tisci_cmd_get_device(dmsc, 0);

	zassert_ok(ret, "Failed to turn ON power domain device 0");

	uint32_t clcnt = 0, resets = 0;
	uint8_t p_state = 0, c_state = 0;
	int state_ret = tisci_get_device_state(dmsc, 0, &clcnt, &resets, &p_state, &c_state);

	zassert_ok(state_ret, "Failed to get device 0 state after ON");

	ret = tisci_cmd_put_device(dmsc, 0);

	zassert_ok(ret, "Failed to turn OFF power domain device 0");

	clcnt = resets = 0;
	p_state = c_state = 0;
	state_ret = tisci_get_device_state(dmsc, 0, &clcnt, &resets, &p_state, &c_state);

	zassert_ok(state_ret, "Failed to get device 0 state after OFF");
}

ZTEST_SUITE(tisci, NULL, NULL, NULL, NULL, NULL);
