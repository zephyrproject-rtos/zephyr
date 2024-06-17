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

#define GET_TARGET_DEVICE(node_id, prop, n) DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, n)),

/* Get the devicetree constants */
constexpr const struct device *controller = DEVICE_DT_GET(CONTROLLER_LABEL);
constexpr const struct device *targets[FORWARD_COUNT] = {
	DT_FOREACH_PROP_ELEM(CONTROLLER_LABEL, forwards, GET_TARGET_DEVICE)};

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

ZTEST(i2c_emul_forwarding, test_write_is_forwarded)
{
	// Try writing some values
	for (uint8_t data = 0; data < 10; ++data) {
		const int expected_call_count = 1 + data;

		zassert_ok(i2c_write(controller, &data, sizeof(data),
				     emulated_target_config[0].address));

		// Expected no reads to be made
		zexpect_equal(0, target_read_requested_0_fake.call_count);
		zexpect_equal(0, target_read_processed_0_fake.call_count);

		// Expected N write requests to be made
		zexpect_equal(expected_call_count, target_write_requested_0_fake.call_count,
			      "Expected to be called %d times, got %d", expected_call_count,
			      target_write_requested_0_fake.call_count);
		zexpect_equal(expected_call_count, target_write_received_0_fake.call_count,
			      "Expected to be called %d times, got %d", expected_call_count,
			      target_write_received_0_fake.call_count);

		// Check that the data written was correct
		zexpect_equal(data, target_write_received_0_fake.arg1_val,
			      "Expected data value %u. got %u", data,
			      target_write_received_0_fake.arg1_val);

		// Expect 1 stop call per write request
		zexpect_equal(expected_call_count, target_stop_0_fake.call_count,
			      "Expected to be called %d times, got %d", expected_call_count,
			      target_stop_0_fake.call_count);
	}
}

ZTEST(i2c_emul_forwarding, test_read_is_forwarded)
{
	// Try reading some values
	for (uint8_t i = 0; i < 10; ++i) {
		const uint8_t expected_data[2] = {
			static_cast<uint8_t>(0x1 * i),
			static_cast<uint8_t>(0x2 * i),
		};
		const unsigned int expected_call_count = 1 + i;
		uint8_t data[2];

		// Setup some lambdas to do the actual reads using 'expected_data'
		target_read_requested_0_fake.custom_fake =
			[expected_data](struct i2c_target_config *, uint8_t *out) -> int {
			*out = expected_data[0];
			return 0;
		};
		target_read_processed_0_fake.custom_fake =
			[expected_data](struct i2c_target_config *, uint8_t *out) -> int {
			*out = expected_data[1];
			return 0;
		};
		zassert_ok(i2c_read(controller, data, sizeof(data),
				    emulated_target_config[0].address));

		// Expect the read functions to be called N times
		zexpect_equal(expected_call_count, target_read_requested_0_fake.call_count,
			      "Expected to be called %d times, got %d", expected_call_count,
			      target_read_requested_0_fake.call_count);
		zexpect_equal(expected_call_count, target_read_processed_0_fake.call_count,
			      "Expected to be called %d times, got %d", expected_call_count,
			      target_read_processed_0_fake.call_count);

		// Expect the data read to match
		zexpect_equal(expected_data[0], data[0], "Expected 0x%02x, got 0x%02x",
			      expected_data[0], data[0]);
		zexpect_equal(expected_data[1], data[1], "Expected 0x%02x, got 0x%02x",
			      expected_data[1], data[1]);

		// Expect 0 write calls
		zexpect_equal(0, target_write_requested_0_fake.call_count);
		zexpect_equal(0, target_write_received_0_fake.call_count);

		// Expect 1 stop call per read
		zexpect_equal(expected_call_count, target_stop_0_fake.call_count,
			      "Expected to be called %d times, got %d", expected_call_count,
			      target_stop_0_fake.call_count);
	}
}

ZTEST(i2c_emul_forwarding, test_recover_failed_write)
{
	uint8_t write_data[2];

	// Fail on the write request (should never call the write_received function)
	target_write_requested_0_fake.return_val = -EINVAL;
	zassert_equal(-EINVAL, i2c_write(controller, write_data, sizeof(write_data),
					 emulated_target_config[0].address));
	zexpect_equal(1, target_write_requested_0_fake.call_count, "Was called %d times",
		      target_write_requested_0_fake.call_count);
	zexpect_equal(0, target_write_received_0_fake.call_count, "Was called %d times",
		      target_write_requested_0_fake.call_count);

	// Next instruction should succeed
	target_write_requested_0_fake.return_val = 0;
	zassert_ok(i2c_write(controller, write_data, sizeof(write_data),
			     emulated_target_config[0].address));
	zexpect_equal(2, target_write_requested_0_fake.call_count, "Was called %d times",
		      target_write_requested_0_fake.call_count);
	zexpect_equal(2, target_write_received_0_fake.call_count, "Was called %d times",
		      target_write_requested_0_fake.call_count);
}

