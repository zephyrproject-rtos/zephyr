/*
 * Copyright (c) 2026 Witekio
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_EMW3080_SPI_H
#define ZEPHYR_DRIVERS_WIFI_EMW3080_SPI_H

#include <stdint.h>

#include <zephyr/kernel.h>

#define EMW3080_SPI_MAX_FRAME_LEN 2500

/* Called from the SPI transport thread for every frame received from the module. */
typedef void (*emw3080_spi_rx_cb_t)(const uint8_t *data, uint16_t len, void *user);

/**
 * @brief Configure the pins, reset the module and start the transport thread.
 *
 * @retval 0 Success.
 * @retval -ENODEV A GPIO or the SPI bus is not ready.
 * @retval -errno GPIO configuration error.
 */
int emw3080_spi_init(emw3080_spi_rx_cb_t rx_cb, void *user);

/**
 * @brief Send one frame to the module in a single SPI transaction.
 *
 * Blocks until the transaction has run. The buffer must stay valid until the
 * function returns.
 *
 * @retval 0 Frame sent.
 * @retval -EINVAL Invalid buffer.
 * @retval -EMSGSIZE Frame larger than EMW3080_SPI_MAX_FRAME_LEN.
 * @retval -ETIMEDOUT Transaction not run before @p timeout.
 * @retval -EPROTO Invalid peripheral header.
 * @retval -EIO FLOW handshake or SPI bus error.
 */
int emw3080_spi_send(const uint8_t *data, uint16_t len, k_timeout_t timeout);

/**
 * @brief Hardware reset of the module.
 *
 * Waits for any running transaction, pulses reset and waits for the module
 * to boot. Pending frames are lost.
 */
void emw3080_spi_reset(void);

#endif /* ZEPHYR_DRIVERS_WIFI_EMW3080_SPI_H */
