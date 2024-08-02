/*
 * Copyright (c) 2021 Jimmy Johnson <catch22@fastmail.net>
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMP108_TMP108_H_
#define ZEPHYR_DRIVERS_SENSOR_TMP108_TMP108_H_

#include <stdint.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor/tmp108.h>

#define TI_TMP108_REG_TEMP		0x00   /** Temperature register */
#define TI_TMP108_REG_CONF		0x01   /** Configuration register */
#define TI_TMP108_REG_LOW_LIMIT		0x02   /** Low alert set register */
#define TI_TMP108_REG_HIGH_LIMIT	0x03   /** High alert set register */

#define AMS_AS6212_CONF	{.CONF_HYS1 = TI_TMP108_CONF_NA,\
			 .CONF_HYS0 = TI_TMP108_CONF_NA,\
			 .CONF_CR0  = 0x0040,	\
			 .CONF_CR1  = 0x0080,	\
			 .CONF_M1   = 0x0000,	\
			 .CONF_TM   = 0x0200,	\
			 .CONF_POL  = 0x0400,	\
			 .CONF_M0   = 0x8000,	\
			 .CONF_RST  = 0x0080,	\
			 .TEMP_MULT = 15625,	\
			 .TEMP_DIV  = 2,        \
			 .WAKEUP_TIME_IN_MS = 120 }

#define TI_TMP108_CONF	{.CONF_HYS0  = 0x0010,	\
			 .CONF_HYS1  = 0x0020,	\
			 .CONF_POL   = 0x0080,	\
			 .CONF_M0    = 0x0100,	\
			 .CONF_M1    = 0x0200,	\
			 .CONF_TM    = 0x0400,	\
			 .CONF_CR0   = 0x2000,	\
			 .CONF_CR1   = 0x4000,	\
			 .CONF_RST   = 0x0022,	\
			 .TEMP_MULT  = 15625,	\
			 .TEMP_DIV   = 4,       \
			 .WAKEUP_TIME_IN_MS = 30 }

#define TI_TMP108_MODE_SHUTDOWN(x) 0
#define TI_TMP108_MODE_ONE_SHOT(x) TI_TMP108_CONF_M0(x)
#define TI_TMP108_MODE_CONTINUOUS(x) TI_TMP108_CONF_M1(x)
#define TI_TMP108_MODE_MASK(x)	~(TI_TMP108_CONF_M0(x) | TI_TMP108_CONF_M1(x))

#define TI_TMP108_FREQ_4_SECS(x) 0
#define TI_TMP108_FREQ_1_HZ(x) TI_TMP108_GET_CONF(x, CONF_CR0)
#define TI_TMP108_FREQ_4_HZ(x) TI_TMP108_GET_CONF(x, CONF_CR1)
#define TI_TMP108_FREQ_16_HZ(x)	(TI_TMP108_GET_CONF(x, CONF_CR1) | \
				TI_TMP108_GET_CONF(x, CONF_CR0))
#define TI_TMP108_FREQ_MASK(x)	~(TI_TMP108_GET_CONF(x, CONF_CR1) | \
				TI_TMP108_GET_CONF(x, CONF_CR0))

#define TI_TMP108_CONF_POL_LOW(x) 0
#define TI_TMP108_CONF_POL_HIGH(x) TI_TMP108_GET_CONF(x, CONF_POL)
#define TI_TMP108_CONF_POL_MASK(x) ~(TI_TMP108_GET_CONF(x, CONF_POL))

#define TI_TMP108_CONF_TM_CMP(x) 0
#define TI_TMP108_CONF_TM_INT(x) TI_TMP108_GET_CONF(x, CONF_TM)
#define TI_TMP108_CONF_TM_MASK(x) ~(TI_TMP108_GET_CONF(x, CONF_TM))

#define TI_TMP108_HYSTER_0_C(x)	0
#define TI_TMP108_HYSTER_1_C(x)	TI_TMP108_GET_CONF(x, CONF_HYS0)
#define TI_TMP108_HYSTER_2_C(x)	TI_TMP108_GET_CONF(x, CONF_HYS1)
#define TI_TMP108_HYSTER_4_C(x)	(TI_TMP108_GET_CONF(x, CONF_HYS1) | \
				TI_TMP108_GET_CONF(x, CONF_HYS0))
#define TI_TMP108_HYSTER_MASK(x) ~(TI_TMP108_GET_CONF(x, CONF_HYS1) | \
				 TI_TMP108_GET_CONF(x, CONF_HYS0))

#define TI_TMP108_CONF_M1(x) TI_TMP108_GET_CONF(x, CONF_M1)
#define TI_TMP108_CONF_M0(x) TI_TMP108_GET_CONF(x, CONF_M0)

#define TMP108_TEMP_MULTIPLIER(x)	TI_TMP108_GET_CONF(x, TEMP_MULT)
#define TMP108_TEMP_DIVISOR(x)	TI_TMP108_GET_CONF(x, TEMP_DIV)
#define TMP108_WAKEUP_TIME_IN_MS(x)	TI_TMP108_GET_CONF(x, WAKEUP_TIME_IN_MS)
#define TMP108_CONF_RST(x)	TI_TMP108_GET_CONF(x, CONF_RST)

#define TI_TMP108_CONF_NA 0x0000

struct tmp_108_reg_def {
	uint16_t CONF_M0;	/** Mode 1 configuration bit */
	uint16_t CONF_M1;	/** Mode 2 configuration bit */
	uint16_t CONF_CR0;	/** Conversion rate 1 configuration bit */
	uint16_t CONF_CR1;	/** Conversion rate 2 configuration bit */
	uint16_t CONF_POL;	/** Alert pin Polarity configuration bit */
	uint16_t CONF_TM;	/** Thermostat mode setting bit */
	uint16_t CONF_HYS1;	/** Temperature hysteresis config 1 bit  */
	uint16_t CONF_HYS0;	/** Temperature hysteresis config 2 bit */
	int32_t TEMP_MULT;	/** Temperature multiplier */
	int32_t TEMP_DIV;	/** Temperature divisor */
	uint16_t WAKEUP_TIME_IN_MS; /** Wake up and conversion time from one shot */
	uint16_t CONF_RST;	/** default reset values on init */
};

#define TI_TMP108_GET_CONF(x, cfg) ((struct tmp108_config *)(x->config))->reg_def.cfg

struct tmp108_config {
	const struct i2c_dt_spec i2c_spec;
	const struct gpio_dt_spec alert_gpio;
	struct tmp_108_reg_def reg_def;
};

struct tmp108_data {
	const struct device *tmp108_dev;

	int16_t sample;

	bool one_shot_mode;

	struct k_work_delayable scheduled_work;

	const struct sensor_trigger *temp_alert_trigger;
	sensor_trigger_handler_t temp_alert_handler;

	sensor_trigger_handler_t data_ready_handler;
	const struct sensor_trigger *data_ready_trigger;

	struct gpio_callback temp_alert_gpio_cb;
};

int tmp_108_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int tmp108_reg_read(const struct device *dev, uint8_t reg, uint16_t *val);

int ti_tmp108_read_temp(const struct device *dev);
void tmp108_trigger_handle_one_shot(struct k_work *work);
void tmp108_trigger_handle_alert(const struct device *port,
				 struct gpio_callback *cb,
				 gpio_port_pins_t pins);

#endif /*  ZEPHYR_DRIVERS_SENSOR_TMP108_TMP108_H_ */
