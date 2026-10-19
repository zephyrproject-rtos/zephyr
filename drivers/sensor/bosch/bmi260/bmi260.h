/*
 * Copyright (c) 2021 Bosch Sensortec GmbH
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2026 Shreyas Acharya
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BOSCH_BMI260_BMI260_H_
#define ZEPHYR_DRIVERS_SENSOR_BOSCH_BMI260_BMI260_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

/* Chunk size used when uploading the configuration data */
#define BMI260_WR_LEN                     32
#define BMI260_CONFIG_FILE_RETRIES        15
#define BMI260_CONFIG_FILE_POLL_PERIOD_US 10000
#define BMI260_INTER_WRITE_DELAY_US       1000

#define BMI260_REG_CHIP_ID         0x00
#define BMI260_REG_ACC_X_LSB       0x0C
#define BMI260_REG_INTERNAL_STATUS 0x21
#define BMI260_REG_TEMPERATURE_0   0x22
#define BMI260_REG_ACC_CONF        0x40
#define BMI260_REG_ACC_RANGE       0x41
#define BMI260_REG_GYR_CONF        0x42
#define BMI260_REG_GYR_RANGE       0x43
#define BMI260_REG_INIT_CTRL       0x59
#define BMI260_REG_INIT_ADDR_0     0x5B
#define BMI260_REG_INIT_DATA       0x5E
#define BMI260_REG_PWR_CONF        0x7C
#define BMI260_REG_PWR_CTRL        0x7D
#define BMI260_REG_CMD             0x7E

/* Register address field of an SPI transfer and the SPI read flag */
#define BMI260_REG_MASK     GENMASK(6, 0)
#define BMI260_SPI_READ_BIT BIT(7)

#define BMI260_CHIP_ID 0x27

#define BMI260_CMD_SOFT_RESET 0xB6

/* Timings in microseconds */
#define BMI260_POWER_ON_TIME        500
#define BMI260_SOFT_RESET_TIME      2000
#define BMI260_TRANSC_DELAY_SUSPEND 450

#define BMI260_PREPARE_CONFIG_LOAD  0x00
#define BMI260_COMPLETE_CONFIG_LOAD 0x01

/* The configuration data is written in 16 bit words, INIT_ADDR_0 takes the word index */
#define BMI260_INIT_ADDR_0_MASK  GENMASK(3, 0)
#define BMI260_INIT_ADDR_1_SHIFT 4

#define BMI260_INST_MESSAGE_MSK     GENMASK(3, 0)
#define BMI260_INST_MESSAGE_INIT_OK 0x01

#define BMI260_PWR_CONF_ADV_PWR_SAVE_MSK BIT(0)
#define BMI260_PWR_CONF_ADV_PWR_SAVE_EN  1
#define BMI260_PWR_CONF_ADV_PWR_SAVE_DIS 0

#define BMI260_PWR_CTRL_MSK     GENMASK(3, 0)
#define BMI260_PWR_CTRL_GYR_EN  BIT(1)
#define BMI260_PWR_CTRL_ACC_EN  BIT(2)
#define BMI260_PWR_CTRL_TEMP_EN BIT(3)

/* ACC_CONF */
#define BMI260_ACC_ODR_MSK  GENMASK(3, 0)
#define BMI260_ACC_BWP_MSK  GENMASK(6, 4)
#define BMI260_ACC_FILT_MSK BIT(7)

#define BMI260_ACC_ODR_25D32_HZ 0x01
#define BMI260_ACC_ODR_25D16_HZ 0x02
#define BMI260_ACC_ODR_25D8_HZ  0x03
#define BMI260_ACC_ODR_25D4_HZ  0x04
#define BMI260_ACC_ODR_25D2_HZ  0x05
#define BMI260_ACC_ODR_25_HZ    0x06
#define BMI260_ACC_ODR_50_HZ    0x07
#define BMI260_ACC_ODR_100_HZ   0x08
#define BMI260_ACC_ODR_200_HZ   0x09
#define BMI260_ACC_ODR_400_HZ   0x0A
#define BMI260_ACC_ODR_800_HZ   0x0B
#define BMI260_ACC_ODR_1600_HZ  0x0C

#define BMI260_ACC_BWP_OSR4_AVG1  0x00
#define BMI260_ACC_BWP_OSR2_AVG2  0x01
#define BMI260_ACC_BWP_NORM_AVG4  0x02
#define BMI260_ACC_BWP_CIC_AVG8   0x03
#define BMI260_ACC_BWP_RES_AVG16  0x04
#define BMI260_ACC_BWP_RES_AVG32  0x05
#define BMI260_ACC_BWP_RES_AVG64  0x06
#define BMI260_ACC_BWP_RES_AVG128 0x07

#define BMI260_ACC_FILT_PWR_OPT  0x00
#define BMI260_ACC_FILT_PERF_OPT 0x01

