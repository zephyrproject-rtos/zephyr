/*
 * Copyright (c) 2020 Intive
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ISL29125_H
#define ZEPHYR_DRIVERS_SENSOR_ISL29125_H

#include <device.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>

/* ISL29125 I2C Address (usually 0x44) */
#define ISL29125_I2C_ADDR DT_INST_0_ISIL_ISL29125_BASE_ADDRESS

/* ISL29125 Registers */
#define ISL29125_DEVICE_ID 0x00
#define ISL29125_CONFIG_1 0x01
#define ISL29125_CONFIG_2 0x02
#define ISL29125_CONFIG_3 0x03
#define ISL29125_THRESHOLD_LL 0x04
#define ISL29125_THRESHOLD_LH 0x05
#define ISL29125_THRESHOLD_HL 0x06
#define ISL29125_THRESHOLD_HH 0x07
#define ISL29125_STATUS 0x08
#define ISL29125_GREEN_L 0x09
#define ISL29125_GREEN_H 0x0A
#define ISL29125_RED_L 0x0B
#define ISL29125_RED_H 0x0C
#define ISL29125_BLUE_L 0x0D
#define ISL29125_BLUE_H 0x0E

/* Configuration Settings */
#define ISL29125_CFG_DEFAULT 0x00

/* CONFIG1
 * Pick a mode, determines what color[s] the sensor samples, if any
 */
#define ISL29125_CFG1_MODE_POWERDOWN 0x00
#define ISL29125_CFG1_MODE_G 0x01
#define ISL29125_CFG1_MODE_R 0x02
#define ISL29125_CFG1_MODE_B 0x03
#define ISL29125_CFG1_MODE_STANDBY 0x04
#define ISL29125_CFG1_MODE_RGB 0x05
#define ISL29125_CFG1_MODE_RG 0x06
#define ISL29125_CFG1_MODE_GB 0x07

/* Light intensity range
 * In a dark environment 375Lux is best, otherwise 10KLux is likely the best
 * option
 */
#define ISL29125_CFG1_375LUX 0x00
#define ISL29125_CFG1_10KLUX 0x08

/* Change this to 12 bit if you want less accuracy, but faster sensor reads
 * At default 16 bit, each sensor sample for a given color is about ~100ms
 */
#define ISL29125_CFG1_16BIT 0x00
#define ISL29125_CFG1_12BIT 0x10

/* Unless you want the interrupt pin to be an input that triggers sensor
 * sampling, leave this on normal
 */
#define ISL29125_CFG1_ADC_SYNC_NORMAL 0x00
#define ISL29125_CFG1_ADC_SYNC_TO_INT 0x20

/* CONFIG2
 * Selects upper or lower range of IR filtering
 */
#define ISL29125_CFG2_IR_OFFSET_OFF 0x00
#define ISL29125_CFG2_IR_OFFSET_ON 0x80

/* Sets amount of IR filtering, can use these presets or any value between
 * 0x00 and 0x3F. Consult datasheet for detailed IR filtering calibration
 */
#define ISL29125_CFG2_IR_ADJUST_LOW 0x00
#define ISL29125_CFG2_IR_ADJUST_MID 0x20
#define ISL29125_CFG2_IR_ADJUST_HIGH 0x3F

/* CONFIG3
 * No interrupts, or interrupts based on a selected color
 */
#define ISL29125_CFG3_NO_INT 0x00
#define ISL29125_CFG3_G_INT 0x01
#define ISL29125_CFG3_R_INT 0x02
#define ISL29125_CFG3_B_INT 0x03
#define ISL29125_CFG3_TH_IRQ_MASK 0x03

/* How many times a sensor sample must hit a threshold before triggering an
 * interrupt. More consecutive samples means more times between interrupts,
 * but less triggers from short transients
 */
#define ISL29125_CFG3_INT_PRST1 0x00
#define ISL29125_CFG3_INT_PRST2 0x04
#define ISL29125_CFG3_INT_PRST4 0x08
#define ISL29125_CFG3_INT_PRST8 0x0C
#define ISL29125_CFG3_INT_MASK  0x0C

/* If you would rather have interrupts trigger when a sensor sampling is
 * complete, enable this. If this is disabled, interrupts are based on comparing
 * sensor data to threshold settings
 */
#define ISL29125_CFG3_RGB_CONV_TO_INT_DISABLE 0x00
#define ISL29125_CFG3_RGB_CONV_TO_INT_ENABLE 0x10

/* STATUS FLAG MASKS */
#define ISL29125_FLAG_INT 0x01
#define ISL29125_FLAG_CONV_DONE 0x02
#define ISL29125_FLAG_BROWNOUT 0x04
#define ISL29125_FLAG_CONV_G 0x10
#define ISL29125_FLAG_CONV_R 0x20
#define ISL29125_FLAG_CONV_B 0x30

struct isl29125_data {
	struct device *i2c;
	u8_t dev_config_1;
	u8_t dev_config_2;
	u8_t dev_config_3;
	u16_t r;
	u16_t g;
	u16_t b;
#if CONFIG_ISL29125_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_callback;

	struct sensor_trigger trigger;
	sensor_trigger_handler_t handler;

#if defined(CONFIG_ISL29125_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_ISL29125_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_ISL29125_TRIGGER_GLOBAL_THREAD)
	struct k_work work_item;
	struct device *device;
#endif

#endif  /* CONFIG_ISL29125_TRIGGER */
};

#ifdef CONFIG_ISL29125_TRIGGER
int isl29125_attr_set(struct device *dev, enum sensor_channel chan,
		      enum sensor_attribute attr,
		      const struct sensor_value *val);

int isl29125_trigger_set(struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int isl29125_init_interrupt(struct device *dev);
#endif
int isl29125_set_config(struct isl29125_data *drv_data);
#endif /* ZEPHYR_DRIVERS_SENSOR_ISL29125_H */
