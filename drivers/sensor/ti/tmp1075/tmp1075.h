/*
 * Copyright (c) 2024 Arrow Electronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMP1075_TMP1075_H_
#define ZEPHYR_DRIVERS_SENSOR_TMP1075_TMP1075_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

/* Extended resolution is not supported on TMP1075 */
#define TMP1075_DATA_NORMAL_SHIFT 4
#define uCELSIUS_IN_CELSIUS       1000000

#define TMP1075_REG_TEMPERATURE 0x00
#define TMP1075_REG_CONFIG      0x01
#define TMP1075_REG_TLOW        0x02
#define TMP1075_REG_THIGH       0x03

/* Scale in micro degrees Celsius -> 0.0625°C per ADC bit resolution */
#define TMP1075_TEMP_SCALE 62500

/* Macro to set or clear the TMP1075_OS (One-shot conversion mode) bit based on a boolean value */
#define TMP1075_SET_ONE_SHOT_CONVERSION(reg, enable)                                               \
	((reg) = ((reg) & ~(1 << 15)) | ((enable) << 15))

/* Macro to set the TMP1075_R (Conversion rate) bits */
#define TMP1075_SET_CONVERSION_RATE(reg, rate) ((reg) |= ((rate) << 13))

/* Macro to set the TMP1075_F (Consecutive fault measurements) bits */
#define TMP1075_SET_CONSECUTIVE_FAULT_MEASUREMENTS(reg, faults) ((reg) |= ((faults) << 11))

/* Macro to set or clear the TMP1075_POL (Polarity of output pin) bit based on a boolean value */
#define TMP1075_SET_ALERT_PIN_POLARITY(reg, activeHigh)                                            \
	((reg) = ((reg) & ~(1 << 10)) | ((activeHigh) << 10))

/* Macro to set or clear the TMP1075_TM (ALERT pin function) bit based on a boolean value */
#define TMP1075_SET_ALERT_PIN_FUNCTION(reg, interruptMode)                                         \
	((reg) = ((reg) & ~(1 << 9)) | ((interruptMode) << 9))

/* Macro to set or clear the TMP1075_SD (Shutdown mode) bit based on a boolean value */
#define TMP1075_SET_SHUTDOWN_MODE(reg, shutdown) ((reg) = ((reg) & ~(1 << 8)) | ((shutdown) << 8))

struct tmp1075_data {
	const struct device *tmp1075_dev;
	int16_t sample;
	uint16_t config_reg;
	const struct sensor_trigger *temp_alert_trigger;
	sensor_trigger_handler_t temp_alert_handler;
	struct gpio_callback temp_alert_gpio_cb;
	bool over_threshold;
};

struct tmp1075_config {
	const struct i2c_dt_spec bus;
	const struct gpio_dt_spec alert_gpio;
	uint8_t cr;
	uint8_t cf;
	bool alert_pol: 1;
	bool one_shot: 1;
	bool interrupt_mode: 1;
	bool shutdown_mode: 1;
};

int tmp1075_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

void tmp1075_trigger_handle_alert(const struct device *port, struct gpio_callback *cb,
				  gpio_port_pins_t pins);

#endif
