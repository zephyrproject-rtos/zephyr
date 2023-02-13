/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS3MDL_LIS3MDL_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS3MDL_LIS3MDL_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define LIS3MDL_REG_WHO_AM_I            0x0F
#define LIS3MDL_CHIP_ID                 0x3D

#define LIS3MDL_REG_CTRL1               0x20
#define LIS3MDL_TEMP_EN_MASK            BIT(7)
#define LIS3MDL_TEMP_EN_SHIFT           7
#define LIS3MDL_OM_MASK                 (BIT(6) | BIT(5))
#define LIS3MDL_OM_SHIFT                5
#define LIS3MDL_MAG_DO_MASK             (BIT(4) | BIT(3) | BIT(2))
#define LIS3MDL_DO_SHIFT                2
#define LIS3MDL_FAST_ODR_MASK           BIT(1)
#define LIS3MDL_FAST_ODR_SHIFT          1
#define LIS3MDL_ST_MASK                 BIT(0)
#define LIS3MDL_ST_SHIFT                0

#define LIS3MDL_ODR_BITS(om_bits, do_bits, fast_odr)	\
	(((om_bits) << LIS3MDL_OM_SHIFT) |		\
	 ((do_bits) << LIS3MDL_DO_SHIFT) |		\
	 ((fast_odr) << LIS3MDL_FAST_ODR_SHIFT))

#define LIS3MDL_REG_CTRL2               0x21
#define LIS3MDL_FS_MASK                 (BIT(6) | BIT(5))
#define LIS3MDL_FS_SHIFT                5
#define LIS3MDL_REBOOT_MASK             BIT(3)
#define LIS3MDL_REBOOT_SHIFT            3
#define LIS3MDL_SOFT_RST_MASK           BIT(2)
#define LIS3MDL_SOFT_RST_SHIFT          2

#define LIS3MDL_FS_IDX                  ((CONFIG_LIS3MDL_FS / 4) - 1)

/* guard against invalid CONFIG_LIS3MDL_FS values */
#if CONFIG_LIS3MDL_FS % 4 != 0 || LIS3MDL_FS_IDX < -1 || LIS3MDL_FS_IDX >= 4
#error "Invalid value for CONFIG_LIS3MDL_FS"
#endif

#define LIS3MDL_REG_CTRL3               0x22
#define LIS3MDL_LP_MASK                 BIT(5)
#define LIS3MDL_LP_SHIFT                5
#define LIS3MDL_SIM_MASK                BIT(2)
#define LIS3MDL_SIM_SHIFT               2
#define LIS3MDL_MD_MASK                 (BIT(1) | BIT(0))
#define LIS3MDL_MD_SHIFT                0

#define LIS3MDL_MD_CONTINUOUS           0
#define LIS3MDL_MD_SINGLE               1
#define LIS3MDL_MD_POWER_DOWN           2
#define LIS3MDL_MD_POWER_DOWN_AUTO      3

#define LIS3MDL_REG_CTRL4               0x23
#define LIS3MDL_OMZ_MASK                (BIT(3) | BIT(2))
#define LIS3MDL_OMZ_SHIFT               2
#define LIS3MDL_BLE_MASK                BIT(1)
#define LIS3MDL_BLE_SHIFT               1

#define LIS3MDL_REG_CTRL5               0x24
#define LIS3MDL_FAST_READ_MASK          BIT(7)
#define LIS3MDL_FAST_READ_SHIFT         7
#define LIS3MDL_BDU_MASK                BIT(6)
#define LIS3MDL_BDU_SHIFT               6

#define LIS3MDL_BDU_EN                  (1 << LIS3MDL_BDU_SHIFT)

#define LIS3MDL_REG_SAMPLE_START        0x28

#define LIS3MDL_REG_INT_CFG             0x30
#define LIS3MDL_INT_X_EN                BIT(7)
#define LIS3MDL_INT_Y_EN                BIT(6)
#define LIS3MDL_INT_Z_EN                BIT(5)
#define LIS3MDL_INT_XYZ_EN              \
	(LIS3MDL_INT_X_EN | LIS3MDL_INT_Y_EN | LIS3MDL_INT_Z_EN)

static const char * const lis3mdl_odr_strings[] = {
	"0.625", "1.25", "2.5", "5", "10", "20",
	"40", "80", "155", "300", "560", "1000"
};

static const uint8_t lis3mdl_odr_bits[] = {
	LIS3MDL_ODR_BITS(0, 0, 0), /* 0.625 Hz */
	LIS3MDL_ODR_BITS(0, 1, 0), /* 1.25 Hz */
	LIS3MDL_ODR_BITS(0, 2, 0), /* 2.5 Hz */
	LIS3MDL_ODR_BITS(0, 3, 0), /* 5 Hz */
	LIS3MDL_ODR_BITS(0, 4, 0), /* 10 Hz */
	LIS3MDL_ODR_BITS(0, 5, 0), /* 20 Hz */
	LIS3MDL_ODR_BITS(0, 6, 0), /* 40 Hz */
	LIS3MDL_ODR_BITS(0, 7, 0), /* 80 Hz */
	LIS3MDL_ODR_BITS(3, 0, 1), /* 155 Hz */
	LIS3MDL_ODR_BITS(2, 0, 1), /* 300 Hz */
	LIS3MDL_ODR_BITS(1, 0, 1), /* 560 Hz */
	LIS3MDL_ODR_BITS(0, 0, 1)  /* 1000 Hz */
};

static const uint16_t lis3mdl_magn_gain[] = {
	6842, 3421, 2281, 1711
};

struct lis3mdl_data {
	int16_t x_sample;
	int16_t y_sample;
	int16_t z_sample;
	int16_t temp_sample;

#ifdef CONFIG_LIS3MDL_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_LIS3MDL_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LIS3MDL_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_LIS3MDL_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_LIS3MDL_TRIGGER */
};

struct lis3mdl_config {
	struct i2c_dt_spec i2c;
#ifdef CONFIG_LIS3MDL_TRIGGER
	struct gpio_dt_spec irq_gpio;
#endif
};

#ifdef CONFIG_LIS3MDL_TRIGGER
int lis3mdl_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int lis3mdl_sample_fetch(const struct device *dev, enum sensor_channel chan);

int lis3mdl_init_interrupt(const struct device *dev);
#endif

#endif /* __SENSOR_LIS3MDL__ */