/* ACC_RANGE */
#define BMI260_ACC_RANGE_MSK GENMASK(1, 0)
#define BMI260_ACC_RANGE_2G  0x00
#define BMI260_ACC_RANGE_4G  0x01
#define BMI260_ACC_RANGE_8G  0x02
#define BMI260_ACC_RANGE_16G 0x03

/* GYR_CONF */
#define BMI260_GYR_ODR_MSK        GENMASK(3, 0)
#define BMI260_GYR_BWP_MSK        GENMASK(5, 4)
#define BMI260_GYR_FILT_NOISE_MSK BIT(6)
#define BMI260_GYR_FILT_MSK       BIT(7)

#define BMI260_GYR_ODR_25_HZ   0x06
#define BMI260_GYR_ODR_50_HZ   0x07
#define BMI260_GYR_ODR_100_HZ  0x08
#define BMI260_GYR_ODR_200_HZ  0x09
#define BMI260_GYR_ODR_400_HZ  0x0A
#define BMI260_GYR_ODR_800_HZ  0x0B
#define BMI260_GYR_ODR_1600_HZ 0x0C
#define BMI260_GYR_ODR_3200_HZ 0x0D

#define BMI260_GYR_BWP_OSR4 0x00
#define BMI260_GYR_BWP_OSR2 0x01
#define BMI260_GYR_BWP_NORM 0x02

#define BMI260_GYR_FILT_NOISE_PWR  0x00
#define BMI260_GYR_FILT_NOISE_PERF 0x01

#define BMI260_GYR_FILT_PWR_OPT  0x00
#define BMI260_GYR_FILT_PERF_OPT 0x01

/* GYR_RANGE */
#define BMI260_GYR_RANGE_MSK     GENMASK(2, 0)
#define BMI260_GYR_RANGE_2000DPS 0x00
#define BMI260_GYR_RANGE_1000DPS 0x01
#define BMI260_GYR_RANGE_500DPS  0x02
#define BMI260_GYR_RANGE_250DPS  0x03
#define BMI260_GYR_RANGE_125DPS  0x04

/* Accelerometer and gyroscope samples are read in one burst, 3 axes of 16 bits each */
#define BMI260_DATA_LEN    12
#define BMI260_DATA_GYR_OFFSET 6

/* Temperature: raw 0x8000 means invalid, 512 LSB per degree, 23 degC at raw value 0 */
#define BMI260_TEMP_INVALID       0x8000
#define BMI260_TEMP_CENTER_OFFSET 23000000LL
#define BMI260_TEMP_LSB_PER_DEG   512LL

struct bmi260_data {
	int16_t ax, ay, az, gx, gy, gz, temp;
	/* Full scale ranges in g and in degrees per second */
	uint8_t acc_range;
	uint16_t gyr_range;
	/* Current output data rate register values */
	uint8_t acc_odr;
	uint8_t gyr_odr;
};

union bmi260_bus {
#if CONFIG_BMI260_BUS_SPI
	struct spi_dt_spec spi;
#endif
#if CONFIG_BMI260_BUS_I2C
	struct i2c_dt_spec i2c;
#endif
};

typedef int (*bmi260_bus_check_fn)(const union bmi260_bus *bus);
typedef int (*bmi260_bus_init_fn)(const union bmi260_bus *bus);
typedef int (*bmi260_reg_read_fn)(const union bmi260_bus *bus, uint8_t start, uint8_t *data,
				  uint16_t len);
typedef int (*bmi260_reg_write_fn)(const union bmi260_bus *bus, uint8_t start,
				   const uint8_t *data, uint16_t len);

struct bmi260_bus_io {
	bmi260_bus_check_fn check;
	bmi260_reg_read_fn read;
	bmi260_reg_write_fn write;
	bmi260_bus_init_fn init;
};

struct bmi260_config {
	union bmi260_bus bus;
	const struct bmi260_bus_io *bus_io;
};

#if CONFIG_BMI260_BUS_SPI
#define BMI260_SPI_OPERATION    (SPI_WORD_SET(8) | SPI_TRANSFER_MSB)
#define BMI260_SPI_ACC_DELAY_US 2
extern const struct bmi260_bus_io bmi260_bus_io_spi;
#endif

#if CONFIG_BMI260_BUS_I2C
extern const struct bmi260_bus_io bmi260_bus_io_i2c;
#endif

int bmi260_reg_read(const struct device *dev, uint8_t reg, uint8_t *data, uint16_t length);

int bmi260_reg_write(const struct device *dev, uint8_t reg, const uint8_t *data, uint16_t length);

int bmi260_reg_write_with_delay(const struct device *dev, uint8_t reg, const uint8_t *data,
				uint16_t length, uint32_t delay_us);

#endif /* ZEPHYR_DRIVERS_SENSOR_BOSCH_BMI260_BMI260_H_ */
