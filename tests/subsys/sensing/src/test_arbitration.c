/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensor.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/emul.h>

#include "icm42688_emul.h"
#include "icm42688_reg.h"

ZTEST(sensing, test_single_connection_arbitration)
{
	const struct emul *icm42688 = EMUL_DT_GET(DT_NODELABEL(icm42688));
	const struct sensing_sensor_info *sensor = SENSING_SENSOR_INFO_GET(
		DT_NODELABEL(accelgyro), SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D);
	const struct sensing_callback_list cb_list;
	sensing_sensor_handle_t handle;

	zassert_not_null(sensor);

	/* Open connection */
	zassert_ok(sensing_open_sensor(sensor, &cb_list, &handle));

	struct sensing_sensor_attribute attribute = {
		.attribute = SENSOR_ATTR_SAMPLING_FREQUENCY,
		.value = FIELD_PREP(GENMASK(31, 24), 100),
		.shift = 8,
	};
	zassert_ok(sensing_set_attributes(handle, SENSING_SENSOR_MODE_DONE, &attribute, 1));

	uint8_t reg_val;
	icm42688_emul_get_reg(icm42688, REG_ACCEL_CONFIG0, &reg_val, 1);

	zassert_equal(0b1000, FIELD_GET(MASK_ACCEL_ODR, reg_val), "ACCEL_CONFIG0=0x%02x",
		      FIELD_GET(MASK_ACCEL_ODR, reg_val));
}

ZTEST(sensing, test_double_connection_arbitration)
{
	const struct emul *icm42688 = EMUL_DT_GET(DT_NODELABEL(icm42688));
	const struct sensing_sensor_info *sensor = SENSING_SENSOR_INFO_GET(
		DT_NODELABEL(accelgyro), SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D);
	const struct sensing_callback_list cb_list;
	sensing_sensor_handle_t handles[2];
	uint8_t reg_val;

	zassert_not_null(sensor);

	/* Open connection */
	zassert_ok(sensing_open_sensor(sensor, &cb_list, &handles[0]));
	zassert_ok(sensing_open_sensor(sensor, &cb_list, &handles[1]));

	struct sensing_sensor_attribute attribute = {
		.attribute = SENSOR_ATTR_SAMPLING_FREQUENCY,
		.value = FIELD_PREP(GENMASK(31, 23), 100),
		.shift = 9,
	};
	zassert_ok(sensing_set_attributes(handles[0], SENSING_SENSOR_MODE_DONE, &attribute, 1));

	attribute.value = FIELD_PREP(GENMASK(31, 23), 200);
	zassert_ok(sensing_set_attributes(handles[1], SENSING_SENSOR_MODE_DONE, &attribute, 1));

	icm42688_emul_get_reg(icm42688, REG_ACCEL_CONFIG0, &reg_val, 1);
	zassert_equal(0b0111, FIELD_GET(MASK_ACCEL_ODR, reg_val), "ACCEL_CONFIG0=0x%02x",
		      FIELD_GET(MASK_ACCEL_ODR, reg_val));

	/* Close the second connection and check that we're back to 100Hz */
	zassert_ok(sensing_close_sensor(handles[1]));
	icm42688_emul_get_reg(icm42688, REG_ACCEL_CONFIG0, &reg_val, 1);
	zassert_equal(0b1000, FIELD_GET(MASK_ACCEL_ODR, reg_val), "ACCEL_CONFIG0=0x%02x",
		      FIELD_GET(MASK_ACCEL_ODR, reg_val));
}
