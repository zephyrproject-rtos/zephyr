/*
 * Copyright 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */
#include "emulated_target.hpp"
#include <cstdint>

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

ZTEST(i2c_emul_forwarding, test_write_is_forwarded)
{
	uint8_t data[] = {0x00, 0x01, 0x02};

	target_buf_write_received_0_fake.custom_fake = [&data](struct i2c_target_config *,
							       uint8_t *buf, uint32_t len) {
		zassert_equal(ARRAY_SIZE(data), len);
		zexpect_mem_equal(data, buf, len);
	};

	zassert_ok(
		i2c_write(controller, data, ARRAY_SIZE(data), emulated_target_config[0].address));

	// Expect 0 reads and 1 write/stop to be made
	zexpect_equal(0, target_buf_read_requested_0_fake.call_count);
	zexpect_equal(1, target_buf_write_received_0_fake.call_count);
	zexpect_equal(1, target_stop_0_fake.call_count);
}

ZTEST(i2c_emul_forwarding, test_read_is_forwarded)
{
	uint8_t expected[] = {0x01, 0x02, 0x03};
	uint8_t data[ARRAY_SIZE(expected)] = {};

	/* Set the custom fake function to a lambda which captures the expected value as a reference.
	 * This means that when the function is executed, we can access 'expected' as though it were
	 * within the lambda's scope.
	 */
	target_buf_read_requested_0_fake.custom_fake = [&expected](struct i2c_target_config *,
								   uint8_t **ptr, uint32_t *len) {
		*ptr = expected;
		*len = ARRAY_SIZE(expected);
		return 0;
	};

	zassert_ok(i2c_read(controller, data, ARRAY_SIZE(expected),
			    emulated_target_config[0].address));

	// Expect 1 read/stop and 0 write to be made
	zexpect_equal(1, target_buf_read_requested_0_fake.call_count);
	zexpect_equal(0, target_buf_write_received_0_fake.call_count);
	zexpect_equal(1, target_stop_0_fake.call_count);
	zexpect_mem_equal(expected, data, ARRAY_SIZE(expected));
}

ZTEST(i2c_emul_forwarding, test_failed_read_request)
{
	uint8_t data;
	target_buf_read_requested_0_fake.return_val = -EINTR;

	zassert_equal(-EINTR, i2c_read(controller, &data, 1, emulated_target_config[0].address));
	zexpect_equal(1, target_buf_read_requested_0_fake.call_count);
	zexpect_equal(0, target_buf_write_received_0_fake.call_count);
	zexpect_equal(0, target_stop_0_fake.call_count);
}

ZTEST(i2c_emul_forwarding, test_read_request_overflow)
{
	uint8_t data;

	/* Set the custom_fake to a local lambda with no capture values. */
	target_buf_read_requested_0_fake.custom_fake = [](struct i2c_target_config *, uint8_t **_,
							  uint32_t *len) {
		*len = UINT32_MAX;
		return 0;
	};

	zassert_equal(-ENOMEM, i2c_read(controller, &data, 1, emulated_target_config[0].address));
	zexpect_equal(1, target_buf_read_requested_0_fake.call_count);
	zexpect_equal(0, target_buf_write_received_0_fake.call_count);
	zexpect_equal(0, target_stop_0_fake.call_count);
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
	target_buf_write_received_0_fake.custom_fake = [&phase](struct i2c_target_config *,
								uint8_t *, uint32_t) {
		zassert_equal(0, phase,
			      "Expected a call to buf_write_received before anything else");
		phase++;
	};
	target_buf_read_requested_0_fake.custom_fake = [&phase](struct i2c_target_config *,
								uint8_t **ptr, uint32_t *len) {
		zassert_equal(1, phase, "Expected a call to buf_Read_requested as the second step");
		phase++;

		// Write a random byte. It doesn't make a difference.
		*ptr = (uint8_t *)&phase;
		*len = 1;
		return 0;
	};
	target_stop_0_fake.custom_fake = [&phase](struct i2c_target_config *) -> int {
		zassert_equal(2, phase, "Expected a call to stop as the 3rd step");
		phase++;
		return 0;
	};

	zassert_ok(i2c_transfer(controller, msgs, ARRAY_SIZE(msgs),
				emulated_target_config[0].address));
	zexpect_equal(1, target_buf_write_received_0_fake.call_count);
	zexpect_equal(1, target_buf_read_requested_0_fake.call_count);
	zexpect_equal(1, target_stop_0_fake.call_count);
	zexpect_equal(3, phase, "Expected a total of 3 phases, but got %d", phase);
}

ZTEST(i2c_emul_forwarding, test_call_pio_forwarded_bus_when_buffering_enabled)
{
	uint8_t data[2];

	zassert_ok(i2c_read(controller, data, ARRAY_SIZE(data), emulated_target_config[1].address));
	zexpect_equal(1, target_read_requested_1_fake.call_count);
	zexpect_equal(1, target_read_processed_1_fake.call_count);
	zexpect_equal(1, target_stop_1_fake.call_count);
}

} // namespace
