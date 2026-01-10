/*
 * Copyright (c) 2025 Lothar Rubusch <l.rubusch@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL313_ADXL313_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL313_ADXL313_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/sensor/adxl313.h>

#define DT_DRV_COMPAT adi_adxl313

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif

/* communication */
#define ADXL313_WRITE_CMD      0x00
#define ADXL313_READ_CMD       0x80
#define ADXL313_MULTIBYTE_FLAG 0x40

#define ADXL313_REG_READ(x)           (FIELD_GET(GENMASK(5, 0), x) | ADXL313_READ_CMD)
#define ADXL313_REG_READ_MULTIBYTE(x) (ADXL313_REG_READ(x) | ADXL313_MULTIBYTE_FLAG)

#define ADXL313_COMPLEMENT_MASK(x) GENMASK(15, (x))
#define ADXL313_COMPLEMENT         GENMASK(15, 10)

/* registers */
#define ADXL313_REG_DEVID0 0x00
#define ADXL313_REG_DEVID1 0x01

#define ADXL313_REG_SOFT_RESET    0x18
#define ADXL313_REG_OFSX          0x1e
#define ADXL313_REG_OFSY          0x1f
#define ADXL313_REG_OFSZ          0x20
#define ADXL313_REG_THRESH_ACT    0x24
#define ADXL313_REG_THRESH_INACT  0x25
#define ADXL313_REG_TIME_INACT    0x26
#define ADXL313_REG_ACT_INACT_CTL 0x27
#define ADXL313_REG_RATE          0x2c
#define ADXL313_REG_POWER_CTL     0x2d
#define ADXL313_REG_INT_ENABLE    0x2e
#define ADXL313_REG_INT_MAP       0x2f
#define ADXL313_REG_INT_SOURCE    0x30
#define ADXL313_REG_DATA_FORMAT   0x31
#define ADXL313_REG_DATA_XYZ_REGS 0x32
#define ADXL313_REG_DATA_X0_REG   0x32
#define ADXL313_REG_DATA_X1_REG   0x33
#define ADXL313_REG_DATA_Y0_REG   0x34
#define ADXL313_REG_DATA_Y1_REG   0x35
#define ADXL313_REG_DATA_Z0_REG   0x36
#define ADXL313_REG_DATA_Z1_REG   0x37
#define ADXL313_REG_FIFO_CTL      0x38
#define ADXL313_REG_FIFO_STATUS   0x39

/* register fields / content values */
#define ADXL313_EXPECTED_DEVID0 0xad
#define ADXL313_EXPECTED_DEVID1 0x1d

/* bw / rate */
#define ADXL313_RATE_ODR_MSK GENMASK(3, 0)

enum adxl313_odr { /* Recommended ODR is betwen 12.5Hz and 400Hz */
	ADXL313_ODR_6_25HZ = ADXL313_DT_ODR_6_25,
	ADXL313_ODR_12_5HZ = ADXL313_DT_ODR_12_5,
	ADXL313_ODR_25HZ = ADXL313_DT_ODR_25,
	ADXL313_ODR_50HZ = ADXL313_DT_ODR_50,
	ADXL313_ODR_100HZ = ADXL313_DT_ODR_100,
	ADXL313_ODR_200HZ = ADXL313_DT_ODR_200,
	ADXL313_ODR_400HZ = ADXL313_DT_ODR_400,
	ADXL313_ODR_800HZ = ADXL313_DT_ODR_800,
	ADXL313_ODR_1600HZ = ADXL313_DT_ODR_1600,
	ADXL313_ODR_3200HZ = ADXL313_DT_ODR_3200,
};

/* power ctl */
#define ADXL313_POWER_CTL_MEASURE     BIT(3)
#define ADXL313_POWER_CTL_AUTO_SLEEP  BIT(4)
#define ADXL313_POWER_CTL_LINK        BIT(5)
#define ADXL313_POWER_CTL_I2C_DISABLE BIT(6)

/* interrupt: en, map and source */
#define ADXL313_INT_OVERRUN   BIT(0)
#define ADXL313_INT_WATERMARK BIT(1)
#define ADXL313_INT_INACT     BIT(3)
#define ADXL313_INT_ACT       BIT(4)
#define ADXL313_INT_DATA_RDY  BIT(7)

