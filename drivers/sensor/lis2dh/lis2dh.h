/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS2DH_LIS2DH_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS2DH_LIS2DH_H_

#include <kernel.h>
#include <device.h>
#include <misc/util.h>
#include <stdint.h>
#include <gpio.h>
#include <sensor.h>
#include <string.h>

#if defined(CONFIG_LIS2DH_BUS_SPI)
#include <spi.h>

#define LIS2DH_BUS_ADDRESS		CONFIG_LIS2DH_SPI_SS_1

#define LIS2DH_SPI_READ_BIT		BIT(7)
#define LIS2DH_SPI_AUTOINC_ADDR		BIT(6)
#define LIS2DH_SPI_ADDR_MASK		BIT_MASK(6)

/* LIS2DH supports only SPI mode 0, word size 8 bits, MSB first */
#define LIS2DH_SPI_CFG			SPI_WORD_SET(8)

#define LIS2DH_BUS_DEV_NAME		CONFIG_LIS2DH_SPI_MASTER_DEV_NAME

#elif defined(CONFIG_LIS2DH_BUS_I2C)
#include <i2c.h>

#define LIS2DH_BUS_ADDRESS		CONFIG_LIS2DH_I2C_ADDR

#define LIS2DH_BUS_DEV_NAME		CONFIG_LIS2DH_I2C_MASTER_DEV_NAME

#endif

#define LIS2DH_AUTOINCREMENT_ADDR	BIT(7)

#define LIS2DH_REG_CTRL1		0x20
#define LIS2DH_ACCEL_XYZ_SHIFT		0
#define LIS2DH_ACCEL_X_EN_BIT		BIT(0)
#define LIS2DH_ACCEL_Y_EN_BIT		BIT(1)
#define LIS2DH_ACCEL_Z_EN_BIT		BIT(2)
#define LIS2DH_ACCEL_EN_BITS		(LIS2DH_ACCEL_X_EN_BIT | \
					LIS2DH_ACCEL_Y_EN_BIT | \
					LIS2DH_ACCEL_Z_EN_BIT)
#define LIS2DH_ACCEL_XYZ_MASK		BIT_MASK(3)

#if defined(CONFIG_LIS2DH_POWER_MODE_LOW)
	#define LIS2DH_LP_EN_BIT	BIT(3)
#elif defined(CONFIG_LIS2DH_POWER_MODE_NORMAL)
	#define LIS2DH_LP_EN_BIT	0
#endif

#define LIS2DH_ODR_1			1
#define LIS2DH_ODR_2			2
#define LIS2DH_ODR_3			3
#define LIS2DH_ODR_4			4
#define LIS2DH_ODR_5			5
#define LIS2DH_ODR_6			6
#define LIS2DH_ODR_7			7
#define LIS2DH_ODR_8			8
#define LIS2DH_ODR_9			9

#if defined(CONFIG_LIS2DH_ODR_1)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_1
#elif defined(CONFIG_LIS2DH_ODR_2)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_2
#elif defined(CONFIG_LIS2DH_ODR_3)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_3
#elif defined(CONFIG_LIS2DH_ODR_4) || defined(CONFIG_LIS2DH_ODR_RUNTIME)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_4
#elif defined(CONFIG_LIS2DH_ODR_5)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_5
#elif defined(CONFIG_LIS2DH_ODR_6)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_6
#elif defined(CONFIG_LIS2DH_ODR_7)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_7
#elif defined(CONFIG_LIS2DH_ODR_8)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_8
#elif defined(CONFIG_LIS2DH_ODR_9_NORMAL) || defined(CONFIG_LIS2DH_ODR_9_LOW)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_9
#endif

#define LIS2DH_ODR_SHIFT		4
#define LIS2DH_ODR_RATE(r)		((r) << LIS2DH_ODR_SHIFT)
#define LIS2DH_ODR_BITS			(LIS2DH_ODR_RATE(LIS2DH_ODR_IDX))
#define LIS2DH_ODR_MASK			(BIT_MASK(4) << LIS2DH_ODR_SHIFT)

#define LIS2DH_REG_CTRL2		0x21
#define LIS2DH_HPIS2_EN_BIT		BIT(1)
#define LIS2DH_FDS_EN_BIT		BIT(3)

#define LIS2DH_REG_CTRL3		0x22
#define LIS2DH_EN_DRDY1_INT1_SHIFT	4
#define LIS2DH_EN_DRDY1_INT1		BIT(LIS2DH_EN_DRDY1_INT1_SHIFT)

#define LIS2DH_REG_CTRL4		0x23
#define LIS2DH_FS_SHIFT			4
#define LIS2DH_FS_MASK			(BIT_MASK(2) << LIS2DH_FS_SHIFT)

#if defined(CONFIG_LIS2DH_ACCEL_RANGE_2G) ||\
	defined(CONFIG_LIS2DH_ACCEL_RANGE_RUNTIME)
	#define LIS2DH_FS_IDX		0
