/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#include "bmi160.h"
#include "checks.h"
#include "fixture.h"

static int mock_spi_io_fail_reg_number = -1;

static int mock_spi_io(const struct emul *target, const struct spi_config *config,
		       const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	ARG_UNUSED(target);
	ARG_UNUSED(config);
	if (mock_spi_io_fail_reg_number >= 0 &&
	    bmi160_spi_is_touching_reg(tx_bufs, rx_bufs, mock_spi_io_fail_reg_number)) {
		return -EIO;
	}
	return -ENOSYS;
}

ZTEST_USER_F(bmi160, test_bmi160_spi_get_offset_fail_to_read_offset_acc)
{
	struct spi_emul_api mock_bus_api;
	struct sensor_value value;

	fixture->emul_spi->bus.spi->mock_api = &mock_bus_api;
	mock_bus_api.io = mock_spi_io;

	enum sensor_channel channels[] = {SENSOR_CHAN_ACCEL_XYZ, SENSOR_CHAN_GYRO_XYZ};
	int fail_registers[] = {BMI160_REG_OFFSET_ACC_X, BMI160_REG_OFFSET_ACC_Y,
				BMI160_REG_OFFSET_ACC_Z, BMI160_REG_OFFSET_GYR_X,
				BMI160_REG_OFFSET_GYR_Y, BMI160_REG_OFFSET_GYR_Z,
				BMI160_REG_OFFSET_EN};

	for (int fail_reg_idx = 0; fail_reg_idx < ARRAY_SIZE(fail_registers); ++fail_reg_idx) {
		mock_spi_io_fail_reg_number = fail_registers[fail_reg_idx];
		for (int chan_idx = 0; chan_idx < ARRAY_SIZE(channels); ++chan_idx) {
			zassert_equal(-EIO, sensor_attr_get(fixture->dev_spi, channels[chan_idx],
							    SENSOR_ATTR_OFFSET, &value));
		}
	}
}
