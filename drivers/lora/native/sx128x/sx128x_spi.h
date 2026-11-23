/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPI transactions for SX128x: opcode, optional address or buffer offset,
 * dummy byte before read data, and commands only while BUSY is low.
 */

#ifndef ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_SPI_H_
#define ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_SPI_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#define SX128X_SPI_BUSY_DEFAULT_TIMEOUT 1000

/* Before any other call */
void sx128x_spi_init(const struct device *dev);

int sx128x_spi_wait_busy(const struct device *dev, uint32_t timeout_ms);

/* Wake the chip from sleep with an NSS falling edge */
int sx128x_spi_wakeup(const struct device *dev);

int sx128x_spi_write_cmd(const struct device *dev, uint8_t opcode,
			 const uint8_t *data, size_t len);

int sx128x_spi_read_cmd(const struct device *dev, uint8_t opcode,
			uint8_t *data, size_t len);

int sx128x_spi_write_regs(const struct device *dev, uint16_t address,
			  const uint8_t *data, size_t len);

int sx128x_spi_read_regs(const struct device *dev, uint16_t address,
			 uint8_t *data, size_t len);

int sx128x_spi_write_buffer(const struct device *dev, uint8_t offset,
			    const uint8_t *data, size_t len);

int sx128x_spi_read_buffer(const struct device *dev, uint8_t offset,
			   uint8_t *data, size_t len);

/* Drive the external RF switch, through whichever enable GPIOs are wired */
void sx128x_spi_set_rf_switch(const struct device *dev, bool enable, bool tx);

/* Configure an optional GPIO: nothing to do when it is not wired */
int sx128x_spi_configure_gpio(const struct gpio_dt_spec *gpio,
			      gpio_flags_t flags, const char *name);

#endif /* ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_SPI_H_ */
