/*
 * Copyright (c) 2023 Fabian Blatz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Backend API for emulated UART
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SERIAL_UART_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SERIAL_UART_EMUL_H_

#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Define the application callback function signature for
 * uart_emul_callback_tx_data_ready_set() function.
 *
 * @param dev UART device instance
 * @param size Number of available bytes in TX buffer
 * @param user_data Arbitrary user data
 */
typedef void (*uart_emul_callback_tx_data_ready_t)(const struct device *dev, size_t size,
						   void *user_data);

/**
 * @brief Set the TX data ready callback
 *
 * This sets up the callback that is called every time data
 * was appended to the TX buffer.
 *
 * @param dev The emulated UART device instance
 * @param cb Pointer to the callback function
 * @param user_data Data to pass to callback function
 */
void uart_emul_callback_tx_data_ready_set(const struct device *dev,
					  uart_emul_callback_tx_data_ready_t cb, void *user_data);

/**
 * @brief Write (copy) data to RX buffer
 *
 * @param dev The emulated UART device instance
 * @param data The data to append
 * @param size Number of bytes to append
 *
 * @return Number of bytes appended
 */
uint32_t uart_emul_put_rx_data(const struct device *dev, uint8_t *data, size_t size);

/**
 * @brief Read data from TX buffer
 *
 * @param dev The emulated UART device instance
 * @param data The address of the output buffer
 * @param size Number of bytes to read
 *
 * @return Number of bytes written to the output buffer
 */
uint32_t uart_emul_get_tx_data(const struct device *dev, uint8_t *data, size_t size);

/**
 * @brief Clear RX buffer content
 *
 * @param dev The emulated UART device instance
 *
 * @return Number of cleared bytes
 */
uint32_t uart_emul_flush_rx_data(const struct device *dev);

/**
 * @brief Clear TX buffer content
 *
 * @param dev The emulated UART device instance
 *
 * @return Number of cleared bytes
 */
uint32_t uart_emul_flush_tx_data(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SERIAL_UART_EMUL_H_ */
