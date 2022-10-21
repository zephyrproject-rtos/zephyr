/*
 * Copyright (c) 2022 Luke Holt
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

/* Register addresses */

#define LTR390_MAIN_CTRL   0x00 /* R/W */
#define LTR390_MEAS_RATE   0x04 /* R/W */
#define LTR390_GAIN	   0x05 /* R/W */
#define LTR390_PART_ID	   0x06 /*  R  */
#define LTR390_MAIN_STATUS 0x07 /*  R  */
#define LTR390_ALS_DATA_0  0x0D /*  R  */
#define LTR390_ALS_DATA_1  0x0E /*  R  */
#define LTR390_ALS_DATA_2  0x0F /*  R  */
#define LTR390_UVS_DATA_0  0x10 /*  R  */
#define LTR390_UVS_DATA_1  0x11 /*  R  */
#define LTR390_UVS_DATA_2  0x12 /*  R  */
#define LTR390_INT_CFG	   0x19 /* R/W */
#define LTR390_INT_PST	   0x1A /* R/W */
#define LTR390_THRES_UP_0  0x21 /* R/W */
#define LTR390_THRES_UP_1  0x22 /* R/W */
#define LTR390_THRES_UP_2  0x23 /* R/W */
#define LTR390_THRES_LO_0  0x24 /* R/W */
#define LTR390_THRES_LO_1  0x25 /* R/W */
#define LTR390_THRES_LO_2  0x26 /* R/W */

/* Reset values for the writeable registers */

#define LTR390_RESET_MAIN_CTRL	0x00
#define LTR390_RESET_MEAS_RATE	0x22
#define LTR390_RESET_GAIN	0x01
#define LTR390_RESET_INT_CFG	0x10
#define LTR390_RESET_INT_PST	0x00
#define LTR390_RESET_THRES_UP_0 0xFF
#define LTR390_RESET_THRES_UP_1 0xFF
#define LTR390_RESET_THRES_UP_2 0x0F
#define LTR390_RESET_THRES_LO_0 0x00
#define LTR390_RESET_THRES_LO_1 0x00
#define LTR390_RESET_THRES_LO_2 0x00

/* MAIN_CTRL register bits */

#define LTR390_MC_SW_RESET 1 << 4
#define LTR390_MC_ALS_MODE 0
#define LTR390_MC_UVS_MODE 1 << 3
#define LTR390_MC_ACTIVE   1 << 1
#define LTR390_MC_STANDBY  0

/* MAIN_STATUS register bits */

#define LTR390_MS_POWER_ON	1 << 5
#define LTR390_MS_INT_TRIGGERED 1 << 4
#define LTR390_MS_NEW_DATA	1 << 3

/* INT_CFG register bits */

#define LTR390_IC_ALS_CHAN    1 << 4
#define LTR390_IC_UVS_CHAN    3 << 4
#define LTR390_IC_INT_ENABLE  1 << 2
#define LTr390_IC_INT_DISABLE 0

enum ltr390_mode {
	LTR390_MODE_ALS,
	LTR390_MODE_UVS,
};

enum ltr390_resolution {
	LTR390_RESOLUTION_20BIT,
	LTR390_RESOLUTION_19BIT,
	LTR390_RESOLUTION_18BIT,
	LTR390_RESOLUTION_17BIT,
	LTR390_RESOLUTION_16BIT,
	LTR390_RESOLUTION_13BIT,
};

enum ltr390_rate {
	LTR390_RATE_25MS,
	LTR390_RATE_50MS,
	LTR390_RATE_100MS,
	LTR390_RATE_200MS,
	LTR390_RATE_500MS,
	LTR390_RATE_1000MS,
	LTR390_RATE_2000MS,
};

enum ltr390_gain {
	LTR390_GAIN_1,
	LTR390_GAIN_3,
	LTR390_GAIN_6,
	LTR390_GAIN_9,
	LTR390_GAIN_18,
};

struct ltr390_data {
	uint32_t light;
	uint32_t uv_index;

#ifdef CONFIG_LTR390_TRIGGER
	struct gpio_callback alert_cb;
	const struct device *dev;
	struct sensor_trigger trig;
	sensor_trigger_handler_t trigger_handler;
#endif

#ifdef CONFIG_LTR390_TRIGGER_OWN_THREAD
	struct k_sem sem;
#endif

#ifdef CONFIG_LTR390_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
};

struct ltr390_config {
	struct i2c_dt_spec i2c;

	/*
	 * Sensor measurement resolution.
	 * Power-up default is 18 Bit.
	 */
	enum ltr390_resolution resolution;

	/*
	 * Rate at which the sensor takes measurements. If the rate is smaller than
	 * the time it takes to convert the measurement (resolution times), the rate
	 * will be lower than programmed. It will be the maximum allowed speed
	 * according to the resolution setting. Power-up default is a rate of 100ms.
	 */
	enum ltr390_rate rate;

	/*
	 * Measurement gain range setting.
	 * Power-up default is a gain of 3.
	 */
	enum ltr390_gain gain;

#ifdef CONFIG_LTR390_TRIGGER

	struct gpio_dt_spec int_gpio;

	/*
	 * The number of consecutive measurements outside of threshold before
	 * an interrupt is triggered.
	 */
	uint8_t int_persist;

#endif
};

/**
 * @brief Read the value of a specified register
 *
 * @param cfg Device config structure
 * @param addr Register address
 * @param value Byte to write the register value to
 * @return int errno
 */
int ltr390_read_register(const struct ltr390_config *cfg, const uint8_t addr, uint8_t *value);

/**
 * @brief Write a value to a specified register
 *
 * @param cfg Device config structure
 * @param addr Register address
 * @param value Value to write to the register
 * @return int errno
 */
int ltr390_write_register(const struct ltr390_config *cfg, const uint8_t addr, const uint8_t value);

/**
 * @brief Perform multiplication on sensor_value.
 *
 * @param val sensor_value struct
 * @param mult multiplier
 */
void sv_mult(struct sensor_value *val, uint32_t mult);

/**
 * @brief Perform division on sensor_value.
 *
 * @param val sensor_value struct
 * @param div divisor
 */
void sv_div(struct sensor_value *val, uint32_t div);

#ifdef CONFIG_LTR390_TRIGGER
int ltr390_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		    const struct sensor_value *val);

int ltr390_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int ltr390_setup_interrupt(const struct device *dev);
#endif
