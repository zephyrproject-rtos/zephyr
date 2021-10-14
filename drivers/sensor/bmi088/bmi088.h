/* Bosch BMI088 inertial measurement unit header
 *
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI088_BMI088_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI088_BMI088_H_

#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <drivers/spi.h>
#include <sys/util.h>


/* gyro register */

/* read-only */
#define BMI088_REG_CHIPID    0x00
#define RATE_X_LSB      0x02
#define RATE_X_MSB      0x03
#define RATE_Y_LSB      0x04
#define RATE_Y_MSB      0x05
#define RATE_Z_LSB      0x06
#define RATE_Z_MSB      0x07
#define INT_STAT_1      0x0A
#define FIFO_STATUS     0x0E
#define FIFO_DATA       0x3F

/* write-only */
#define GYRO_SOFTRESET  0x14

/* read/write */
#define GYRO_RANGE      0x0F
#define GYRO_BANDWIDTH  0x10
#define GYRO_LPM1       0x11
#define GYRO_INT_CTRL   0x15
#define IO_CONF         0x16
#define IO_MAP          0x18
#define FIFO_WM_EN      0x1E
#define FIFO_EXT_INT_S  0x34
#define G_FIFO_CONF_0   0x3D
#define G_FIFO_CONF_1   0x3E
#define GYRO_SELFTEST   0x3C

/* bit-fields */

/* GYRO_INT_STAT_1 */
#define GYRO_FIFO_INT   BIT(4)
#define GYRO_DRDY       BIT(7)
/* FIFO_STATUS */
#define FIFO_FRAMECOUNT
#define FIFO_OVERRUN    BIT(7)
/* GYRO_INT_CTRL */
#define GYRO_FIFO_EN    BIT(6)
#define GYRO_DATA_EN    BIT(7)

/* Indicates a read operation; bit 7 is clear on write s*/
#define BMI088_REG_READ			BIT(7)
#define BMI088_REG_MASK			0x7f

#define BMI088_CHIP_ID          0x0F

/* end of default settings */

struct bmi088_range {
	uint16_t range;
	uint8_t reg_val;
};

#define BMI088_BUS_SPI		DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)


typedef bool (*bmi088_bus_ready_fn)(const struct device *dev);
typedef int (*bmi088_reg_read_fn)(const struct device *dev,
				  uint8_t reg_addr, void *data, uint8_t len);
typedef int (*bmi088_reg_write_fn)(const struct device *dev,
				   uint8_t reg_addr, void *data, uint8_t len);
/*
struct bmi088_bus_io {
	bmi088_bus_ready_fn ready;
	bmi088_reg_read_fn read;
	bmi088_reg_write_fn write;
};
*/
struct bmi088_cfg {
    struct spi_dt_spec bus;
	//const struct bmi088_bus_io *bus_io;
};


/* Each sample has X, Y and Z */
#define BMI088_AXES             3
struct bmi088_gyro_sample {
    uint16_t gyr[BMI088_AXES];
};

struct bmi088_scale {
	uint16_t gyr; /* micro radians/s/lsb */
};

struct bmi088_data {
	const struct device *bus;
	struct bmi088_gyro_sample sample;
	struct bmi088_scale scale;
};

static inline struct bmi088_data *to_data(const struct device *dev)
{
	return dev->data;
}

static inline const struct bmi088_cfg *to_config(const struct device *dev)
{
	return dev->config;
}

int bmi088_read(const struct device *dev, uint8_t reg_addr,
		void *data, uint8_t len);
int bmi088_byte_read(const struct device *dev, uint8_t reg_addr,
		     uint8_t *byte);
int bmi088_byte_write(const struct device *dev, uint8_t reg_addr,
		      uint8_t byte);
int bmi088_word_write(const struct device *dev, uint8_t reg_addr,
		      uint16_t word);
int bmi088_reg_field_update(const struct device *dev, uint8_t reg_addr,
			    uint8_t pos, uint8_t mask, uint8_t val);
static inline int bmi088_reg_update(const struct device *dev,
				    uint8_t reg_addr,
				    uint8_t mask, uint8_t val)
{
	return bmi088_reg_field_update(dev, reg_addr, 0, mask, val);
}

int bmi088_acc_slope_config(const struct device *dev,
			    enum sensor_attribute attr,
			    const struct sensor_value *val);
int32_t bmi088_acc_reg_val_to_range(uint8_t reg_val);
int32_t bmi088_gyr_reg_val_to_range(uint8_t reg_val);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI088_BMI088_H_ */
