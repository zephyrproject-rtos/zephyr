/*
 * Copyright (c) 2017-2019 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_AMG88XX_AMG88XX_H_
#define ZEPHYR_DRIVERS_SENSOR_AMG88XX_AMG88XX_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

#define AMG88XX_PCLT		0x00 /* Setting Power control register */
#define AMG88XX_RST		0x01 /* Reset register */
#define AMG88XX_FPSC		0x02 /* Setting frame rate register */
#define AMG88XX_INTC		0x03 /* Setting interrupt control register */
#define AMG88XX_STAT		0x04 /* Status register */
#define AMG88XX_SCLR		0x05 /* Status clear register */
#define AMG88XX_AVE		0x07 /* Setting average register */
#define AMG88XX_INTHL		0x08 /* Interrupt level upper limit [7:0] */
#define AMG88XX_INTHH		0x09 /* Interrupt level upper limit [11:8] */
#define AMG88XX_INTLL		0x0a /* Interrupt level lower limit [7:0] */
#define AMG88XX_INTLH		0x0b /* Interrupt level lower limit [11:8] */
#define AMG88XX_INTSL		0x0c /* Interrupt hysteresis level [7:0] */
#define AMG88XX_INTSH		0x0d /* Interrupt hysteresis level [11:8] */
#define AMG88XX_TTHL		0x0e /* Thermistor temperature data [7:0] */
#define AMG88XX_TTHH		0x0f /* Thermistor temperature data [10:8] */
#define AMG88XX_INT0		0x10 /* Pixel 1..8 Interrupt Result */
#define AMG88XX_INT1		0x11 /* Pixel 9..16 Interrupt Result */
#define AMG88XX_INT2		0x12 /* Pixel 17..24 Interrupt Result */
#define AMG88XX_INT3		0x13 /* Pixel 25..32 Interrupt Result */
#define AMG88XX_INT4		0x14 /* Pixel 33..40 Interrupt Result */
#define AMG88XX_INT5		0x15 /* Pixel 41..48 Interrupt Result */
#define AMG88XX_INT6		0x16 /* Pixel 49..56 Interrupt Result */
#define AMG88XX_INT7		0x17 /* Pixel 57..64 Interrupt Result */

#define AMG88XX_OUTPUT_BASE	0x80 /* Base address for the output values */

#define AMG88XX_PCLT_NORMAL_MODE	0x00
#define AMG88XX_PCLT_SLEEP_MODE		0x10
#define AMG88XX_PCLT_STAND_BY_60S_MODE	0x20
#define AMG88XX_PCLT_STAND_BY_10S_MODE	0x21

#define AMG88XX_RST_FLAG_RST		0x30
#define AMG88XX_RST_INITIAL_RST		0x3F

#define AMG88XX_FPSC_10FPS		0x00
#define AMG88XX_FPSC_1FPS		0x01

#define AMG88XX_INTC_DISABLED		0x00
#define AMG88XX_INTC_DIFF_MODE		0x01
#define AMG88XX_INTC_ABS_MODE		0x03

#define AMG88XX_STAT_INTF_MASK		0x02
#define AMG88XX_STAT_OVF_IRS_MASK	0x04

#define AMG88XX_SCLR_INTCLR_MASK	0x02
#define AMG88XX_SCLR_OVS_CLR_MASK	0x04

#define AMG88XX_AVE_MAMOD_MASK		0x20

/* 1 LSB is equivalent to 0.25 degree Celsius scaled to micro degrees */
#define AMG88XX_TREG_LSB_SCALING	250000

#define AMG88XX_WAIT_MODE_CHANGE_US	50000
#define AMG88XX_WAIT_INITIAL_RESET_US	2000

struct amg88xx_config {
	const struct i2c_dt_spec i2c;
#ifdef CONFIG_AMG88XX_TRIGGER
	const struct gpio_dt_spec int_gpio;
#endif
};

struct amg88xx_data {
	int16_t sample[64];

#ifdef CONFIG_AMG88XX_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t drdy_handler;
	struct sensor_trigger drdy_trigger;

	sensor_trigger_handler_t th_handler;
	struct sensor_trigger th_trigger;

#if defined(CONFIG_AMG88XX_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_AMG88XX_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_AMG88XX_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_AMG88XX_TRIGGER */
};

#ifdef CONFIG_AMG88XX_TRIGGER
int amg88xx_attr_set(const struct device *dev,
		     enum sensor_channel chan,
		     enum sensor_attribute attr,
		     const struct sensor_value *val);

int amg88xx_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int amg88xx_init_interrupt(const struct device *dev);
#endif /* CONFIG_AMG88XX_TRIGGER */

#endif
