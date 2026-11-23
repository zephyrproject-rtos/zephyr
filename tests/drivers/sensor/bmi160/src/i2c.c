/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#include "bmi160.h"
#include "checks.h"
#include "fixture.h"

static int mock_i2c_transfer_fail_reg_number = -1;

static int mock_i2c_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
			     int addr)
{
	ARG_UNUSED(target);
	ARG_UNUSED(addr);
	if (mock_i2c_transfer_fail_reg_number >= 0 && i2c_is_read_op(&msgs[1]) &&
	    bmi160_i2c_is_touching_reg(msgs, num_msgs, mock_i2c_transfer_fail_reg_number)) {
		return -EIO;
	}
	return -ENOSYS;
}

ZTEST_USER_F(bmi160, test_bmi160_i2c_get_offset_fail_to_read_offset_acc)
{
	struct i2c_emul_api mock_bus_api;
	struct sensor_value value;

	fixture->emul_i2c->bus.i2c->mock_api = &mock_bus_api;
	mock_bus_api.transfer = mock_i2c_transfer;

	enum sensor_channel channels[] = {SENSOR_CHAN_ACCEL_XYZ, SENSOR_CHAN_GYRO_XYZ};
	int fail_registers[] = {BMI160_REG_OFFSET_ACC_X, BMI160_REG_OFFSET_ACC_Y,
				BMI160_REG_OFFSET_ACC_Z, BMI160_REG_OFFSET_GYR_X,
				BMI160_REG_OFFSET_GYR_Y, BMI160_REG_OFFSET_GYR_Z,
				BMI160_REG_OFFSET_EN};

	for (int fail_reg_idx = 0; fail_reg_idx < ARRAY_SIZE(fail_registers); ++fail_reg_idx) {
		mock_i2c_transfer_fail_reg_number = fail_registers[fail_reg_idx];
		for (int chan_idx = 0; chan_idx < ARRAY_SIZE(channels); ++chan_idx) {
			zassert_equal(-EIO, sensor_attr_get(fixture->dev_i2c, channels[chan_idx],
							    SENSOR_ATTR_OFFSET, &value));
		}
	}
}
