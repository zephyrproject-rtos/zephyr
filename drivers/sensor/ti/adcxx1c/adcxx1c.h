/*
 * Copyright (c) 2024 Bert Outtier
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADCXX1C_ADCXX1C_H_
#define ZEPHYR_DRIVERS_SENSOR_ADCXX1C_ADCXX1C_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

/**
 * @name    ADCxx1C register addresses
 * @{
 */
#define ADCXX1C_CONV_RES_ADDR     (0)
#define ADCXX1C_ALERT_STATUS_ADDR (1)
#define ADCXX1C_CONF_ADDR         (2)
#define ADCXX1C_LOW_LIMIT_ADDR    (3)
#define ADCXX1C_HIGH_LIMIT_ADDR   (4)
#define ADCXX1C_HYSTERESIS_ADDR   (5)
#define ADCXX1C_LOWEST_CONV_ADDR  (6)
#define ADCXX1C_HIGHEST_CONV_ADDR (7)
/** @} */

/**
 * @name    ADCxx1C Config flags
 * @{
 */
#define ADCXX1C_CONF_ALERT_PIN_EN  BIT(2)
#define ADCXX1C_CONF_ALERT_FLAG_EN BIT(3)
/** @} */

/**
 * @brief   ADC resolution
 */
enum {
	ADCXX1C_RES_8BITS = 8,   /**< 8 bits resolution (ADC081C family) */
	ADCXX1C_RES_10BITS = 10, /**< 10 bits resolution (ADC101C family) */
	ADCXX1C_RES_12BITS = 12, /**< 12 bits resolution (ADC121C family) */
};

/**
 * @brief   module types
 */
enum {
	ADCXX1C_TYPE_ADC081C = 0, /**< 8 bits resolution (ADC081C family) */
	ADCXX1C_TYPE_ADC101C,     /**< 10 bits resolution (ADC101C family) */
	ADCXX1C_TYPE_ADC121C,     /**< 12 bits resolution (ADC121C family) */
};

/**
 * @brief   Conversion interval configuration value
 */
enum {
	ADCXX1C_CYCLE_DISABLED = 0, /**< No cycle conversion */
	ADCXX1C_CYCLE_32,           /**< Conversion cycle = Tconvert x 32 */
	ADCXX1C_CYCLE_64,           /**< Conversion cycle = Tconvert x 64 */
	ADCXX1C_CYCLE_128,          /**< Conversion cycle = Tconvert x 128 */
	ADCXX1C_CYCLE_256,          /**< Conversion cycle = Tconvert x 256 */
	ADCXX1C_CYCLE_512,          /**< Conversion cycle = Tconvert x 512 */
	ADCXX1C_CYCLE_1024,         /**< Conversion cycle = Tconvert x 1024 */
	ADCXX1C_CYCLE_2048,         /**< Conversion cycle = Tconvert x 2048 */
};

struct adcxx1c_config {
	struct i2c_dt_spec bus;

	int variant;
	int resolution; /**< resolution */
	uint8_t cycle;  /**< conversion interval */

#ifdef CONFIG_ADCXX1C_TRIGGER
	struct gpio_dt_spec alert_gpio;
#endif /* CONFIG_ADCXX1C_TRIGGER */
};

struct adcxx1c_data {
	int16_t v_sample;
	uint8_t bits;
	uint8_t conf;

#ifdef CONFIG_ADCXX1C_TRIGGER
	const struct device *dev;
	struct gpio_callback alert_cb;

	sensor_trigger_handler_t handler;
	const struct sensor_trigger *trigger;

#if defined(CONFIG_ADCXX1C_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ADCXX1C_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_ADCXX1C_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_ADCXX1C_TRIGGER */
};

int adcxx1c_read_regs(const struct device *dev, uint8_t reg, int16_t *out);

int adcxx1c_write_reg(const struct device *dev, uint8_t reg, uint8_t val);

#ifdef CONFIG_ADCXX1C_TRIGGER
int adcxx1c_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     const struct sensor_value *val);

int adcxx1c_attr_get(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     struct sensor_value *val);

int adcxx1c_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int adcxx1c_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ADCXX1C_ADCXX1C_H_ */
