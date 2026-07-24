/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMP581_BMP581_BUS_H_
#define ZEPHYR_DRIVERS_SENSOR_BMP581_BMP581_BUS_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/i3c.h>

enum bmp581_bus_type {
	BMP581_BUS_TYPE_I2C,
	BMP581_BUS_TYPE_SPI,
	BMP581_BUS_TYPE_I3C,
};

struct bmp581_bus {
	/** Blocking I2C for register access and soft reset; async uses prep_reg_*_rtio_async(). */
	const struct i2c_dt_spec *i2c_spec;
	struct {
		struct rtio *ctx;
		struct rtio_iodev *iodev;
		enum bmp581_bus_type type;
#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(bosch_bmp581, i3c)
		struct {
			struct i3c_device_desc *desc;
			const struct i3c_device_id id;
		} i3c;
#endif
	} rtio;
};

/**
 * Prepare register read (sub-address write then read). @p reg_addr must stay valid until
 * the transfer completes (e.g. @c struct bmp581_stream i2c_reg_* fields).
 */
int bmp581_prep_reg_read_rtio_async(const struct bmp581_bus *bus, const uint8_t *reg_addr,
				    uint8_t *buf, size_t size, struct rtio_sqe **out);

/**
 * Prepare register write (sub-address then payload). @p reg_addr and @p buf must stay
 * readable until the transfer completes.
 */
int bmp581_prep_reg_write_rtio_async(const struct bmp581_bus *bus, const uint8_t *reg_addr,
				     const uint8_t *buf, size_t size, struct rtio_sqe **out);

/** Blocking register read (I2C via @p bus->i2c_spec; SPI/I3C via RTIO). */
int bmp581_reg_read_rtio(const struct bmp581_bus *bus, uint8_t start, uint8_t *buf, int size);

/** Blocking register write (I2C via @p bus->i2c_spec; SPI/I3C via RTIO). */
int bmp581_reg_write_rtio(const struct bmp581_bus *bus, uint8_t reg, const uint8_t *buf, int size);

/** Blocking I2C burst write (len <= 8), e.g. soft reset. */
int bmp581_bus_i2c_burst_write(const struct bmp581_bus *bus, const uint8_t *data, size_t len);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMP581_BMP581_BUS_H_ */
