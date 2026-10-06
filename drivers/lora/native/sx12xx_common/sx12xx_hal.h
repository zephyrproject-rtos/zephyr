/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPI transactions shared by the native Semtech SX12xx drivers. The SX126x
 * and SX128x families talk the same protocol: an opcode, then an address or
 * buffer offset, a dummy byte before read data, and a command only while
 * BUSY is low. Only some opcodes differ.
 */

#ifndef ZEPHYR_DRIVERS_LORA_NATIVE_SX12XX_HAL_H_
#define ZEPHYR_DRIVERS_LORA_NATIVE_SX12XX_HAL_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

/* The same on every SX12xx */
#define SX12XX_CMD_GET_STATUS 0xC0
#define SX12XX_CMD_SET_SLEEP  0x84

#define SX12XX_BUSY_DEFAULT_TIMEOUT 1000

/* Opcodes that differ between the chip families */
struct sx12xx_hal_opcodes {
	uint8_t write_register;
	uint8_t read_register;
	uint8_t write_buffer;
	uint8_t read_buffer;
};

/*
 * The part of the device config the common functions use: the first member
 * of the chip driver's config, as they take it from dev->config
 */
struct sx12xx_hal_config {
	struct spi_dt_spec spi;
	const struct sx12xx_hal_opcodes *opcodes;
	/* BUSY: a GPIO on discrete chips, a status flag on integrated ones */
	bool (*is_busy)(const struct device *dev);
	struct gpio_dt_spec tx_enable;
	struct gpio_dt_spec rx_enable;
};

/*
 * The part of the device data the common functions use: the first member of
 * the chip driver's data, as they take it from dev->data
 */
struct sx12xx_hal_data {
	/* One transaction with the chip at a time: BUSY check and transfer */
	struct k_mutex bus_lock;
};

/* Before any other call */
void sx12xx_hal_init(const struct device *dev);

int sx12xx_hal_wait_busy(const struct device *dev, uint32_t timeout_ms);

/* Wake the chip from sleep with an NSS falling edge */
int sx12xx_hal_wakeup(const struct device *dev);

int sx12xx_hal_write_cmd(const struct device *dev, uint8_t opcode,
			 const uint8_t *data, size_t len);

int sx12xx_hal_read_cmd(const struct device *dev, uint8_t opcode,
			uint8_t *data, size_t len);

int sx12xx_hal_write_regs(const struct device *dev, uint16_t address,
			  const uint8_t *data, size_t len);

int sx12xx_hal_read_regs(const struct device *dev, uint16_t address,
			 uint8_t *data, size_t len);

int sx12xx_hal_write_buffer(const struct device *dev, uint8_t offset,
			    const uint8_t *data, size_t len);

int sx12xx_hal_read_buffer(const struct device *dev, uint8_t offset,
			   uint8_t *data, size_t len);

/* Drive the external RF switch, through whichever enable GPIOs are wired */
void sx12xx_hal_set_rf_switch(const struct device *dev, bool enable, bool tx);

/* Configure an optional GPIO: nothing to do when it is not wired */
int sx12xx_hal_configure_gpio(const struct gpio_dt_spec *gpio,
			      gpio_flags_t flags, const char *name);

#endif /* ZEPHYR_DRIVERS_LORA_NATIVE_SX12XX_HAL_H_ */
