/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SENSOR_LIS2DH_H__
#define __SENSOR_LIS2DH_H__

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
#define LIS2DH_SPI_CFG			0x80

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
	struct device *bus;

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

static inline int lis2dh_burst_read(struct device *bus, u8_t start_addr,
				    u8_t *buf, u8_t num_bytes)
{
#if defined(CONFIG_LIS2DH_BUS_SPI)
	int status;
	u8_t tx_buf = LIS2DH_SPI_READ_BIT | LIS2DH_SPI_AUTOINC_ADDR |
		      start_addr;

	status = spi_slave_select(bus, LIS2DH_BUS_ADDRESS);
	status = spi_transceive(bus, &tx_buf, 1, buf, num_bytes);

	SYS_LOG_DBG("tx=0x%x num=0x%x, status=%d", tx_buf,
		    num_bytes, status);

	return status;
#elif defined(CONFIG_LIS2DH_BUS_I2C)
	return i2c_burst_read(bus, LIS2DH_BUS_ADDRESS, start_addr, buf,
			      num_bytes);
#else
	return -ENODEV;
#endif
}

static inline int lis2dh_reg_read_byte(struct device *bus, u8_t reg_addr,
				       u8_t *value)
{
#if defined(CONFIG_LIS2DH_BUS_SPI)
	int status;
	u8_t tx_buf = LIS2DH_SPI_READ_BIT | reg_addr;
	u8_t rx_buf[2];

	status = spi_slave_select(bus, LIS2DH_BUS_ADDRESS);
	status = spi_transceive(bus, &tx_buf, 1, rx_buf, 2);

	if (status == 0) {
		*value = rx_buf[1];
	}

	SYS_LOG_DBG("tx=0x%x rx1=0x%x", tx_buf, rx_buf[1]);

	return status;
#elif defined(CONFIG_LIS2DH_BUS_I2C)
	return i2c_reg_read_byte(bus, LIS2DH_BUS_ADDRESS, reg_addr, value);
#else
	return -ENODEV;
#endif
}

static inline int lis2dh_burst_write(struct device *bus, u8_t start_addr,
				     u8_t *buf, u8_t num_bytes)
{
#if defined(CONFIG_LIS2DH_BUS_SPI)
	int status;
	u8_t dummy;

	buf[0] = LIS2DH_SPI_AUTOINC_ADDR | start_addr;

	status = spi_slave_select(bus, LIS2DH_BUS_ADDRESS);
	status = spi_transceive(bus, buf, num_bytes, &dummy, 0);

	SYS_LOG_DBG("tx=0x%x num=0x%x, status=%d", buf[0],
		    num_bytes, status);

	return status;
#elif defined(CONFIG_LIS2DH_BUS_I2C)
	return i2c_burst_write(bus, LIS2DH_BUS_ADDRESS, start_addr, buf,
			       num_bytes);
#else
	return -ENODEV;
#endif
}

static inline int lis2dh_reg_write_byte(struct device *bus, u8_t reg_addr,
					u8_t value)
{
#if defined(CONFIG_LIS2DH_BUS_SPI)
	u8_t tx_buf[2] = { reg_addr & LIS2DH_SPI_ADDR_MASK, value };
	u8_t dummy;

	spi_slave_select(bus, LIS2DH_BUS_ADDRESS);

	SYS_LOG_DBG("tx0=0x%x tx1=0x%x", tx_buf[0], tx_buf[1]);

	return spi_transceive(bus, tx_buf, 2, &dummy, 0);
#elif defined(CONFIG_LIS2DH_BUS_I2C)
	u8_t tx_buf[2] = {reg_addr, value};

	return i2c_write(bus, tx_buf, sizeof(tx_buf), LIS2DH_BUS_ADDRESS);
#else
	return -ENODEV;
#endif
}

static inline int lis2dh_bus_configure(struct device *bus)
{
#if defined(CONFIG_LIS2DH_BUS_SPI)
	struct spi_config config = {
				.config = LIS2DH_SPI_CFG,
				.max_sys_freq = CONFIG_LIS2DH_SPI_FREQUENCY,
			};

	return spi_configure(bus, &config);
#elif defined(CONFIG_LIS2DH_BUS_I2C)
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

int lis2dh_reg_field_update(struct device *bus, u8_t reg_addr,
			    u8_t pos, u8_t mask, u8_t val);

int lis2dh_acc_slope_config(struct device *dev, enum sensor_attribute attr,
			    const struct sensor_value *val);
#endif

#endif /* __SENSOR_LIS2DH__ */
