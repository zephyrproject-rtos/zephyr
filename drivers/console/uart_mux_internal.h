/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/console/uart_mux.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Send data to real UART (the data should be muxed already)
 *
 * @param uart Muxed uart
 * @param buf Data to send
 * @param size Data length
 *
 * @return >=0 if data was sent (and number of bytes sent), <0 if error
 */
int uart_mux_send(const struct device *uart, const uint8_t *buf, size_t size);

/**
 * @brief Receive unmuxed data.
 *
 * @param mux UART mux device structure.
 * @param dlci DLCI id for the muxing channel that should receive the data.
 * @param data Received data (already unmuxed)
 * @param len Length of the received data
 *
 * @retval >=0 No errors, number of bytes received
 * @retval <0 Error
 */
int uart_mux_recv(const struct device *mux, struct gsm_dlci *dlci,
		  uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
