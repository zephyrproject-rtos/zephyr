/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Milan Pipaliya <milan.pipaliya@dnkmail.in>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file sts35.h
 * @brief STS35-DIS temperature sensor driver.
 *
 * Defines I2C commands, CRC-8 parameters, measurement repeatability levels,
 * driver configuration and runtime data structures for the STS35 Zephyr
 * sensor driver.
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_STS35_STS35_H_
#define ZEPHYR_DRIVERS_SENSOR_STS35_STS35_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

/* ---- Measurement commands ---- */
#define STS35_CMD_MEAS_HIGH   0x2400U /* Single shot, high repeatability */
#define STS35_CMD_MEAS_MEDIUM 0x240BU /* Single shot, medium repeatability */
#define STS35_CMD_MEAS_LOW    0x2416U /* Single shot, low repeatability */

/* ---- General commands ---- */
#define STS35_CMD_FETCH_DATA   0xE000U /* Fetch periodic measurement data */
#define STS35_CMD_BREAK        0x3093U /* Abort periodic measurement mode */
#define STS35_CMD_SOFT_RESET   0x30A2U /* Soft reset and reload calibration */
#define STS35_CMD_HEATER_EN    0x306DU /* Enable internal heater */
#define STS35_CMD_HEATER_DIS   0x3066U /* Disable internal heater */
#define STS35_CMD_READ_STATUS  0xF32DU /* Read 16-bit status register */
#define STS35_CMD_CLEAR_STATUS 0x3041U /* Clear status register alert flags */

/* ---- Data format ---- */
#define STS35_WORD_CRC_LEN 3U /* 16-bit data word followed by CRC-8 */

/* ---- Temperature conversion ---- */
#define STS35_TEMP_MIN_MC   (-45000000LL) /* Minimum temperature: -45 °C */
#define STS35_TEMP_RANGE_MC 175000000LL   /* Temperature range: 175 °C */
#define STS35_TEMP_RAW_MAX  65535U        /* Maximum 16-bit raw value */

/* ---- CRC-8 parameters ---- */
#define STS35_CRC8_POLY 0x31U /* CRC-8 polynomial: x^8 + x^5 + x^4 + 1 */
#define STS35_CRC8_INIT 0xFFU /* Initial CRC value */

/* ---- Timing constants ---- */
#define STS35_SOFT_RESET_TIME_MS 2U /* Reset recovery time */

/* ---- Measurement repeatability ---- */
enum sts35_repeatability {
	STS35_REPEATABILITY_LOW = 0, /* Lowest repeatability, shortest duration */
	STS35_REPEATABILITY_MEDIUM,  /* Medium repeatability and duration */
	STS35_REPEATABILITY_HIGH,    /* Highest repeatability, longest duration */
};

/* ---- Driver configuration ---- */
struct sts35_config {
	struct i2c_dt_spec bus; /**< I2C bus specification from devicetree. */

	uint8_t repeatability; /**< Measurement repeatability setting. */
};

/* ---- Runtime sensor data ---- */
struct sts35_data {
	uint16_t sample; /**< Raw 16-bit temperature measurement. */
};

#endif /* ZEPHYR_DRIVERS_SENSOR_STS35_STS35_H_ */
