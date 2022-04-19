/*
 * Copyright (c) 2022 Nikolaus Huber
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SENSOR_VEML7700_H__
#define __SENSOR_VEML7700_H__

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#define VEML7700_DEFAULT_ADDR 0x10

/* Commands */
#define VEML7700_ALS_CONF_0     0x00    /* Control register */
#define VEML7700_ALS_WH         0x01    /* High threshold window setting (MSB) */
#define VEML7700_ALS_WL         0x02    /* Low threshold window setting (LSB) */
#define VEML7700_PSM            0x03    /* Power save mode setting */
#define VEML7700_ALS            0x04    /* Sensor data reading */
#define VEML7700_WHITE          0x05    /* White light data reading */
#define VEML7700_INTR_STAT      0x06    /* Interrupt status register */

#define VEML7700_INTERRUPT_HIGH 0x4000  /* Interrupt status for high threshold */
#define VEML7700_INTERRUPT_LOW  0x8000  /* Interrupt status for low threshold */

enum veml7700_gain_t {
	GAIN_X1,
	GAIN_X2,
	GAIN_X1_8,
	GAIN_X1_4
};

#define VEML7700_GAIN_MASK  0x03
#define VEML7700_GAIN_SHIFT 11

enum veml7700_it_t {
	IT_25MS,
	IT_50MS,
	IT_100MS,
	IT_200MS,
	IT_400MS,
	IT_800MS
};

#define VEML7700_IT_MASK    0x03c0
#define VEML7700_IT_SHIFT   6

enum veml7700_persist_t {
	PERSISTENCE_1,
	PERSISTENCE_2,
	PERSISTENCE_4,
	PERSISTENCE_8
};

#define VEML7700_PERS_MASK  0x30
#define VEML7700_PERS_SHIFT 4

enum veml7700_psm_mode_t {
	MODE_1,
	MODE_2,
	MODE_3,
	MODE_4
};

struct veml7700_data {
	/* Used for sensor reading conversion */
	float factor1;
	float factor2;

	/* The raw ALS reading from the sensor */
	uint16_t als_reading;
};

struct veml7700_config {
	struct i2c_dt_spec bus;
	enum veml7700_gain_t gain;
	enum veml7700_it_t it;
	enum veml7700_persist_t persistence;
	int psm_enabled;
	enum veml7700_psm_mode_t psm_mode;
};

#endif /* __SENSOR_VEML7700_H__ */
