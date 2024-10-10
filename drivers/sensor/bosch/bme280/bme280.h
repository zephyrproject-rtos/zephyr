/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BME280_BME280_H_
#define ZEPHYR_DRIVERS_SENSOR_BME280_BME280_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>

#define DT_DRV_COMPAT bosch_bme280

#define BME280_BUS_SPI DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#define BME280_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

union bme280_bus {
#if BME280_BUS_SPI
	struct spi_dt_spec spi;
#endif
#if BME280_BUS_I2C
	struct i2c_dt_spec i2c;
#endif
};

typedef int (*bme280_bus_check_fn)(const union bme280_bus *bus);
typedef int (*bme280_reg_read_fn)(const union bme280_bus *bus, uint8_t start, uint8_t *buf,
				  int size);
typedef int (*bme280_reg_write_fn)(const union bme280_bus *bus, uint8_t reg, uint8_t val);

struct bme280_bus_io {
	bme280_bus_check_fn check;
	bme280_reg_read_fn read;
	bme280_reg_write_fn write;
};

#if BME280_BUS_SPI
#define BME280_SPI_OPERATION (SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA)
extern const struct bme280_bus_io bme280_bus_io_spi;
#endif

#if BME280_BUS_I2C
extern const struct bme280_bus_io bme280_bus_io_i2c;
#endif

#define BME280_REG_PRESS_MSB      0xF7
#define BME280_REG_COMP_START     0x88
#define BME280_REG_HUM_COMP_PART1 0xA1
#define BME280_REG_HUM_COMP_PART2 0xE1
#define BME280_REG_ID             0xD0
#define BME280_REG_CONFIG         0xF5
#define BME280_REG_CTRL_MEAS      0xF4
#define BME280_REG_CTRL_HUM       0xF2
#define BME280_REG_STATUS         0xF3
#define BME280_REG_RESET          0xE0

#define BMP280_CHIP_ID_SAMPLE_1 0x56
#define BMP280_CHIP_ID_SAMPLE_2 0x57
#define BMP280_CHIP_ID_MP       0x58
#define BME280_CHIP_ID          0x60
#define BME280_MODE_SLEEP       0x00
#define BME280_MODE_FORCED      0x01
#define BME280_MODE_NORMAL      0x03
#define BME280_SPI_3W_DISABLE   0x00
#define BME280_CMD_SOFT_RESET   0xB6
#define BME280_STATUS_MEASURING 0x08
#define BME280_STATUS_IM_UPDATE 0x01

#if defined CONFIG_BME280_MODE_NORMAL
#define BME280_MODE BME280_MODE_NORMAL
#elif defined CONFIG_BME280_MODE_FORCED
#define BME280_MODE BME280_MODE_FORCED
#endif

#if defined CONFIG_BME280_TEMP_OVER_1X
#define BME280_TEMP_OVER (1 << 5)
#elif defined CONFIG_BME280_TEMP_OVER_2X
#define BME280_TEMP_OVER (2 << 5)
#elif defined CONFIG_BME280_TEMP_OVER_4X
#define BME280_TEMP_OVER (3 << 5)
#elif defined CONFIG_BME280_TEMP_OVER_8X
#define BME280_TEMP_OVER (4 << 5)
#elif defined CONFIG_BME280_TEMP_OVER_16X
#define BME280_TEMP_OVER (5 << 5)
#endif

#if defined CONFIG_BME280_PRESS_OVER_1X
#define BME280_PRESS_OVER (1 << 2)
#elif defined CONFIG_BME280_PRESS_OVER_2X
#define BME280_PRESS_OVER (2 << 2)
#elif defined CONFIG_BME280_PRESS_OVER_4X
#define BME280_PRESS_OVER (3 << 2)
#elif defined CONFIG_BME280_PRESS_OVER_8X
#define BME280_PRESS_OVER (4 << 2)
#elif defined CONFIG_BME280_PRESS_OVER_16X
#define BME280_PRESS_OVER (5 << 2)
#endif

#if defined CONFIG_BME280_HUMIDITY_OVER_1X
#define BME280_HUMIDITY_OVER 1
#elif defined CONFIG_BME280_HUMIDITY_OVER_2X
#define BME280_HUMIDITY_OVER 2
#elif defined CONFIG_BME280_HUMIDITY_OVER_4X
#define BME280_HUMIDITY_OVER 3
#elif defined CONFIG_BME280_HUMIDITY_OVER_8X
#define BME280_HUMIDITY_OVER 4
#elif defined CONFIG_BME280_HUMIDITY_OVER_16X
#define BME280_HUMIDITY_OVER 5
#endif

