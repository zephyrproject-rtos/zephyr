/* Bosch bma530 3-axis accelerometer driver
 *
 * Copyright (c) 2024 Arrow Electronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMA530_H_
#define ZEPHYR_DRIVERS_SENSOR_BMA530_H_

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

/*
 * Register definitions
 */

#define BMA530_REG_CHIP_ID		(0x00)
#define BMA530_REG_HEALTH		(0x02)
#define BMA530_REG_CMD_SUSPEND		(0x04)
#define BMA530_REG_CONFIG_STATUS	(0x10)
#define BMA530_REG_SENSOR_STATUS	(0x11)
#define BMA530_REG_INT1_0_STATUS	(0x12)
#define BMA530_REG_INT1_1_STATUS	(0x13)
#define BMA530_REG_INT2_0_STATUS	(0x14)
#define BMA530_REG_INT2_1_STATUS	(0x15)
#define BMA530_REG_I3C_0_STATUS		(0x16)
#define BMA530_REG_I3C_1_STATUS		(0x17)
#define BMA530_REG_ACC_DATA_0		(0x18)
#define BMA530_REG_ACC_DATA_1		(0x19)
#define BMA530_REG_ACC_DATA_2		(0x1A)
#define BMA530_REG_ACC_DATA_3		(0x1B)
#define BMA530_REG_ACC_DATA_4		(0x1C)
#define BMA530_REG_ACC_DATA_5		(0x1D)
#define BMA530_REG_TEMP_DATA		(0x1E)
#define BMA530_REG_SENSORTIME_0		(0x1F)
#define BMA530_REG_SENSORTIME_1		(0x20)
#define BMA530_REG_SENSORTIME_2		(0x21)
#define BMA530_REG_FIFO_LEVEL_0		(0x22)
#define BMA530_REG_FIFO_LEVEL_1		(0x23)
#define BMA530_REG_FIFO_DATA_OUT	(0x24)
#define BMA530_REG_ACCEL_CONF_0		(0x30)
#define BMA530_REG_ACCEL_CONF_1		(0x31)
#define BMA530_REG_ACCEL_CONF_2		(0x32)
#define BMA530_REG_TEMP_CONF		(0x33)
#define BMA530_REG_INT1_CONF		(0x34)
#define BMA530_REG_INT2_CONF		(0x35)
#define BMA530_REG_INT_MAP_0		(0x36)
#define BMA530_REG_INT_MAP_1		(0x37)
#define BMA530_REG_INT_MAP_2		(0x38)
#define BMA530_REG_INT_MAP_3		(0x39)
#define BMA530_REG_IF_CONF_0		(0x3A)
#define BMA530_REG_IF_CONF_1		(0x3B)
#define BMA530_REG_FIFO_CONTROL		(0x40)
#define BMA530_REG_FIFO_CONFIG_0	(0x41)
#define BMA530_REG_FIFO_CONFIG_1	(0x42)
#define BMA530_REG_FIFO_WM_0		(0x43)
#define BMA530_REG_FIFO_WM_1		(0x44)
#define BMA530_REG_FEAT_ENG_CONF	(0x50)
#define BMA530_REG_FEAT_ENG_STATUS	(0x51)
#define BMA530_REG_FEAT_ENG_GP_FLAGS	(0x52)
#define BMA530_REG_FEAT_ENG_GPR_CONF	(0x53)
#define BMA530_REG_FEAT_ENG_GPR_CTRL	(0x54)
#define BMA530_REG_FEAT_ENG_GPR_0	(0x55)
#define BMA530_REG_FEAT_ENG_GPR_1	(0x56)
#define BMA530_REG_FEAT_ENG_GPR_2	(0x57)
#define BMA530_REG_FEAT_ENG_GPR_3	(0x58)
#define BMA530_REG_FEAT_ENG_GPR_4	(0x59)
#define BMA530_REG_FEAT_ENG_GPR_5	(0x5A)
#define BMA530_REG_FEATURE_DATA_ADDR	(0x5E)
#define BMA530_REG_FEATURE_DATA_TX	(0x5F)
#define BMA530_REG_ACC_OFFSET_0		(0x70)
#define BMA530_REG_ACC_OFFSET_1		(0x71)
#define BMA530_REG_ACC_OFFSET_2		(0x72)
#define BMA530_REG_ACC_OFFSET_3		(0x73)
#define BMA530_REG_ACC_OFFSET_4		(0x74)
#define BMA530_REG_ACC_OFFSET_5		(0x75)
#define BMA530_REG_ACC_SELF_TEST	(0x76)
#define BMA530_REG_CMD			(0x7E)

/*
 * BMA530 constants
 */
#define BMA530_CHIP_ID			(0xC2)
#define BMA530_ACC_CHANNEL_SIZE_BYTES	(2)
#define BMA530_ACC_CHANNEL_SIZE_BITS	(BMA530_ACC_CHANNEL_SIZE_BYTES * 8)
#define BMA530_PACKET_SIZE_ACC		(BMA530_REG_ACC_DATA_5 - BMA530_REG_ACC_DATA_0 + 1)
#define BMA530_PACKET_SIZE_TEMP		(1)
#define BMA530_PACKET_SIZE_ACC_TEMP	(BMA530_PACKET_SIZE_ACC + BMA530_PACKET_SIZE_TEMP)