#elif defined(CONFIG_LIS2DH_ACCEL_RANGE_4G)
	#define LIS2DH_FS_IDX		1
#elif defined(CONFIG_LIS2DH_ACCEL_RANGE_8G)
	#define LIS2DH_FS_IDX		2
#elif defined(CONFIG_LIS2DH_ACCEL_RANGE_16G)
	#define LIS2DH_FS_IDX		3
#endif

#define LIS2DH_FS_SELECT(fs)		((fs) << LIS2DH_FS_SHIFT)
#define LIS2DH_FS_BITS			(LIS2DH_FS_SELECT(LIS2DH_FS_IDX))
#define LIS2DH_ACCEL_SCALE(range_g)	((SENSOR_G * 2 * (range_g)) / 65636LL)

#define LIS2DH_REG_CTRL5		0x24
#define LIS2DH_LIR_INT2_SHIFT		1
#define LIS2DH_EN_LIR_INT2		BIT(LIS2DH_LIR_INT2_SHIFT)

#define LIS2DH_REG_CTRL6		0x25
#define LIS2DH_EN_INT2_INT2_SHIFT	5
#define LIS2DH_EN_INT2_INT2		BIT(LIS2DH_EN_INT2_INT2_SHIFT)

#define LIS2DH_REG_REFERENCE		0x26

#define LIS2DH_REG_STATUS		0x27
#define LIS2DH_STATUS_ZYZ_OVR		BIT(7)
#define LIS2DH_STATUS_Z_OVR		BIT(6)
#define LIS2DH_STATUS_Y_OVR		BIT(5)
#define LIS2DH_STATUS_X_OVR		BIT(4)
#define LIS2DH_STATUS_OVR_MASK		(BIT_MASK(4) << 4)
#define LIS2DH_STATUS_ZYX_DRDY		BIT(3)
#define LIS2DH_STATUS_Z_DRDY		BIT(2)
#define LIS2DH_STATUS_Y_DRDY		BIT(1)
#define LIS2DH_STATUS_X_DRDY		BIT(0)
#define LIS2DH_STATUS_DRDY_MASK		BIT_MASK(4)

#define LIS2DH_REG_ACCEL_X_LSB		0x28
#define LIS2DH_REG_ACCEL_Y_LSB		0x2A
#define LIS2DH_REG_ACCEL_Z_LSB		0x2C
#define LIS2DH_REG_ACCEL_X_MSB		0x29
#define LIS2DH_REG_ACCEL_Y_MSB		0x2B
#define LIS2DH_REG_ACCEL_Z_MSB		0x2D

#define LIS2DH_REG_INT1_CFG		0x30
#define LIS2DH_REG_INT2_CFG		0x34
#define LIS2DH_AOI_CFG			BIT(7)
#define LIS2DH_INT_CFG_ZHIE_ZUPE	BIT(5)
#define LIS2DH_INT_CFG_ZLIE_ZDOWNE	BIT(4)
#define LIS2DH_INT_CFG_YHIE_YUPE	BIT(3)
#define LIS2DH_INT_CFG_YLIE_YDOWNE	BIT(2)
#define LIS2DH_INT_CFG_XHIE_XUPE	BIT(1)
#define LIS2DH_INT_CFG_XLIE_XDOWNE	BIT(0)

#define LIS2DH_REG_INT2_SRC		0x35

#define LIS2DH_REG_INT2_THS		0x36

#define LIS2DH_REG_INT2_DUR		0x37

/* sample buffer size includes status register */
#if defined(CONFIG_LIS2DH_BUS_SPI)
#define LIS2DH_BUF_SZ			8
#define LIS2DH_DATA_OFS			1
#else
#define LIS2DH_BUF_SZ			7
#define LIS2DH_DATA_OFS			0
#endif

union lis2dh_sample {
	u8_t raw[LIS2DH_BUF_SZ];
	struct {
#if defined(CONFIG_LIS2DH_BUS_SPI)
		u8_t dummy;
#endif
		u8_t status;
		s16_t xyz[3];
	} __packed;
};

struct lis2dh_data {
#if defined(CONFIG_LIS2DH_BUS_SPI)
	struct device *spi;
	struct spi_config spi_cfg;
#else
	struct device *bus;
#endif
	union lis2dh_sample sample;
	/* current scaling factor, in micro m/s^2 / lsb */
	u16_t scale;

#ifdef CONFIG_LIS2DH_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_int1_cb;
	struct gpio_callback gpio_int2_cb;

	sensor_trigger_handler_t handler_drdy;
	sensor_trigger_handler_t handler_anymotion;
	atomic_t trig_flags;
	enum sensor_channel chan_drdy;

#if defined(CONFIG_LIS2DH_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_LIS2DH_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LIS2DH_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_LIS2DH_TRIGGER */
};

