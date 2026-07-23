/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_NFC_MFRC522_MFRC522_H_
#define ZEPHYR_DRIVERS_NFC_MFRC522_MFRC522_H_

#define DT_DRV_COMPAT nxp_mfrc522

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/nfc.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/spi.h>

struct mfrc522_transport_api {
	int (*read_reg)(const struct device *dev, uint8_t reg, uint8_t *val);
	int (*write_reg)(const struct device *dev, uint8_t reg, uint8_t val);
};

struct mfrc522_config {
	const struct mfrc522_transport_api *transport;
	struct spi_dt_spec bus;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec irq_gpio;
};

struct mfrc522_data {
	struct k_mutex lock;
	struct k_sem irq_sem;
	struct gpio_callback irq_cb;
	bool hw_tx_crc;
	bool hw_rx_crc;
	uint32_t timeout_us;
};

extern const struct nfc_driver_api mfrc522_nfc_api;

#define MFRC522_SPI_OPERATION (SPI_WORD_SET(8) | SPI_TRANSFER_MSB)
extern const struct mfrc522_transport_api mfrc522_spi_transport;

int mfrc522_init_common(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_NFC_MFRC522_MFRC522_H_ */
