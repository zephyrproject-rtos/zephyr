/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MCP9808_MCP9808_H_
#define ZEPHYR_DRIVERS_SENSOR_MCP9808_MCP9808_H_

#include <errno.h>

#include <zephyr/types.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/util.h>
#include <drivers/gpio.h>

#define MCP9808_REG_CONFIG		0x01
#define MCP9808_REG_UPPER_LIMIT		0x02
#define MCP9808_REG_LOWER_LIMIT		0x03
#define MCP9808_REG_CRITICAL		0x04
#define MCP9808_REG_TEMP_AMB		0x05

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
#define MCP9808_CFG_ALERT_MODE_INT	BIT(0)
#define MCP9808_CFG_ALERT_ENA		BIT(3)
#define MCP9808_CFG_ALERT_STATE		BIT(4)
#define MCP9808_CFG_INT_CLEAR		BIT(5)

/* 16 bits are used for temperature and state encoding:
 * * Bits 0..11 encode the temperature in a 2s complement signed value
 *   in Celsius with 1/16 Cel resolution
 * * Bit 12 is set to indicate a negative temperature
 * * Bit 13 is set to indicate a temperature below the lower threshold
 * * Bit 14 is set to indicate a temperature above the upper threshold
 * * Bit 15 is set to indicate a temperature above the critical threshold
 */
#define MCP9808_TEMP_SCALE_CEL		16 /* signed */
#define MCP9808_TEMP_SIGN_BIT		BIT(12)
#define MCP9808_TEMP_ABS_MASK		((uint16_t)(MCP9808_TEMP_SIGN_BIT - 1U))
#define MCP9808_TEMP_LWR_BIT		BIT(13)
#define MCP9808_TEMP_UPR_BIT		BIT(14)
#define MCP9808_TEMP_CRT_BIT		BIT(15)

struct mcp9808_data {
	const struct device *i2c_master;

	uint16_t reg_val;

#ifdef CONFIG_MCP9808_TRIGGER
	const struct device *alert_gpio;
	struct gpio_callback alert_cb;

	const struct device *dev;

	struct sensor_trigger trig;
	sensor_trigger_handler_t trigger_handler;
#endif

#ifdef CONFIG_MCP9808_TRIGGER_OWN_THREAD
	struct k_sem sem;
#endif

#ifdef CONFIG_MCP9808_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
};

struct mcp9808_config {
	const char *i2c_bus;
	uint16_t i2c_addr;
#ifdef CONFIG_MCP9808_TRIGGER
	uint8_t alert_pin;
	uint8_t alert_flags;
	const char *alert_controller;
#endif /* CONFIG_MCP9808_TRIGGER */
};

int mcp9808_reg_read(const struct device *dev, uint8_t reg, uint16_t *val);

#ifdef CONFIG_MCP9808_TRIGGER
int mcp9808_attr_set(const struct device *dev, enum sensor_channel chan,
		     enum sensor_attribute attr,
		     const struct sensor_value *val);
int mcp9808_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);
int mcp9808_setup_interrupt(const struct device *dev);
#endif /* CONFIG_MCP9808_TRIGGER */

/* Encode a signed temperature in scaled Celsius to the format used in
 * register values.
 */
static inline uint16_t mcp9808_temp_reg_from_signed(int temp)
{
	/* Get the 12-bit 2s complement value */
	uint16_t rv = temp & MCP9808_TEMP_ABS_MASK;

	if (temp < 0) {
		rv |= MCP9808_TEMP_SIGN_BIT;
	}
	return rv;
}

/* Decode a register temperature value to a signed temperature in
 * scaled Celsius.
 */
static inline int mcp9808_temp_signed_from_reg(uint16_t reg)
{
	int rv = reg & MCP9808_TEMP_ABS_MASK;

	if (reg & MCP9808_TEMP_SIGN_BIT) {
		/* Convert 12-bit 2s complement to signed negative
		 * value.
		 */
		rv = -(1U + (rv ^ MCP9808_TEMP_ABS_MASK));
	}
	return rv;
}

#endif /* ZEPHYR_DRIVERS_SENSOR_MCP9808_MCP9808_H_ */
