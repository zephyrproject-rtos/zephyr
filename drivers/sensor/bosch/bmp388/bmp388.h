/* Bosch BMP388 pressure sensor
 *
 * Copyright (c) 2020 Facebook, Inc. and its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp388-ds001.pdf
 */

#ifndef ZEPHYR_BMP388_H
#define ZEPHYR_BMP388_H

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT  bosch_bmp388
#define BMP388_BUS_SPI DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#define BMP388_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT  bosch_bmp390
#define BMP390_BUS_SPI DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#define BMP390_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#undef DT_DRV_COMPAT

#if defined(BMP388_BUS_SPI) || defined(BMP390_BUS_SPI)
#define BMP3XX_USE_SPI_BUS
#endif

#if defined(BMP388_BUS_I2C) || defined(BMP390_BUS_I2C)
#define BMP3XX_USE_I2C_BUS
#endif

union bmp388_bus {
#ifdef BMP3XX_USE_SPI_BUS
	struct spi_dt_spec spi;
#endif
#ifdef BMP3XX_USE_I2C_BUS
	struct i2c_dt_spec i2c;
#endif
};

typedef int (*bmp388_bus_check_fn)(const union bmp388_bus *bus);
typedef int (*bmp388_reg_read_fn)(const union bmp388_bus *bus,
				  uint8_t start, uint8_t *buf, int size);
typedef int (*bmp388_reg_write_fn)(const union bmp388_bus *bus,
				   uint8_t reg, uint8_t val);

struct bmp388_bus_io {
	bmp388_bus_check_fn check;
	bmp388_reg_read_fn read;
	bmp388_reg_write_fn write;
};

#ifdef BMP3XX_USE_SPI_BUS
#define BMP388_SPI_OPERATION (SPI_WORD_SET(8) | SPI_TRANSFER_MSB |	\
			      SPI_MODE_CPOL | SPI_MODE_CPHA)
extern const struct bmp388_bus_io bmp388_bus_io_spi;
#endif

#ifdef BMP3XX_USE_I2C_BUS
extern const struct bmp388_bus_io bmp388_bus_io_i2c;
#endif

/* registers */
#define BMP388_REG_CHIPID       0x00
#define BMP388_REG_ERR_REG      0x02
#define BMP388_REG_STATUS       0x03
#define BMP388_REG_DATA0        0x04
#define BMP388_REG_DATA1        0x05
#define BMP388_REG_DATA2        0x06
#define BMP388_REG_DATA3        0x07
#define BMP388_REG_DATA4        0x08
#define BMP388_REG_DATA5        0x09
#define BMP388_REG_SENSORTIME0  0x0C
#define BMP388_REG_SENSORTIME1  0x0D
#define BMP388_REG_SENSORTIME2  0x0E
#define BMP388_REG_SENSORTIME3  0x0F
#define BMP388_REG_EVENT        0x10
#define BMP388_REG_INT_STATUS   0x11
#define BMP388_REG_FIFO_LENGTH0 0x12
#define BMP388_REG_FIFO_LENGTH1 0x13
#define BMP388_REG_FIFO_DATA    0x14
#define BMP388_REG_FIFO_WTM0    0x15
#define BMP388_REG_FIFO_WTM1    0x16
#define BMP388_REG_FIFO_CONFIG1 0x17
#define BMP388_REG_FIFO_CONFIG2 0x18
#define BMP388_REG_INT_CTRL     0x19
#define BMP388_REG_IF_CONF      0x1A
#define BMP388_REG_PWR_CTRL     0x1B
#define BMP388_REG_OSR          0x1C
#define BMP388_REG_ODR          0x1D
#define BMP388_REG_CONFIG       0x1F
#define BMP388_REG_CALIB0       0x31
#define BMP388_REG_CMD          0x7E

/* BMP388_REG_CHIPID */
#define BMP388_CHIP_ID 0x50

/* BMP388_REG_STATUS */
#define BMP388_STATUS_FATAL_ERR  BIT(0)
#define BMP388_STATUS_CMD_ERR    BIT(1)
#define BMP388_STATUS_CONF_ERR   BIT(2)
#define BMP388_STATUS_CMD_RDY    BIT(4)
#define BMP388_STATUS_DRDY_PRESS BIT(5)
#define BMP388_STATUS_DRDY_TEMP  BIT(6)

