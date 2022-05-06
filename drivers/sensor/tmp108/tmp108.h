/*
 * Copyright (c) 2021 Jimmy Johnson <catch22@fastmail.net>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMP108_TMP108_H_
#define ZEPHYR_DRIVERS_SENSOR_TMP108_TMP108_H_

#include <stdint.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor/tmp108.h>

#define TI_TMP108_REG_TEMP		0x00   /** Temperature register */
#define TI_TMP108_REG_CONF		0x01   /** Configuration register */
#define TI_TMP108_REG_LOW_LIMIT		0x02   /** Low alert set register */
#define TI_TMP108_REG_HIGH_LIMIT	0x03   /** High alert set register */


#define TI_TMP108_CONF_M0	0x0100 /** Mode 1 configuration bit */
#define TI_TMP108_CONF_M1	0x0200 /** Mode 2 configuration bit */
#define TI_TMP108_CONF_CR0	0x2000 /** Conversion rate 1 configuration bit */
#define TI_TMP108_CONF_CR1	0x4000 /** Conversion rate 2 configuration bit */
#define TI_TMP108_CONF_POL	0x0080 /** Alert pin Polarity configuration bit */
#define TI_TMP108_CONF_TM	0x0400 /** Thermostat mode setting bit */
#define TI_TMP108_CONF_HYS1	0x0020 /** Temperature hysteresis config 1 bit  */
#define TI_TMP108_CONF_HYS0	0x0010 /** Temperature hysteresis config 2 bit */
#define TI_TMP108_CONF_WFH	OVER_TEMP_MASK
#define TI_TMP108_CONF_WFL	UNDER_TEMP_MASK

#define TI_TMP108_MODE_SHUTDOWN		0
#define TI_TMP108_MODE_ONE_SHOT		TI_TMP108_CONF_M0
#define TI_TMP108_MODE_CONTINUOUS	TI_TMP108_CONF_M1
#define TI_TMP108_MODE_MASK		~(TI_TMP108_CONF_M1 | TI_TMP108_CONF_M1)

#define TI_TMP108_FREQ_4_SECS	0
#define TI_TMP108_FREQ_1_HZ	TI_TMP108_CONF_CR0
#define TI_TMP108_FREQ_4_HZ	TI_TMP108_CONF_CR1
#define TI_TMP108_FREQ_16_HZ	(TI_TMP108_CONF_CR1 | TI_TMP108_CONF_CR0)
#define TI_TMP108_FREQ_MASK	~(TI_TMP108_CONF_M1 | TI_TMP108_CONF_M1)

#define TI_TMP108_CONF_POL_LOW		0
#define TI_TMP108_CONF_POL_HIGH		TI_TMP108_CONF_POL
#define TI_TMP108_CONF_POL_MASK		~(TI_TMP108_CONF_POL)

#define TI_TMP108_CONF_TM_CMP		0
#define TI_TMP108_CONF_TM_INT		TI_TMP108_CONF_TM
#define TI_TMP108_CONF_TM_MASK		~(TI_TMP108_CONF_TM)

#define TI_TMP108_HYSTER_0_C	0
#define TI_TMP108_HYSTER_1_C	TI_TMP108_CONF_HYS0
#define TI_TMP108_HYSTER_2_C	TI_TMP108_CONF_HYS1
#define TI_TMP108_HYSTER_4_C	(TI_TMP108_CONF_HYS1 | TI_TMP108_CONF_HYS0)
#define TI_TMP108_HYSTER_MASK	~(TI_TMP108_CONF_HYS1 | TI_TMP108_CONF_HYS0)

struct tmp108_data {
	const struct device *tmp108_dev;

	uint16_t sample;

	bool one_shot_mode;

	struct k_work_delayable scheduled_work;

	struct sensor_trigger temp_alert_trigger;
	sensor_trigger_handler_t temp_alert_handler;

	sensor_trigger_handler_t data_ready_handler;
	struct sensor_trigger data_ready_trigger;

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
