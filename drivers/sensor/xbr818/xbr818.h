/*
 * Copyright (c) 2024 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_XBR818_XBR818_H_
#define ZEPHYR_DRIVERS_SENSOR_XBR818_XBR818_H_

#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

/* 32Khz clockrate, most time values are multiple of this */
#define SENSOR_XBR818_CLOCKRATE 32000

struct xbr818_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec i2c_en;
	struct gpio_dt_spec io_val;
};

struct xbr818_data {
	bool value;
	uint32_t trigger_type;
	sensor_trigger_handler_t handler;
	struct gpio_callback gpio_cb;
	const struct sensor_trigger *trigger;
	const struct device *dev;
	struct k_work work;
};

/* reference rd-04 module manual for more information */
/* [0-2]: power of PA
 * [4-6]: mixer trim
 */
#define XBR818_RF_POWER            0x03
#define XBR818_RF_EN_SEL           0x04
/* minimum value of 2 */
#define XBR818_SAMPLE_RATE_DIVIDER 0x10
/* [0]: enable detection
 * [1-2]: readable data. 0: det_dc_sum 1: det_ac_sum 2: det_dc_used 3: det_noise
 * [3]: enable read on 0x28-0x29
 * [4]: signal detection threshold. 0: auto by pin 1: register
 * [7]: enable read on 0x26-0x29
 */
#define XBR818_I2C_OUT             0x13
/* Threshold for detection
 * [0-7]
 */
#define XBR818_THRESHOLD_1         0x18
/* [8-15] */
#define XBR818_THRESHOLD_2         0x19
/* Threshold for noise
 * [0-7]
 */
#define XBR818_THRESHOLD_NOISE_1   0x1A
/* [8-15] */
#define XBR818_THRESHOLD_NOISE_2   0x1B
/* Delay Time (in 1/32000 seconds)
 * [0-7]
 */
#define XBR818_DELAY_TIME_1        0x1D
/* [8-15] */
#define XBR818_DELAY_TIME_2        0x1E
/* [16-23] */
#define XBR818_DELAY_TIME_3        0x1F
/* [0]: enable
 * [1-2]: light sensor timer. 0: disabled 1: 4 sec 2: 1 minute 3: 1 hour
 * [3-4]: output timer. 0: 1 sec 1: 1 minute 2: 1 hour 3: 1 day
 * [5]: delay time. 0: 'configure by pin' 1: configure by register
 */
#define XBR818_TIMER_CTRL          0x1C
/* Lock Time (in 1/32000 seconds)
 * [0-7]
 */
#define XBR818_LOCK_TIME_1         0x20
/* [8-15] */
#define XBR818_LOCK_TIME_2         0x21
/* [16-23] */
#define XBR818_LOCK_TIME_3         0x22
/* Pin settings
 * [0-3]: IO_VAL pin
 * 0xc: io_value_out, 0xd: io_value_out inverted, 0xf: GPIO
 * [4-7]: INT_IRQ pin
 * 0x0: t3_int_irq, 0x9: io_value_out, 0xa: io_value_out inverted, 0xf: GPIO
 */
#define XBR818_PIN_SETTINGS        0x23
/* [0]: ADC1 is configured for VCO trimming. 0: enable, 1: disable
 * [1]: Low power mode is pin or register. 0: pin 1: register
 * [2]: If IO_VAL pin is GPIO, output. 0: no 1: yes
 * [3]: if INT_IRQ pin is GPIO, output. 0:no 1:yes
 */
#define XBR818_IO_ACTIVE_VALUE_REG 0x24

#endif /* ZEPHYR_DRIVERS_SENSOR_XBR818_XBR818_H_ */