/* BMP388_REG_INT_CTRL */
#define BMP388_INT_CTRL_DRDY_EN_POS  6
#define BMP388_INT_CTRL_DRDY_EN_MASK BIT(6)

/* BMP388_REG_PWR_CTRL */
#define BMP388_PWR_CTRL_PRESS_EN    BIT(0)
#define BMP388_PWR_CTRL_TEMP_EN     BIT(1)
#define BMP388_PWR_CTRL_MODE_POS    4
#define BMP388_PWR_CTRL_MODE_MASK   (0x03 << BMP388_PWR_CTRL_MODE_POS)
#define BMP388_PWR_CTRL_MODE_SLEEP  (0x00 << BMP388_PWR_CTRL_MODE_POS)
#define BMP388_PWR_CTRL_MODE_FORCED (0x01 << BMP388_PWR_CTRL_MODE_POS)
#define BMP388_PWR_CTRL_MODE_NORMAL (0x03 << BMP388_PWR_CTRL_MODE_POS)

/* BMP388_REG_OSR */
#define BMP388_ODR_POS  0
#define BMP388_ODR_MASK 0x1F

/* BMP388_REG_ODR */
#define BMP388_OSR_PRESSURE_POS  0
#define BMP388_OSR_PRESSURE_MASK (0x07 << BMP388_OSR_PRESSURE_POS)
#define BMP388_OSR_TEMP_POS      3
#define BMP388_OSR_TEMP_MASK     (0x07 << BMP388_OSR_TEMP_POS)

/* BMP388_REG_CONFIG */
#define BMP388_IIR_FILTER_POS  1
#define BMP388_IIR_FILTER_MASK (0x7 << BMP388_IIR_FILTER_POS)

/* BMP388_REG_CMD */
#define BMP388_CMD_FIFO_FLUSH 0xB0
#define BMP388_CMD_SOFT_RESET 0xB6

/* default PWR_CTRL settings */
#define BMP388_PWR_CTRL_ON	    \
	(BMP388_PWR_CTRL_PRESS_EN | \
	 BMP388_PWR_CTRL_TEMP_EN |  \
	 BMP388_PWR_CTRL_MODE_NORMAL)
#define BMP388_PWR_CTRL_OFF 0

#define BMP388_SAMPLE_BUFFER_SIZE (6)

struct bmp388_cal_data {
	uint16_t t1;
	uint16_t t2;
	int8_t t3;
	int16_t p1;
	int16_t p2;
	int8_t p3;
	int8_t p4;
	uint16_t p5;
	uint16_t p6;
	int8_t p7;
	int8_t p8;
	int16_t p9;
	int8_t p10;
	int8_t p11;
} __packed;

struct bmp388_sample {
	uint32_t press;
	uint32_t raw_temp;
	int64_t comp_temp;
};

struct bmp388_config {
	union bmp388_bus bus;
	const struct bmp388_bus_io *bus_io;

#ifdef CONFIG_BMP388_TRIGGER
	struct gpio_dt_spec gpio_int;
#endif

	uint8_t iir_filter;
};

struct bmp388_data {
	uint8_t odr;
	uint8_t osr_pressure;
	uint8_t osr_temp;
	uint8_t chip_id;
	struct bmp388_cal_data cal;

#if defined(CONFIG_BMP388_TRIGGER)
	struct gpio_callback gpio_cb;
#endif

	struct bmp388_sample sample;

#ifdef CONFIG_BMP388_TRIGGER_OWN_THREAD
	struct k_sem sem;
#endif

#ifdef CONFIG_BMP388_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif

#if defined(CONFIG_BMP388_TRIGGER_GLOBAL_THREAD) || \
	defined(CONFIG_BMP388_TRIGGER_DIRECT)
	const struct device *dev;
#endif

#ifdef CONFIG_BMP388_TRIGGER
	sensor_trigger_handler_t handler_drdy;
	const struct sensor_trigger *trig_drdy;
#endif /* CONFIG_BMP388_TRIGGER */
};

int bmp388_trigger_mode_init(const struct device *dev);
int bmp388_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);
int bmp388_reg_field_update(const struct device *dev,
			    uint8_t reg,
			    uint8_t mask,
			    uint8_t val);

#endif /* ZEPHYR_BMP388_H */
