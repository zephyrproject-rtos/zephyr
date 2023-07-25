/*
 * Copyright (c) 2020 Richard Osterloh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_VCNL4040_VCNL4040_H_
#define ZEPHYR_DRIVERS_SENSOR_VCNL4040_VCNL4040_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

/* Registers all 16 bits */
#define VCNL4040_REG_ALS_CONF		0x00
#define VCNL4040_REG_ALS_THDH		0x01
#define VCNL4040_REG_ALS_THDL		0x02
#define VCNL4040_REG_PS_CONF		0x03
#define VCNL4040_REG_PS_MS		0x04
#define VCNL4040_REG_PS_CANC		0x05
#define VCNL4040_REG_PS_THDL		0x06
#define VCNL4040_REG_PS_THDH		0x07
#define VCNL4040_REG_PS_DATA		0x08
#define VCNL4040_REG_ALS_DATA		0x09
#define VCNL4040_REG_WHITE_DATA		0x0A
#define VCNL4040_REG_INT_FLAG		0x0B
#define VCNL4040_REG_DEVICE_ID		0x0C

#define VCNL4040_RW_REG_COUNT		0x08 /* [0x00, 0x07] */

#define VCNL4040_DEFAULT_ID		0x0186

#define VCNL4040_LED_I_POS		8
#define VCNL4040_PS_HD_POS		11
#define VCNL4040_PS_HD_MASK		BIT(VCNL4040_PS_HD_POS)
#define VCNL4040_PS_DUTY_POS		6
#define VCNL4040_PS_IT_POS		1
#define VCNL4040_PS_SD_POS		0
#define VCNL4040_PS_SD_MASK		BIT(VCNL4040_PS_SD_POS)
#define VCNL4040_ALS_IT_POS		6
#define VCNL4040_ALS_INT_EN_POS		1
#define VCNL4040_ALS_INT_EN_MASK	BIT(VCNL4040_ALS_INT_EN_POS)
#define VCNL4040_ALS_SD_POS		0
#define VCNL4040_ALS_SD_MASK		BIT(VCNL4040_ALS_SD_POS)

enum led_current {
	VCNL4040_LED_CURRENT_50MA,
	VCNL4040_LED_CURRENT_75MA,
	VCNL4040_LED_CURRENT_100MA,
	VCNL4040_LED_CURRENT_120MA,
	VCNL4040_LED_CURRENT_140MA,
	VCNL4040_LED_CURRENT_160MA,
	VCNL4040_LED_CURRENT_180MA,
	VCNL4040_LED_CURRENT_200MA,
};

enum led_duty_cycle {
	VCNL4040_LED_DUTY_1_40,
	VCNL4040_LED_DUTY_1_80,
	VCNL4040_LED_DUTY_1_160,
	VCNL4040_LED_DUTY_1_320,
};

enum ambient_integration_time {
	VCNL4040_AMBIENT_INTEGRATION_TIME_80MS,
	VCNL4040_AMBIENT_INTEGRATION_TIME_160MS,
	VCNL4040_AMBIENT_INTEGRATION_TIME_320MS,
	VCNL4040_AMBIENT_INTEGRATION_TIME_640MS,
};

enum proximity_integration_time {
	VCNL4040_PROXIMITY_INTEGRATION_TIME_1T,
	VCNL4040_PROXIMITY_INTEGRATION_TIME_1_5T,
	VCNL4040_PROXIMITY_INTEGRATION_TIME_2T,
	VCNL4040_PROXIMITY_INTEGRATION_TIME_2_5T,
	VCNL4040_PROXIMITY_INTEGRATION_TIME_3T,
	VCNL4040_PROXIMITY_INTEGRATION_TIME_3_5T,
	VCNL4040_PROXIMITY_INTEGRATION_TIME_4T,
	VCNL4040_PROXIMITY_INTEGRATION_TIME_8T,
};

enum proximity_type {
	VCNL4040_PROXIMITY_INT_DISABLE,
	VCNL4040_PROXIMITY_INT_CLOSE,
	VCNL4040_PROXIMITY_INT_AWAY,
	VCNL4040_PROXIMITY_INT_CLOSE_AWAY,
};

enum interrupt_type {
	VCNL4040_PROXIMITY_AWAY = 1,
	VCNL4040_PROXIMITY_CLOSE,
	VCNL4040_AMBIENT_HIGH = 4,
	VCNL4040_AMBIENT_LOW,
};

struct vcnl4040_config {
	struct i2c_dt_spec i2c;
#ifdef CONFIG_VCNL4040_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif
	enum led_current led_i;
	enum led_duty_cycle led_dc;
	enum ambient_integration_time als_it;
	enum proximity_integration_time proxy_it;
	enum proximity_type proxy_type;
};

struct vcnl4040_data {
	struct k_mutex mutex;
#ifdef CONFIG_VCNL4040_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	enum interrupt_type int_type;
	sensor_trigger_handler_t proxy_handler;
	const struct sensor_trigger *proxy_trigger;
	sensor_trigger_handler_t als_handler;
	const struct sensor_trigger *als_trigger;
#endif
#ifdef CONFIG_VCNL4040_TRIGGER_OWN_THREAD
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_VCNL4040_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#endif
#ifdef CONFIG_VCNL4040_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
	uint16_t proximity;
	uint16_t light;
	float sensitivity;
};

int vcnl4040_read(const struct device *dev, uint8_t reg, uint16_t *out);
int vcnl4040_write(const struct device *dev, uint8_t reg, uint16_t value);

#ifdef CONFIG_VCNL4040_TRIGGER
int vcnl4040_trigger_init(const struct device *dev);

int vcnl4040_attr_set(const struct device *dev,
		      enum sensor_channel chan,
		      enum sensor_attribute attr,
		      const struct sensor_value *val);

int vcnl4040_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);
#endif

#endif