ZTEST(i2c_emul_forwarding, test_recover_failed_read)
{
	uint8_t read_data[2];

	// Fail the read_requested (should never call the read_processed function)
	target_read_requested_0_fake.return_val = -EINVAL;
	zassert_equal(-EINVAL, i2c_read(controller, read_data, sizeof(read_data),
					emulated_target_config[0].address));
	zexpect_equal(1, target_read_requested_0_fake.call_count, "Was called %d times",
		      target_read_requested_0_fake.call_count);
	zexpect_equal(0, target_read_processed_0_fake.call_count, "Was called %d times",
		      target_read_processed_0_fake.call_count);

	// Next instruction should pass
	target_read_requested_0_fake.return_val = 0;
	zassert_ok(i2c_read(controller, read_data, sizeof(read_data),
			    emulated_target_config[0].address));
	zexpect_equal(2, target_read_requested_0_fake.call_count, "Was called %d times",
		      target_read_requested_0_fake.call_count);
	zexpect_equal(1, target_read_processed_0_fake.call_count, "Was called %d times",
		      target_read_processed_0_fake.call_count);
}

ZTEST(i2c_emul_forwarding, test_transfer_is_forwarded)
{
	uint8_t write_data[1] = {};
	uint8_t read_data[2] = {};

	struct i2c_msg msgs[] = {
		{
			.buf = write_data,
			.len = sizeof(write_data),
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = read_data,
			.len = sizeof(read_data),
			.flags = I2C_MSG_READ | I2C_MSG_STOP,
		},
	};

	int phase = 0;
	target_write_requested_0_fake.custom_fake = [&phase](struct i2c_target_config *) -> int {
		zassert_equal(0, phase, "Expected a call to write_requested before anything else");
		phase++;
		return 0;
	};
	target_write_received_0_fake.custom_fake = [&phase](struct i2c_target_config *,
							    uint8_t) -> int {
		zassert_equal(1, phase, "Expected a call to write_received as the second step");
		phase++;
		return 0;
	};
	target_read_requested_0_fake.custom_fake = [&phase](struct i2c_target_config *,
							    uint8_t *) -> int {
		zassert_equal(2, phase, "Expected a call to read_requested as the 3rd step");
		phase++;
		return 0;
	};
	target_read_processed_0_fake.custom_fake = [&phase](struct i2c_target_config *,
							    uint8_t *) -> int {
		zassert_equal(3, phase, "Expected a call to read_processed as the 4th step");
		phase++;
		return 0;
	};
	target_stop_0_fake.custom_fake = [&phase](struct i2c_target_config *) -> int {
		zassert_equal(4, phase, "Expected a call to stop as the 5th step");
		phase++;
		return 0;
	};
	zassert_ok(i2c_transfer(controller, msgs, ARRAY_SIZE(msgs),
				emulated_target_config[0].address));
	zexpect_equal(1, target_write_requested_0_fake.call_count,
		      "Expected target_write_requested to be called once, but got %d",
		      target_write_requested_0_fake.call_count);
	zexpect_equal(1, target_write_received_0_fake.call_count,
		      "Expected target_write_received to be called once, but got %d",
		      target_write_received_0_fake.call_count);
	zexpect_equal(1, target_read_requested_0_fake.call_count,
		      "Expected target_read_requested to be called once, but got %d",
		      target_read_requested_0_fake.call_count);
	zexpect_equal(1, target_read_processed_0_fake.call_count,
		      "Expected target_read_processed to be called once, but got %d",
		      target_read_processed_0_fake.call_count);
	zexpect_equal(1, target_stop_0_fake.call_count,
		      "Expected target_stop to be called once, but got %d",
		      target_stop_0_fake.call_count);
	zexpect_equal(5, phase, "Expected a total of 5 phases, but got %d", phase);
}

ZTEST(i2c_emul_forwarding, test_forward_two_targets)
{
	uint8_t read_data[2];

	// Read the second forward and ensure that we only forwarded to the correct one
	zassert_ok(i2c_read(controller, read_data, sizeof(read_data),
			    emulated_target_config[1].address));

	// Check that we got the forward
	zexpect_equal(1, target_read_requested_1_fake.call_count,
		      "Expected to be called 1 time, got %d",
		      target_read_requested_1_fake.call_count);
	zexpect_equal(1, target_read_processed_1_fake.call_count,
		      "Expected to be called 1 time, got %d",
		      target_read_processed_1_fake.call_count);

	// Check that we didn't forward to the first target
	zexpect_equal(0, target_read_requested_0_fake.call_count,
		      "Expected to be called 0 times, got %d",
		      target_read_requested_0_fake.call_count);
	zexpect_equal(0, target_read_processed_0_fake.call_count,
		      "Expected to be called 0 times, got %d",
		      target_read_processed_0_fake.call_count);
}
} // namespace
