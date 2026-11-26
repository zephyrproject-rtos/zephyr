/*
 * Copyright (c) 2026 Shreyas Acharya
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BOSCH_BMI260_BMI260_H_
#define ZEPHYR_DRIVERS_SENSOR_BOSCH_BMI260_BMI260_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>

/*
 * The device answers on either I2C or SPI, and which one is settled per
 * instance at build time. The core keeps no knowledge of the difference: it
 * reaches the bus through bmi260_bus_io, whose two implementations live in
 * bmi260_i2c.c and bmi260_spi.c.
 */
union bmi260_bus {
#if defined(CONFIG_BMI260_BUS_SPI)
	struct spi_dt_spec spi;
#endif
#if defined(CONFIG_BMI260_BUS_I2C)
	struct i2c_dt_spec i2c;
#endif
};

struct bmi260_bus_io {
	/* Reports whether the bus controller this device hangs off is usable */
	int (*check)(const union bmi260_bus *bus);
	int (*read)(const union bmi260_bus *bus, uint8_t start, uint8_t *data, uint16_t len);
	int (*write)(const union bmi260_bus *bus, uint8_t start, const uint8_t *data, uint16_t len);
	/*
	 * Puts the device's interface into the state the rest of the driver
	 * expects. The device powers up listening on I2C, so the SPI
	 * implementation spends a throwaway read to move it to SPI, and the
	 * I2C one has nothing to do.
	 */
	int (*init)(const union bmi260_bus *bus);
};

struct bmi260_config {
	union bmi260_bus bus;
	const struct bmi260_bus_io *bus_io;
	/* Initial rates and full-scale ranges, as the devicetree asked for them. */
	uint16_t acc_odr_hz;
	uint16_t gyr_odr_hz;
	uint8_t acc_range_g;
	uint16_t gyr_range_dps;
};

/*
 * Raw counts from the most recent fetch. sensor_channel_get() is required to
 * be pure with respect to the last fetch, so it converts from here and never
 * touches the bus.
 */
struct bmi260_data {
	int16_t acc[3];
	int16_t gyr[3];
	int16_t temp;

	/*
	 * The ranges the device is configured for, kept so that conversion
	 * needs no bus access. Updated only after a write succeeds.
	 */
	uint8_t acc_range_g;
	uint16_t gyr_range_dps;
};

#if defined(CONFIG_BMI260_BUS_SPI)
#define BMI260_SPI_OPERATION (SPI_WORD_SET(8) | SPI_TRANSFER_MSB)
extern const struct bmi260_bus_io bmi260_bus_io_spi;
#endif

#if defined(CONFIG_BMI260_BUS_I2C)
extern const struct bmi260_bus_io bmi260_bus_io_i2c;
#endif

int bmi260_bus_check(const struct device *dev);
int bmi260_bus_init(const struct device *dev);
int bmi260_reg_read(const struct device *dev, uint8_t start, uint8_t *data, uint16_t len);
int bmi260_reg_write(const struct device *dev, uint8_t start, const uint8_t *data, uint16_t len);

/*
 * The device ignores writes that arrive closer together than roughly 450 us
 * while advanced power save is on, so the initialization path spaces them.
 */
int bmi260_reg_write_with_delay(const struct device *dev, uint8_t start, const uint8_t *data,
				uint16_t len, uint32_t delay_us);

#endif /* ZEPHYR_DRIVERS_SENSOR_BOSCH_BMI260_BMI260_H_ */