/* data format */
#define ADXL313_DATA_FORMAT_RANGE      GENMASK(1, 0)
#define ADXL313_DATA_FORMAT_LJUSTIFY   BIT(2) /* enable left justified */
#define ADXL313_DATA_FORMAT_FULL_RES   BIT(3)
#define ADXL313_DATA_FORMAT_INT_INVERT BIT(5) /* enable int active low */
#define ADXL313_DATA_FORMAT_3WIRE_SPI  BIT(6) /* enable 3-wire SPI */
#define ADXL313_DATA_FORMAT_SELF_TEST  BIT(7) /* enable self test */

enum adxl313_range {
	ADXL313_RANGE_0_5G = 0,
	ADXL313_RANGE_1G,
	ADXL313_RANGE_2G,
	ADXL313_RANGE_4G,
};

/* fifo: ctl and status */
#define ADXL313_FIFO_SAMPLE_SIZE 6

#define ADXL313_FIFO_CTL_MODE_BYPASSED  0x00
#define ADXL313_FIFO_CTL_MODE_OLD_SAVED 0x40
#define ADXL313_FIFO_CTL_MODE_STREAMED  0x80
#define ADXL313_FIFO_CTL_MODE_TRIGGERED 0xc0
#define ADXL313_FIFO_CTL_SAMPLES_MSK    GENMASK(4, 0)
#define ADXL313_FIFO_STATUS_ENTRIES_MSK GENMASK(5, 0)
#define ADXL313_FIFO_MAX_SIZE           32

#define ADXL313_BUS_I2C 0
#define ADXL313_BUS_SPI 1

struct adxl313_xyz_accel_data {
	enum adxl313_range selected_range;
	bool is_full_res;
	int16_t x;
	int16_t y;
	int16_t z;
};

union adxl313_bus {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif
};

/* register cache */
#define ADXL313_CACHE_START ADXL313_REG_SOFT_RESET  /* first entry */
#define ADXL313_CACHE_END   ADXL313_REG_FIFO_STATUS /* last entry */
#define ADXL313_CACHE_SIZE  (ADXL313_CACHE_END - ADXL313_CACHE_START + 1)
#define cache_idx(reg)      ((reg) - ADXL313_CACHE_START)

struct adxl313_dev_data {
	uint8_t reg_cache[ADXL313_CACHE_SIZE];
	struct adxl313_xyz_accel_data sample[ADXL313_FIFO_MAX_SIZE];
	int8_t fifo_entries; /* the actual read FIFO entries */
	uint8_t sample_idx;  /* index counting up sample_number entries */
	bool is_full_res;
	enum adxl313_odr odr;
	enum adxl313_range selected_range;
};

typedef bool (*adxl313_bus_is_ready_fn)(const union adxl313_bus *bus);
typedef int (*adxl313_reg_access_fn)(const struct device *dev, uint8_t cmd, uint8_t reg_addr,
				     uint8_t *data, size_t length);

struct adxl313_dev_config {
	const union adxl313_bus bus;
	adxl313_bus_is_ready_fn bus_is_ready;
	adxl313_reg_access_fn reg_access;
	enum adxl313_range selected_range;
	enum adxl313_odr odr;
	uint8_t bus_type;
};

int adxl313_reg_write_mask(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t data);

int adxl313_reg_access(const struct device *dev, uint8_t cmd, uint8_t addr, uint8_t *data,
		       size_t len);

int adxl313_reg_write(const struct device *dev, uint8_t addr, uint8_t *data, uint8_t len);

int adxl313_raw_reg_read(const struct device *dev, uint8_t addr, uint8_t *data, uint8_t len);

int adxl313_reg_read(const struct device *dev, uint8_t addr, uint8_t *data, uint8_t len);

int adxl313_reg_write_byte(const struct device *dev, uint8_t addr, uint8_t val);

int adxl313_reg_read_byte(const struct device *dev, uint8_t addr, uint8_t *buf);

int adxl313_read_sample(const struct device *dev, struct adxl313_xyz_accel_data *sample);

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL313_ADXL313_H_ */
