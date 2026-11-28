/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nxp_lcu.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

ZTEST(nxp_lcu, test_ownership_and_cell_isolation)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(lcu_0));
	const struct device *other = DEVICE_DT_GET(DT_NODELABEL(lcu_1));
	struct nxp_lcu_cell_config config = {
		.outputs = {{.lut = 0xAAAAU}},
		.software_inputs = BIT(0),
	};
	struct nxp_lcu_state state;
	uint8_t owners[2];

	zassert_true(device_is_ready(dev));
	zassert_true(device_is_ready(other));
	zassert_ok(nxp_lcu_request_cell(dev, 0, &owners[0]));
	zassert_equal(nxp_lcu_request_cell(dev, 0, &owners[1]), -EBUSY);
	zassert_equal(nxp_lcu_release(dev, 0, &owners[1]), -EPERM);
	zassert_ok(nxp_lcu_request_cell(dev, 1, &owners[1]));
	zassert_ok(nxp_lcu_request_cell(other, 0, &owners[1]));
	zassert_ok(nxp_lcu_configure(dev, 0, &owners[0], &config));
	zassert_ok(nxp_lcu_configure(dev, 1, &owners[1], &config));
	zassert_ok(nxp_lcu_configure(other, 0, &owners[1], &config));
	zassert_ok(nxp_lcu_set_inputs(dev, 1, &owners[1], BIT(0)));
	zassert_ok(nxp_lcu_set_outputs(dev, 1, &owners[1], BIT(0)));
	zassert_ok(nxp_lcu_set_outputs(other, 0, &owners[1], BIT(0)));
	zassert_ok(nxp_lcu_set_outputs(dev, 0, &owners[0], BIT(0)));
	zassert_equal(nxp_lcu_configure(dev, 0, &owners[0], &config), -EBUSY);
	zassert_ok(nxp_lcu_release(dev, 0, &owners[0]));
	k_busy_wait(1U);
	zassert_ok(nxp_lcu_get_state(dev, 1, &owners[1], &state));
	zassert_equal(state.enabled, BIT(0));
	zassert_equal(state.outputs & BIT(0), BIT(0));
	zassert_ok(nxp_lcu_get_state(other, 0, &owners[1], &state));
	zassert_equal(state.enabled, BIT(0));
	zassert_ok(nxp_lcu_release(dev, 1, &owners[1]));
	zassert_ok(nxp_lcu_release(other, 0, &owners[1]));
}

ZTEST(nxp_lcu, test_invalid_configuration_does_not_enable_outputs)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(lcu_0));
	struct nxp_lcu_cell_config config = {0};
	struct nxp_lcu_state state;

	zassert_true(device_is_ready(dev));
	zassert_equal(nxp_lcu_request_cell(dev, 3, dev), -EINVAL);
	zassert_equal(nxp_lcu_request_cell(dev, 0, NULL), -EINVAL);
	zassert_ok(nxp_lcu_request_cell(dev, 0, dev));
	zassert_equal(nxp_lcu_set_outputs(dev, 0, dev, BIT(0)), -EINVAL);
	config.sources[0] = 25U;
	zassert_equal(nxp_lcu_configure(dev, 0, dev, &config), -EINVAL);
	zassert_ok(nxp_lcu_get_state(dev, 0, dev, &state));
	zassert_equal(state.enabled, 0);
	config.sources[0] = 0U;
	zassert_ok(nxp_lcu_configure(dev, 0, dev, &config));
	zassert_equal(nxp_lcu_set_outputs(dev, 0, dev, BIT(4)), -EINVAL);
	zassert_ok(nxp_lcu_get_state(dev, 0, dev, &state));
	zassert_equal(state.enabled, 0);
	zassert_ok(nxp_lcu_release(dev, 0, dev));
}

ZTEST_SUITE(nxp_lcu, NULL, NULL, NULL, NULL, NULL);
