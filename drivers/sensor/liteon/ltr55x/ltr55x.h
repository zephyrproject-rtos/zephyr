/*
 * Copyright (c) 2025 Konrad Sikora
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LITEON_LTR55X_LTR55X_H_
#define ZEPHYR_DRIVERS_SENSOR_LITEON_LTR55X_LTR55X_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* Register addresses */
#define LTR55X_ALS_CONTR      0x80
#define LTR55X_MEAS_RATE      0x85
#define LTR55X_PART_ID        0x86
#define LTR55X_MANUFAC_ID     0x87
#define LTR55X_ALS_DATA_CH1_0 0x88
#define LTR55X_ALS_DATA_CH1_1 0x89
#define LTR55X_ALS_DATA_CH0_0 0x8A
#define LTR55X_ALS_DATA_CH0_1 0x8B
#define LTR55X_ALS_PS_STATUS  0x8C

/* Bit masks and shifts for ALS_CONTR register */
#define LTR55X_ALS_CONTR_MODE_MASK      BIT(0)
#define LTR55X_ALS_CONTR_MODE_SHIFT     0
#define LTR55X_ALS_CONTR_SW_RESET_MASK  BIT(1)
#define LTR55X_ALS_CONTR_SW_RESET_SHIFT 1
#define LTR55X_ALS_CONTR_GAIN_MASK      GENMASK(4, 2)
#define LTR55X_ALS_CONTR_GAIN_SHIFT     2

/* Bit masks and shifts for MEAS_RATE register */
#define LTR55X_MEAS_RATE_REPEAT_MASK    GENMASK(2, 0)
#define LTR55X_MEAS_RATE_REPEAT_SHIFT   0
#define LTR55X_MEAS_RATE_INT_TIME_MASK  GENMASK(5, 3)
#define LTR55X_MEAS_RATE_INT_TIME_SHIFT 3

/* Bit masks and shifts for PART_ID register */
#define LTR55X_PART_ID_REVISION_MASK  GENMASK(3, 0)
#define LTR55X_PART_ID_REVISION_SHIFT 0
#define LTR55X_PART_ID_NUMBER_MASK    GENMASK(7, 4)
#define LTR55X_PART_ID_NUMBER_SHIFT   4

/* Bit masks and shifts for MANUFAC_ID register */
#define LTR55X_MANUFAC_ID_IDENTIFICATION_MASK  GENMASK(7, 0)
#define LTR55X_MANUFAC_ID_IDENTIFICATION_SHIFT 0

/* Bit masks and shifts for ALS_STATUS register */
#define LTR55X_ALS_PS_STATUS_PS_DATA_STATUS_MASK   BIT(0)
#define LTR55X_ALS_PS_STATUS_PS_DATA_STATUS_SHIFT  0
#define LTR55X_ALS_PS_STATUS_PS_INTR_STATUS_MASK   BIT(1)
#define LTR55X_ALS_PS_STATUS_PS_INTR_STATUS_SHIFT  1
#define LTR55X_ALS_PS_STATUS_ALS_DATA_STATUS_MASK  BIT(2)
#define LTR55X_ALS_PS_STATUS_ALS_DATA_STATUS_SHIFT 2
#define LTR55X_ALS_PS_STATUS_ALS_INTR_STATUS_MASK  BIT(3)
#define LTR55X_ALS_PS_STATUS_ALS_INTR_STATUS_SHIFT 3
#define LTR55X_ALS_PS_STATUS_ALS_GAIN_MASK         GENMASK(6, 4)
#define LTR55X_ALS_PS_STATUS_ALS_GAIN_SHIFT        4
#define LTR55X_ALS_PS_STATUS_ALS_DATA_VALID_MASK   BIT(7)
#define LTR55X_ALS_PS_STATUS_ALS_DATA_VALID_SHIFT  7

#define LTR553_ALS_CONTR_MODE_ACTIVE 0x1

/* Expected sensor IDs */
#define LTR329_PART_ID_VALUE         0xA0
#define LTR55X_MANUFACTURER_ID_VALUE 0x05

/* Timing definitions - refer to LTR-329ALS-01 datasheet */
#define LTR55X_INIT_STARTUP_MS        100
#define LTR55X_WAKEUP_FROM_STANDBY_MS 10

/* Convert als-gain value in device-tree to register values */
#define LTR55X_ALS_GAIN_VALUE_1  0
#define LTR55X_ALS_GAIN_VALUE_2  1
#define LTR55X_ALS_GAIN_VALUE_4  2
#define LTR55X_ALS_GAIN_VALUE_8  3
#define LTR55X_ALS_GAIN_VALUE_48 6
#define LTR55X_ALS_GAIN_VALUE_96 7

/* Macros to set and get register fields */
#define LTR55X_REG_SET(reg, field, value)                                                          \
	(((value) << LTR55X_##reg##_##field##_SHIFT) & LTR55X_##reg##_##field##_MASK)
#define LTR55X_REG_GET(reg, field, value)                                                          \
	(((value) & LTR55X_##reg##_##field##_MASK) >> LTR55X_##reg##_##field##_SHIFT)

struct ltr55x_config {
	const struct i2c_dt_spec bus;
	uint8_t als_gain;
	uint8_t als_integration_time;
	uint8_t als_measurement_rate;
};

struct ltr55x_data {
	uint16_t als_ch0;
	uint16_t als_ch1;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_LITEON_LTR55X_LTR55X_H_ */
