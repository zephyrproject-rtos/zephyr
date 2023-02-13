/* Würth Elektronic WSEN-ITDS 3-axis Accel sensor driver
 *
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ITDS_H_
#define ZEPHYR_DRIVERS_SENSOR_ITDS_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>

/* registers */
#define ITDS_REG_TEMP_L			0x0d
#define ITDS_REG_DEV_ID			0x0f
#define ITDS_REG_CTRL1			0x20
#define ITDS_REG_CTRL2			0x21
#define ITDS_REG_CTRL3			0x22
#define ITDS_REG_CTRL4			0x23
#define ITDS_REG_CTRL5			0x24
#define ITDS_REG_CTRL6			0x25
#define ITDS_REG_STATUS			0x27
#define ITDS_REG_X_OUT_L		0x28
#define ITDS_REG_Y_OUT_L		0x2a
#define ITDS_REG_Z_OUT_L		0x2c
#define ITDS_REG_FIFO_CTRL		0x2e
#define ITDS_REG_FIFO_SAMPLES		0x2f
#define ITDS_REG_STATUS_DETECT		0x37
#define ITDS_REG_WAKEUP_EVENT		0x38
#define ITDS_REG_CTRL7			0x3f

/* bitfields */
#define ITDS_MASK_SCALE			GENMASK(5, 4)
#define ITDS_MASK_BDU_INC_ADD		GENMASK(3, 2)
#define ITDS_MASK_FIFOTH		GENMASK(4, 0)
#define ITDS_MASK_FIFOMODE		GENMASK(7, 5)
#define ITDS_MASK_MODE			GENMASK(3, 0)
#define ITDS_MASK_SAMPLES_COUNT		GENMASK(5, 0)
#define ITDS_MASK_ODR			GENMASK(7, 4)
#define ITDS_MASK_INT_DRDY		BIT(0)
#define ITDS_MASK_INT_FIFOTH		BIT(1)
#define ITDS_MASK_INT_EN		BIT(5)

#define ITDS_EVENT_DRDY			BIT(0)
#define ITDS_EVENT_DRDY_T		BIT(6)
#define ITDS_EVENT_FIFO_TH		BIT(7)
#define ITDS_FIFO_MODE_BYPASS		0
#define ITDS_FIFO_MODE_FIFO		BIT(5)
#define ITDS_DEVICE_ID			0x44
#define ITDS_ACCL_FIFO_SIZE		32
#define ITDS_TEMP_OFFSET		25

enum operation_mode {
	ITDS_OP_MODE_LOW_POWER	= BIT(0),
	ITDS_OP_MODE_NORMAL	= BIT(1),
	ITDS_OP_MODE_HIGH_PERF	= BIT(2),
};

enum itds_accel_range_const {
	ITDS_ACCL_RANGE_2G,
	ITDS_ACCL_RANGE_4G,
	ITDS_ACCL_RANGE_8G,
	ITDS_ACCL_RANGE_16G,
	ITDS_ACCL_RANGE_END
};

enum itds_odr_const {
	ITDS_ODR_0,
	ITDS_ODR_1_6,
	ITDS_ODR_12_5,
	ITDS_ODR_25,
	ITDS_ODR_50,
	ITDS_ODR_100,
	ITDS_ODR_200,
	ITDS_ODR_400,
	ITDS_ODR_800,
	ITDS_ODR_1600,
	ITDS_ODR_MAX
};

struct itds_odr {
	uint16_t freq;
	uint16_t mfreq;
};

struct itds_accel_range {
	uint16_t range;
	uint8_t reg_val;
};

struct itds_device_config {
	struct i2c_dt_spec i2c;
#ifdef	CONFIG_ITDS_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif
	int def_odr;
	int def_op_mode;
};

#define ITDS_SAMPLE_SIZE	3
struct itds_device_data {
#ifdef	CONFIG_ITDS_TRIGGER
	struct gpio_callback gpio_cb;
	struct k_work work;
#endif
	int16_t samples[ITDS_SAMPLE_SIZE];
	int16_t temprature;
	uint16_t scale;
	enum operation_mode op_mode;
	const struct device *dev;

#ifdef CONFIG_ITDS_TRIGGER
	sensor_trigger_handler_t handler_drdy;
#endif /* CONFIG_ITDS_TRIGGER */
};

int itds_trigger_mode_init(const struct device *dev);
int itds_trigger_set(const struct device *dev,
		     const struct sensor_trigger *trig,
		     sensor_trigger_handler_t handler);

#endif /* ZEPHYR_DRIVERS_SENSOR_ITDS_H_*/
