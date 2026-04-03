/*
 * Copyright (c) 2020 Sven Herrmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MPR_H_
#define ZEPHYR_DRIVERS_SENSOR_MPR_H_

#include <zephyr/drivers/i2c.h>

/* MPR output measurement command */
#define MPR_OUTPUT_MEASUREMENT_COMMAND (0xAA)

/* MPR status byte masks */
#define MPR_STATUS_MASK_MATH_SATURATION       (0x01)
#define MPR_STATUS_MASK_INTEGRITY_TEST_FAILED (0x04)
#define MPR_STATUS_MASK_BUSY                  (0x20)
#define MPR_STATUS_MASK_POWER_ON              (0x40)

/* MPR register read maximum retries */
#ifndef MPR_REG_READ_MAX_RETRIES
#define MPR_REG_READ_MAX_RETRIES (3)
#endif

/* MPR register read data conversion delay [ms] */
#ifndef MPR_REG_READ_DATA_CONV_DELAY_MS
#define MPR_REG_READ_DATA_CONV_DELAY_MS (5)
#endif

struct mpr_data {
	uint32_t reg_val;
};

struct mpr_config {
	struct i2c_dt_spec i2c;
};

int mpr_reg_read(const struct device *dev, uint8_t reg, uint16_t *val);

#endif /* ZEPHYR_DRIVERS_SENSOR_MPR_H_ */