#define SYS_LOG_DOMAIN "lis2dh"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

#if defined(CONFIG_LIS2DH_BUS_SPI)
int lis2dh_spi_access(struct lis2dh_data *ctx, u8_t cmd,
		      void *data, size_t length);
#endif

static inline int lis2dh_burst_read(struct device *dev, u8_t start_addr,
				    u8_t *buf, u8_t num_bytes)
{
	struct lis2dh_data *lis2dh = dev->driver_data;

#if defined(CONFIG_LIS2DH_BUS_SPI)
	start_addr |= LIS2DH_SPI_READ_BIT | LIS2DH_SPI_AUTOINC_ADDR;

	return lis2dh_spi_access(lis2dh, start_addr, buf, num_bytes);
#elif defined(CONFIG_LIS2DH_BUS_I2C)
	return i2c_burst_read(lis2dh->bus, LIS2DH_BUS_ADDRESS,
			      start_addr | LIS2DH_AUTOINCREMENT_ADDR,
			      buf, num_bytes);
#else
	return -ENODEV;
#endif
}

static inline int lis2dh_reg_read_byte(struct device *dev, u8_t reg_addr,
				       u8_t *value)
{
	struct lis2dh_data *lis2dh = dev->driver_data;

#if defined(CONFIG_LIS2DH_BUS_SPI)
	reg_addr |= LIS2DH_SPI_READ_BIT;

	return lis2dh_spi_access(lis2dh, reg_addr, value, 1);
#elif defined(CONFIG_LIS2DH_BUS_I2C)
	return i2c_reg_read_byte(lis2dh->bus, LIS2DH_BUS_ADDRESS,
				 reg_addr, value);
#else
	return -ENODEV;
#endif
}

static inline int lis2dh_burst_write(struct device *dev, u8_t start_addr,
				     u8_t *buf, u8_t num_bytes)
{
	struct lis2dh_data *lis2dh = dev->driver_data;

#if defined(CONFIG_LIS2DH_BUS_SPI)
	start_addr |= LIS2DH_SPI_AUTOINC_ADDR;

	return lis2dh_spi_access(lis2dh, start_addr, buf, num_bytes);
#elif defined(CONFIG_LIS2DH_BUS_I2C)
	return i2c_burst_write(lis2dh->bus, LIS2DH_BUS_ADDRESS,
			       start_addr | LIS2DH_AUTOINCREMENT_ADDR,
			       buf, num_bytes);
#else
	return -ENODEV;
#endif
}

static inline int lis2dh_reg_write_byte(struct device *dev, u8_t reg_addr,
					u8_t value)
{
	struct lis2dh_data *lis2dh = dev->driver_data;

#if defined(CONFIG_LIS2DH_BUS_SPI)
	reg_addr &= LIS2DH_SPI_ADDR_MASK;

	return lis2dh_spi_access(lis2dh, reg_addr, &value, 1);
#elif defined(CONFIG_LIS2DH_BUS_I2C)
	u8_t tx_buf[2] = {reg_addr, value};

	return i2c_write(lis2dh->bus, tx_buf, sizeof(tx_buf),
			 LIS2DH_BUS_ADDRESS);
#else
	return -ENODEV;
#endif
}

static inline int lis2dh_bus_configure(struct device *dev)
{
	struct lis2dh_data *lis2dh = dev->driver_data;

#if defined(CONFIG_LIS2DH_BUS_SPI)
	lis2dh->spi = device_get_binding(LIS2DH_BUS_DEV_NAME);
	if (lis2dh->spi == NULL) {
		SYS_LOG_ERR("Could not get pointer to %s device",
			    LIS2DH_BUS_DEV_NAME);
		return -EINVAL;
	}

	lis2dh->spi_cfg.operation = LIS2DH_SPI_CFG;
	lis2dh->spi_cfg.frequency = CONFIG_LIS2DH_SPI_FREQUENCY;
	lis2dh->spi_cfg.slave = LIS2DH_BUS_ADDRESS;

	return 0;
#elif defined(CONFIG_LIS2DH_BUS_I2C)
	lis2dh->bus = device_get_binding(LIS2DH_BUS_DEV_NAME);
	if (lis2dh->bus == NULL) {
		SYS_LOG_ERR("Could not get pointer to %s device",
			    LIS2DH_BUS_DEV_NAME);
		return -EINVAL;
	}

	return 0;
#else
	return -ENODEV;
#endif
}

#ifdef CONFIG_LIS2DH_TRIGGER
int lis2dh_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int lis2dh_init_interrupt(struct device *dev);

int lis2dh_reg_field_update(struct device *dev, u8_t reg_addr,
			    u8_t pos, u8_t mask, u8_t val);

int lis2dh_acc_slope_config(struct device *dev, enum sensor_attribute attr,
			    const struct sensor_value *val);
#endif

#endif /* __SENSOR_LIS2DH__ */