#if defined CONFIG_BME280_STANDBY_05MS
#define BME280_STANDBY 0
#elif defined CONFIG_BME280_STANDBY_62MS
#define BME280_STANDBY (1 << 5)
#elif defined CONFIG_BME280_STANDBY_125MS
#define BME280_STANDBY (2 << 5)
#elif defined CONFIG_BME280_STANDBY_250MS
#define BME280_STANDBY (3 << 5)
#elif defined CONFIG_BME280_STANDBY_500MS
#define BME280_STANDBY (4 << 5)
#elif defined CONFIG_BME280_STANDBY_1000MS
#define BME280_STANDBY (5 << 5)
#elif defined CONFIG_BME280_STANDBY_2000MS
#define BME280_STANDBY (6 << 5)
#elif defined CONFIG_BME280_STANDBY_4000MS
#define BME280_STANDBY (7 << 5)
#endif

#if defined CONFIG_BME280_FILTER_OFF
#define BME280_FILTER 0
#elif defined CONFIG_BME280_FILTER_2
#define BME280_FILTER (1 << 2)
#elif defined CONFIG_BME280_FILTER_4
#define BME280_FILTER (2 << 2)
#elif defined CONFIG_BME280_FILTER_8
#define BME280_FILTER (3 << 2)
#elif defined CONFIG_BME280_FILTER_16
#define BME280_FILTER (4 << 2)
#endif

#define BME280_CTRL_MEAS_VAL (BME280_PRESS_OVER | BME280_TEMP_OVER | BME280_MODE)
#define BME280_CONFIG_VAL    (BME280_STANDBY | BME280_FILTER | BME280_SPI_3W_DISABLE)

#define BME280_CTRL_MEAS_OFF_VAL (BME280_PRESS_OVER | BME280_TEMP_OVER | BME280_MODE_SLEEP)

/* Convert to Q15.16 */
#define BME280_TEMP_CONV      100
#define BME280_TEMP_SHIFT     16
/* Treat UQ24.8 as Q23.8
 * Need to divide by 1000 to convert to kPa
 */
#define BME280_PRESS_CONV_KPA 1000
#define BME280_PRESS_SHIFT    23
/* Treat UQ22.10 as Q21.10 */
#define BME280_HUM_SHIFT      21

struct bme280_reading {
	/* Compensated values. */
	int32_t comp_temp;
	uint32_t comp_press;
	uint32_t comp_humidity;
};

struct bme280_data {
	/* Compensation parameters. */
	uint16_t dig_t1;
	int16_t dig_t2;
	int16_t dig_t3;
	uint16_t dig_p1;
	int16_t dig_p2;
	int16_t dig_p3;
	int16_t dig_p4;
	int16_t dig_p5;
	int16_t dig_p6;
	int16_t dig_p7;
	int16_t dig_p8;
	int16_t dig_p9;
	uint8_t dig_h1;
	int16_t dig_h2;
	uint8_t dig_h3;
	int16_t dig_h4;
	int16_t dig_h5;
	int8_t dig_h6;

	/* Carryover between temperature and pressure/humidity compensation. */
	int32_t t_fine;

	uint8_t chip_id;

	struct bme280_reading reading;
};

/*
 * RTIO
 */

struct bme280_decoder_header {
	uint64_t timestamp;
} __attribute__((__packed__));

struct bme280_encoded_data {
	struct bme280_decoder_header header;
	struct {
		/** Set if `temp` has data */
		uint8_t has_temp: 1;
		/** Set if `press` has data */
		uint8_t has_press: 1;
		/** Set if `humidity` has data */
		uint8_t has_humidity: 1;
	} __attribute__((__packed__));
	struct bme280_reading reading;
};

int bme280_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

void bme280_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

int bme280_sample_fetch(const struct device *dev, enum sensor_channel chan);

int bme280_sample_fetch_helper(const struct device *dev, enum sensor_channel chan,
			       struct bme280_reading *reading);

#endif /* ZEPHYR_DRIVERS_SENSOR_BME280_BME280_H_ */
