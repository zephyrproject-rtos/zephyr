/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private definitions of the STHS34PF80 infrared presence and motion sensor driver.
 *
 * The register access is done through the STMicroelectronics STdC library
 * (sths34pf80_reg.h) provided by the hal_st module. The read path uses
 * native asynchronous RTIO bus transfers.
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_STHS34PF80_STHS34PF80_H_
#define ZEPHYR_DRIVERS_SENSOR_STHS34PF80_STHS34PF80_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <stmemsc.h>
#include "sths34pf80_reg.h"

struct sths34pf80_config {
	/** STdC library bus context (register read/write callbacks and handle). */
	stmdev_ctx_t ctx;
	/** Bus specification referenced by @c ctx.handle. */
	union {
		const struct i2c_dt_spec i2c;
		const struct spi_dt_spec spi;
	} stmemsc_cfg;
	/** Value of the "odr" devicetree property. */
	uint8_t odr;
	/** Bus-ready check (I2C or SPI). */
	bool (*bus_ready)(const struct sths34pf80_config *cfg);
	/** OR-ed into the register address for reads (I2C, SPI). */
	uint8_t read_mask;
	/** True if the device is on an SPI bus. */
	bool is_spi;
};

struct sths34pf80_data {
	/** RTIO context used to queue the asynchronous bus transfers. */
	struct rtio *rtio_ctx;
	/** Bus iodev (I2C or SPI) the transfers are submitted to. */
	struct rtio_iodev *iodev;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_STHS34PF80_STHS34PF80_H_ */
