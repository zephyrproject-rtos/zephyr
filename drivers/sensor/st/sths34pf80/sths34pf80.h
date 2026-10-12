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

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/regmap.h>
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
	/** Bus type, used by rtio_read_regs_async() to set the I2C restart/stop flags. */
	rtio_bus_type bus_type;
#ifdef CONFIG_STHS34PF80_TRIGGER
	/** INT/DRDY pin (the "int-gpios" devicetree property). */
	struct gpio_dt_spec int_gpio;
#endif
};

struct sths34pf80_data {
	/** RTIO context used to queue the asynchronous bus transfers. */
	struct rtio *rtio_ctx;
	/** Bus iodev (I2C or SPI) the transfers are submitted to. */
	struct rtio_iodev *iodev;
#ifdef CONFIG_STHS34PF80_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_work work;
	sensor_trigger_handler_t handler;
	struct sensor_trigger trig;
#endif
};

#ifdef CONFIG_STHS34PF80_TRIGGER
/**
 * @brief Initialize the INT/DRDY GPIO interrupt for the sensor.
 *
 * Does nothing and returns 0 if "int-gpios" is not present in the devicetree.
 *
 * @param dev Sensor device instance.
 *
 * @retval 0 On success or when no INT GPIO is configured.
 * @retval -ENODEV INT GPIO controller is not ready.
 * @retval <0 GPIO configuration error.
 */
int sths34pf80_trigger_init(const struct device *dev);

/**
 * @brief Set or clear the data-ready trigger handler.
 *
 * Only @c SENSOR_TRIG_DATA_READY is supported. Pass NULL for @p handler to
 * disable the interrupt and route the INT pin to high-impedance.
 *
 * @param dev     Sensor device instance.
 * @param trig    Trigger to activate; only SENSOR_TRIG_DATA_READY is supported.
 * @param handler Callback invoked on each data-ready event, or NULL to disable.
 *
 * @retval 0        On success.
 * @retval -ENOTSUP No INT GPIO configured, or trigger type is not DATA_READY.
 * @retval -EIO     Failed to configure the INT pin routing in the sensor.
 * @retval <0       GPIO configuration error.
 */
int sths34pf80_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			   sensor_trigger_handler_t handler);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_STHS34PF80_STHS34PF80_H_ */
