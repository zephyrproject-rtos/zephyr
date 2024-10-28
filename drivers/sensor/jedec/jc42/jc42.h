/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_JEDEC_JC42_H_
#define ZEPHYR_DRIVERS_SENSOR_JEDEC_JC42_H_

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#define JC42_REG_CONFIG      0x01
#define JC42_REG_UPPER_LIMIT 0x02
#define JC42_REG_LOWER_LIMIT 0x03
#define JC42_REG_CRITICAL    0x04
#define JC42_REG_TEMP_AMB    0x05

/* 16 bits control configuration and state.
 *
 * * Bit 0 controls alert signal output mode
 * * Bit 1 controls interrupt polarity
 * * Bit 2 disables upper and lower threshold checking
 * * Bit 3 enables alert signal output
 * * Bit 4 records alert status
 * * Bit 5 records interrupt status
 * * Bit 6 locks the upper/lower window registers
 * * Bit 7 locks the critical register
 * * Bit 8 enters shutdown mode
 * * Bits 9-10 control threshold hysteresis
 */
#define JC42_CFG_ALERT_MODE_INT BIT(0)
#define JC42_CFG_ALERT_ENA      BIT(3)
#define JC42_CFG_ALERT_STATE    BIT(4)
#define JC42_CFG_INT_CLEAR      BIT(5)

/* 16 bits are used for temperature and state encoding:
 * * Bits 0..11 encode the temperature in a 2s complement signed value
 *   in Celsius with 1/16 Cel resolution
 * * Bit 12 is set to indicate a negative temperature
 * * Bit 13 is set to indicate a temperature below the lower threshold
 * * Bit 14 is set to indicate a temperature above the upper threshold
 * * Bit 15 is set to indicate a temperature above the critical threshold
 */
#define JC42_TEMP_SCALE_CEL 16 /* signed */
#define JC42_TEMP_SIGN_BIT  BIT(12)
#define JC42_TEMP_ABS_MASK  ((uint16_t)(JC42_TEMP_SIGN_BIT - 1U))
#define JC42_TEMP_LWR_BIT   BIT(13)
#define JC42_TEMP_UPR_BIT   BIT(14)
#define JC42_TEMP_CRT_BIT   BIT(15)

#define JC42_REG_RESOLUTION 0x08

struct jc42_data {
	uint16_t reg_val;

#ifdef CONFIG_JC42_TRIGGER
	struct gpio_callback alert_cb;

	const struct device *dev;

	const struct sensor_trigger *trig;
	sensor_trigger_handler_t trigger_handler;
#endif

#ifdef CONFIG_JC42_TRIGGER_OWN_THREAD
	struct k_sem sem;
#endif

#ifdef CONFIG_JC42_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
};

struct jc42_config {
	struct i2c_dt_spec i2c;
	uint8_t resolution;
#ifdef CONFIG_JC42_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif /* CONFIG_JC42_TRIGGER */
};

int jc42_reg_read(const struct device *dev, uint8_t reg, uint16_t *val);
int jc42_reg_write_16bit(const struct device *dev, uint8_t reg, uint16_t val);
int jc42_reg_write_8bit(const struct device *dev, uint8_t reg, uint8_t val);

#ifdef CONFIG_JC42_TRIGGER
int jc42_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		  const struct sensor_value *val);
int jc42_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		     sensor_trigger_handler_t handler);
int jc42_setup_interrupt(const struct device *dev);
#endif /* CONFIG_JC42_TRIGGER */

/* Encode a signed temperature in scaled Celsius to the format used in
 * register values.
 */
static inline uint16_t jc42_temp_reg_from_signed(int temp)
{
	/* Get the 12-bit 2s complement value */
	uint16_t rv = temp & JC42_TEMP_ABS_MASK;

	if (temp < 0) {
		rv |= JC42_TEMP_SIGN_BIT;
	}
	return rv;
}

/* Decode a register temperature value to a signed temperature in
 * scaled Celsius.
 */
static inline int jc42_temp_signed_from_reg(uint16_t reg)
{
	int rv = reg & JC42_TEMP_ABS_MASK;

	if (reg & JC42_TEMP_SIGN_BIT) {
		/* Convert 12-bit 2s complement to signed negative
		 * value.
		 */
		rv = -(1U + (rv ^ JC42_TEMP_ABS_MASK));
	}
	return rv;
}

#endif /* ZEPHYR_DRIVERS_SENSOR_JEDEC_JC42_H_ */