#ifdef CONFIG_BMA530_TEMPERATURE
#define BMA530_PACKET_SIZE_MAX		(BMA530_PACKET_SIZE_ACC_TEMP)
#else
#define BMA530_PACKET_SIZE_MAX		(BMA530_PACKET_SIZE_ACC)
#endif /* CONFIG_BMA530_TEMPERATURE */

/* Value 0 in temperature register means 23 degrees C */
#define BMA530_TEMP_OFFSET		(23)

#define BMA530_REG_HEALTH_MASK		(0xF)
#define BMA530_HEALTH_OK		(0xF)
#define BMA530_HEALTH_CHECK_RETRIES	(100)

/*
 * Bit positions and masks
 */
#define BMA530_MASK_ACC_CONF_ODR	GENMASK(3, 0)
#define BMA530_MASK_ACC_RANGE		GENMASK(1, 0)
#define BMA530_BIT_ACC_PWR_MODE		BIT(7)
#define BMA530_SHIFT_ACC_PWR_MODE	(7)

/* Bandwidth parameters */
#define BMA530_POWER_MODE_LPM	(0x0)
#define BMA530_POWER_MODE_HPM	(0x1)

/* Full-scale ranges */
#define BMA530_RANGE_2G		(0x0)
#define BMA530_RANGE_4G		(0x1)
#define BMA530_RANGE_8G		(0x2)
#define BMA530_RANGE_16G	(0x3)

/* Output data rates (ODR) */
#define BMA530_ODR_RES_1_5625	(0x00)
#define BMA530_ODR_RES_3_125	(0x01)
#define BMA530_ODR_RES_6_25	(0x02)
#define BMA530_ODR_12_5		(0x03)
#define BMA530_ODR_25		(0x04)
#define BMA530_ODR_50		(0x05)
#define BMA530_ODR_100		(0x06)
#define BMA530_ODR_200		(0x07)
#define BMA530_ODR_400		(0x08)
#define BMA530_ODR_800_RES	(0x09)
#define BMA530_ODR_1600_RES	(0x0A)
#define BMA530_ODR_3200_RES	(0x0B)
#define BMA530_ODR_6400_RES	(0x0C)

/* Available ODR rates are different in different power modes. */
#define BMA530_ODR_MIN_HPM	BMA530_ODR_12_5
#define BMA530_ODR_MAX_HPM	BMA530_ODR_6400_RES
#define BMA530_ODR_MAX_LPM	BMA530_ODR_400

/*
 * BMA530 commands
 */

#define BMA530_CMD_SOFT_RESET	(0xB6)

/*
 * Other constants
 */

/* In offset registers (0x70-0x75) LSB is 0.98 [mG] or 980 [uG] */
#define BMA530_OFFSET_MICROG_PER_BIT	(980)

/* Offsets are 9-bit wide */
#define INT9_MIN			(-(1 << (9-1)))
#define INT9_MAX			((1 << (9-1)) - 1)
#define BMA530_OFFSET_MICROG_MIN	(INT9_MIN * BMA530_OFFSET_MICROG_PER_BIT)
#define BMA530_OFFSET_MICROG_MAX	(INT9_MAX * BMA530_OFFSET_MICROG_PER_BIT)

/*
 * Types
 */

union bma530_bus_cfg {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
};

struct bma530_config {
	int (*bus_init)(const struct device *dev);
	const union bma530_bus_cfg bus_cfg;
	int16_t full_scale_range;
	uint8_t accel_odr;
	uint8_t power_mode;
};

/**
 * Used to implement bus-specific R/W operations. See bma530_i2c.c and bma530_spi.c.
 */
struct bma530_hw_operations {
	int (*read_data)(const struct device *dev, uint8_t reg_addr, uint8_t *value, uint8_t len);
	int (*write_data)(const struct device *dev, uint8_t reg_addr, uint8_t *value, uint8_t len);
	int (*read_reg)(const struct device *dev, uint8_t reg_addr, uint8_t *value);
	int (*update_reg)(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t value);
};

struct bma530_data {
	int16_t x;
	int16_t y;
	int16_t z;
	/** Current full-scale range setting (in micro g's) as a register value */
	uint32_t accel_fs_range;
	/** Current output data rate as a register value */
	uint8_t accel_odr;
	/** Pointer to bus-specific I/O API */
	const struct bma530_hw_operations *hw_ops;
	/** High or low power mode */
	bool high_power_mode;
#ifdef CONFIG_BMA530_TEMPERATURE
	/** Accelerometer die temperature */
	int8_t temp;
#endif /* CONFIG_BMA530_TEMPERATURE */
};

int bma530_spi_init(const struct device *dev);
int bma530_i2c_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA530_H_ */
