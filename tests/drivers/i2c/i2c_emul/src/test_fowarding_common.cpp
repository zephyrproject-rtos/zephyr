/*
 * Copyright 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */
#include "emulated_target.hpp"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

namespace
{

/* Get the devicetree constants */
constexpr const struct device *controller = DEVICE_DT_GET(CONTROLLER_LABEL);
constexpr const struct device *targets[FORWARD_COUNT] = {
	DT_FOREACH_PROP_ELEM_SEP(CONTROLLER_LABEL, forwards, DEVICE_DT_GET_BY_IDX, (,))};

static void *i2c_emul_forwarding_setup(void)
{
	// Register the target
	for (int i = 0; i < FORWARD_COUNT; ++i) {
		zassert_ok(i2c_target_register(targets[i], &emulated_target_config[i]));
	}

	return NULL;
}

static void i2c_emul_forwarding_before(void *fixture)
{
	ARG_UNUSED(fixture);

	// Reset all fakes
	FFF_FAKES_LIST_FOREACH(RESET_FAKE);
	FFF_RESET_HISTORY();
}

static void i2c_emul_forwarding_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	// Unregister the I2C target callbacks
	for (int i = 0; i < FORWARD_COUNT; ++i) {
		zassert_ok(i2c_target_unregister(targets[i], &emulated_target_config[i]));
	}
}

ZTEST_SUITE(i2c_emul_forwarding, NULL, i2c_emul_forwarding_setup, i2c_emul_forwarding_before, NULL,
	    i2c_emul_forwarding_teardown);

/* Common tests */

ZTEST(i2c_emul_forwarding, test_invalid_address_for_target)
{
	uint8_t data = 0;
	int rc = i2c_write(targets[0], &data, 1, emulated_target_config[0].address + 1);
	zassert_equal(-EINVAL, rc, "Expected %d (-EINVAL), but got %d", -EINVAL, rc);
	zexpect_equal(0, target_read_requested_0_fake.call_count);
	zexpect_equal(0, target_read_processed_0_fake.call_count);
	zexpect_equal(0, target_write_requested_0_fake.call_count);
	zexpect_equal(0, target_write_received_0_fake.call_count);
	zexpect_equal(0, target_buf_write_received_0_fake.call_count);
	zexpect_equal(0, target_buf_read_requested_0_fake.call_count);
	zexpect_equal(0, target_stop_0_fake.call_count);
}

ZTEST(i2c_emul_forwarding, test_error_in_stop)
{
	uint8_t data = 0;

	target_stop_0_fake.return_val = -EINTR;
	zassert_equal(-EINTR, i2c_write(controller, &data, 1, emulated_target_config[0].address));
	zexpect_equal(1, target_stop_0_fake.call_count);
}

} // namespace
