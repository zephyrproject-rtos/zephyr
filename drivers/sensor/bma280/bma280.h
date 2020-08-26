/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMA280_BMA280_H_
#define ZEPHYR_DRIVERS_SENSOR_BMA280_BMA280_H_

#include <device.h>
#include <sys/util.h>
#include <zephyr/types.h>
#include <drivers/gpio.h>

#define BMA280_I2C_ADDRESS		DT_INST_REG_ADDR(0)

#define BMA280_REG_CHIP_ID		0x00
#if DT_INST_PROP(0, is_bmc150)
	#define BMA280_CHIP_ID		0xFA
#else
	#define BMA280_CHIP_ID		0xFB
#endif

#define BMA280_REG_PMU_BW		0x10
#if CONFIG_BMA280_PMU_BW_1
	#define BMA280_PMU_BW		0x08
#elif CONFIG_BMA280_PMU_BW_2
	#define BMA280_PMU_BW		0x09
#elif CONFIG_BMA280_PMU_BW_3
	#define BMA280_PMU_BW		0x0A
#elif CONFIG_BMA280_PMU_BW_4
	#define BMA280_PMU_BW		0x0B
#elif CONFIG_BMA280_PMU_BW_5
	#define BMA280_PMU_BW		0x0C
#elif CONFIG_BMA280_PMU_BW_6
	#define BMA280_PMU_BW		0x0D
#elif CONFIG_BMA280_PMU_BW_7
	#define BMA280_PMU_BW		0x0E
#elif CONFIG_BMA280_PMU_BW_8
	#define BMA280_PMU_BW		0x0F
#endif

/*
 * BMA280_PMU_FULL_RANGE measured in mili-m/s^2 instead
 * of m/s^2 to avoid using struct sensor_value for it
 */
#define BMA280_REG_PMU_RANGE		0x0F
#if CONFIG_BMA280_PMU_RANGE_2G
	#define BMA280_PMU_RANGE	0x03
	#define BMA280_PMU_FULL_RANGE	(4 * SENSOR_G)
#elif CONFIG_BMA280_PMU_RANGE_4G
	#define BMA280_PMU_RANGE	0x05
	#define BMA280_PMU_FULL_RANGE	(8 * SENSOR_G)
#elif CONFIG_BMA280_PMU_RANGE_8G
	#define BMA280_PMU_RANGE	0x08
	#define BMA280_PMU_FULL_RANGE	(16 * SENSOR_G)
#elif CONFIG_BMA280_PMU_RANGE_16G
	#define BMA280_PMU_RANGE	0x0C
	#define BMA280_PMU_FULL_RANGE	(32 * SENSOR_G)
#endif

#define BMA280_REG_TEMP			0x08

#define BMA280_REG_INT_STATUS_0		0x09
#define BMA280_BIT_SLOPE_INT_STATUS	BIT(2)
#define BMA280_REG_INT_STATUS_1		0x0A
#define BMA280_BIT_DATA_INT_STATUS	BIT(7)

#define BMA280_REG_INT_EN_0		0x16
#define BMA280_BIT_SLOPE_EN_X		BIT(0)
#define BMA280_BIT_SLOPE_EN_Y		BIT(1)
#define BMA280_BIT_SLOPE_EN_Z		BIT(2)
#define BMA280_SLOPE_EN_XYZ (BMA280_BIT_SLOPE_EN_X | \
		BMA280_BIT_SLOPE_EN_Y | BMA280_BIT_SLOPE_EN_X)

#define BMA280_REG_INT_EN_1		0x17
#define BMA280_BIT_DATA_EN		BIT(4)

#define BMA280_REG_INT_MAP_0		0x19
#define BMA280_INT_MAP_0_BIT_SLOPE	BIT(2)

#define BMA280_REG_INT_MAP_1		0x1A
#define BMA280_INT_MAP_1_BIT_DATA	BIT(0)

#define BMA280_REG_INT_RST_LATCH	0x21
#define BMA280_INT_MODE_LATCH		0x0F
#define BMA280_BIT_INT_LATCH_RESET	BIT(7)

#define BMA280_REG_INT_5		0x27
#define BMA280_SLOPE_DUR_SHIFT		0
#define BMA280_SLOPE_DUR_MASK		(3 << BMA280_SLOPE_DUR_SHIFT)

#define BMA280_REG_SLOPE_TH		0x28

#define BMA280_REG_ACCEL_X_LSB		0x2
#define BMA280_REG_ACCEL_Y_LSB		0x4
#define BMA280_REG_ACCEL_Z_LSB		0x6

#if DT_INST_PROP(0, is_bmc150)
	#define BMA280_ACCEL_LSB_BITS	4
	#define BMA280_ACCEL_LSB_SHIFT	4
#else
	#define BMA280_ACCEL_LSB_BITS	6
	#define BMA280_ACCEL_LSB_SHIFT	2
#endif
#define BMA280_ACCEL_LSB_MASK		\
		(BIT_MASK(BMA280_ACCEL_LSB_BITS) << BMA280_ACCEL_LSB_SHIFT)

#define BMA280_REG_ACCEL_X_MSB		0x3
#define BMA280_REG_ACCEL_Y_MSB		0x5
#define BMA280_REG_ACCEL_Z_MSB		0x7

#define BMA280_THREAD_PRIORITY		10
#define BMA280_THREAD_STACKSIZE_UNIT	1024

struct bma280_data {
	struct device *i2c;
	int16_t x_sample;
	int16_t y_sample;
	int16_t z_sample;
	int8_t temp_sample;

#ifdef CONFIG_BMA280_TRIGGER
	struct device *dev;
	struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

	struct sensor_trigger any_motion_trigger;
	sensor_trigger_handler_t any_motion_handler;

#if defined(CONFIG_BMA280_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_BMA280_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_BMA280_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_BMA280_TRIGGER */
};

#ifdef CONFIG_BMA280_TRIGGER
int bma280_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int bma280_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val);

int bma280_init_interrupt(struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA280_BMA280_H_ */
