/*
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMP11X_TMP11X_H_
#define ZEPHYR_DRIVERS_SENSOR_TMP11X_TMP11X_H_

#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#define TMP11X_REG_TEMP        0x0
#define TMP11X_REG_CFGR        0x1
#define TMP11X_REG_HIGH_LIM    0x2
#define TMP11X_REG_LOW_LIM     0x3
#define TMP11X_REG_EEPROM_UL   0x4
#define TMP11X_REG_EEPROM1     0x5
#define TMP11X_REG_EEPROM2     0x6
#define TMP11X_REG_EEPROM3     0x7
#define TMP117_REG_TEMP_OFFSET 0x7
#define TMP11X_REG_EEPROM4     0x8
#define TMP11X_REG_DEVICE_ID   0xF

#define TMP11X_RESOLUTION     78125 /* in tens of uCelsius*/
#define TMP11X_RESOLUTION_DIV 10000000

#define TMP116_DEVICE_ID 0x1116
#define TMP117_DEVICE_ID 0x0117
#define TMP119_DEVICE_ID 0x2117

#define TMP11X_CFGR_RESET       BIT(1)
#define TMP11X_CFGR_AVG         (BIT(5) | BIT(6))
#define TMP11X_CFGR_CONV        (BIT(7) | BIT(8) | BIT(9))
#define TMP11X_CFGR_MODE        (BIT(10) | BIT(11))
#define TMP11X_CFGR_DATA_READY  BIT(13)
#define TMP11X_EEPROM_UL_UNLOCK BIT(15)
#define TMP11X_EEPROM_UL_BUSY   BIT(14)

/* Alert pin configuration bits */
#define TMP11X_CFGR_ALERT_DR_SEL  BIT(2) /* ALERT pin select (1=Data ready) (0=alert) */
#define TMP11X_CFGR_ALERT_PIN_POL BIT(3) /* Alert pin polarity */
#define TMP11X_CFGR_ALERT_MODE    BIT(4) /* Alert pin mode (1=therm, 0=alert) */

#define TMP11X_AVG_1_SAMPLE    0
#define TMP11X_AVG_8_SAMPLES   BIT(5)
#define TMP11X_AVG_32_SAMPLES  BIT(6)
#define TMP11X_AVG_64_SAMPLES  (BIT(5) | BIT(6))
#define TMP11X_MODE_CONTINUOUS 0
#define TMP11X_MODE_SHUTDOWN   BIT(10)
#define TMP11X_MODE_ONE_SHOT   (BIT(10) | BIT(11))

struct tmp11x_data {
	uint16_t sample;
	uint16_t id;
#ifdef CONFIG_TMP11X_TRIGGER
	const struct device *dev;
	struct gpio_callback alert_cb;
	sensor_trigger_handler_t alert_handler;
	const struct sensor_trigger *alert_trigger;

#if defined(CONFIG_TMP11X_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_TMP11X_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_TMP11X_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif /* CONFIG_TMP11X_TRIGGER_OWN_THREAD */
#endif /* CONFIG_TMP11X_TRIGGER */
};

struct tmp11x_dev_config {
	struct i2c_dt_spec bus;
	uint16_t odr;
	uint16_t oversampling;
	bool alert_pin_polarity;
	bool alert_mode;
	bool alert_dr_sel;
	bool store_attr_values;
#ifdef CONFIG_TMP11X_TRIGGER
	struct gpio_dt_spec alert_gpio;
#endif
};

/* Function declarations */
int tmp11x_write_config(const struct device *dev, uint16_t mask, uint16_t conf);
int tmp11x_reg_read(const struct device *dev, uint8_t reg, uint16_t *val);

#ifdef CONFIG_TMP11X_TRIGGER
int tmp11x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);
int tmp11x_init_interrupt(const struct device *dev);
#endif

#endif /*  ZEPHYR_DRIVERS_SENSOR_TMP11X_TMP11X_H_ */
