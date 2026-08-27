/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/i2c.h>

#include "bmi323_i2c.h"

static int bosch_bmi323_i2c_read_words(const void *context, uint8_t offset, uint16_t *words,
				       uint16_t words_count)
{
	const struct i2c_dt_spec *i2c = (const struct i2c_dt_spec *)context;
	uint8_t dbuf[(words_count * 2) + IMU_BOSCH_BMI323_I2C_DUMMY_OFFSET];
	int ret;

	ret = i2c_burst_read_dt(i2c, offset, dbuf, sizeof(dbuf));
	if (!ret) {
		/* Copy actual data except first 2 bytes of dummy data */
		memcpy(words, &dbuf[IMU_BOSCH_BMI323_I2C_DUMMY_OFFSET], (words_count * 2));
	}
	k_usleep(2);

	return ret;
}

static int bosch_bmi323_i2c_write_words(const void *context, uint8_t offset, uint16_t *words,
					uint16_t words_count)
{
	const struct i2c_dt_spec *i2c = (const struct i2c_dt_spec *)context;
	uint8_t dbuf[(words_count * 2) + sizeof(offset)];
	int ret;

	/* Prepare buffer with offset and data */
	dbuf[0] = offset;
	memcpy(&dbuf[sizeof(offset)], words, (words_count * 2));

	ret = i2c_write_dt(i2c, dbuf, sizeof(dbuf));
	k_usleep(2);

	return ret;
}

static int bosch_bmi323_i2c_init(const void *context)
{
	const struct i2c_dt_spec *i2c = (const struct i2c_dt_spec *)context;
	uint8_t sensor_id_raw[2 + IMU_BOSCH_BMI323_I2C_DUMMY_OFFSET];
	uint16_t sensor_id;
	int ret;

	if (!i2c_is_ready_dt(i2c)) {
		return -ENODEV;
	}

	ret = i2c_burst_read_dt(i2c, IMU_BOSCH_BMI323_REG_CHIP_ID, sensor_id_raw,
				sizeof(sensor_id_raw));
	if (ret < 0) {
		return ret;
	}

	/* Skip 2 dummy bytes to get actual chip ID */
	sensor_id = sys_get_le16(&sensor_id_raw[IMU_BOSCH_BMI323_I2C_DUMMY_OFFSET]);
	if ((sensor_id & 0xFF) != 0x43) {
		return -ENODEV;
	}

	return 0;
}

const struct bosch_bmi323_bus_api bosch_bmi323_i2c_bus_api = {
	.read_words = bosch_bmi323_i2c_read_words,
	.write_words = bosch_bmi323_i2c_write_words,
	.init = bosch_bmi323_i2c_init
};
