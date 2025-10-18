/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bmi323.h"
#include <zephyr/device.h>
#include <zephyr/drivers/i3c.h>

#define IMU_BOSCH_BMI323_REG_I3C_DUMMY_OFFSET 0x2

static int bosch_bmi323_i3c_read_words(const void *context, uint8_t offset, uint16_t *words,
				       uint16_t words_count)
{
	const struct i3c_dt_spec *i3c = (const struct i3c_dt_spec *)context;
	uint8_t dbuf[(words_count * 2) + IMU_BOSCH_BMI323_REG_I3C_DUMMY_OFFSET];
	int ret;

	ret = i3c_read_dt(i3c, offset, dbuf, sizeof(dbuf));
	if (!ret) {
		/* Copy actual data except first 2 bytes of dummy data */
		memcpy(words, &dbuf[IMU_BOSCH_BMI323_REG_I3C_DUMMY_OFFSET], (words_count * 2));
	}
	k_usleep(2);

	return ret;
}

static int bosch_bmi323_i3c_write_words(const void *context, uint8_t offset, uint16_t *words,
					uint16_t words_count)
{
	const struct i3c_dt_spec *i3c = (const struct i3c_dt_spec *)context;
	uint8_t dbuf[(words_count * 2) + sizeof(offset)];
	int ret;

	/* Prepare buffer with offset and data */
	dbuf[0] = offset;
	memcpy(&dbuf[sizeof(offset)], words, (words_count * 2));

	ret = i3c_write_dt(i3c, dbuf, sizeof(dbuf));
	k_usleep(2);

	return ret;
}

static int bosch_bmi323_i3c_init(const void *context)
{
	const struct i3c_dt_spec *i3c = (const struct i3c_dt_spec *)context;
	uint16_t sensor_id[2];
	int ret;

	if (i3c_is_ready_dt(i3c) == false) {
		return -ENODEV;
	}

	ret = i3c_read_dt(i3c, IMU_BOSCH_BMI323_REG_CHIP_ID, (uint8_t *)sensor_id,
			(IMU_BOSCH_BMI323_REG_I3C_DUMMY_OFFSET * 2));
	if (ret < 0) {
		return ret;
	}

	return 0;
}

const struct bosch_bmi323_bus_api bosch_bmi323_i3c_bus_api = {
	.read_words = bosch_bmi323_i3c_read_words,
	.write_words = bosch_bmi323_i3c_write_words,
	.init = bosch_bmi323_i3c_init
};
