/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for UART MUX drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UART_MUX_H_
#define ZEPHYR_INCLUDE_DRIVERS_UART_MUX_H_

/**
 * @brief UART Mux Interface
 * @defgroup uart_mux_interface UART Mux Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gsm_dlci;

/**
 * @typedef uart_mux_attach_cb_t
 *
 * @brief Define the user callback function which is called when
 * the UART mux is attached properly.
 *
 * @param mux UART mux device
 * @param dlci_address DLCI id for the virtual muxing channel
 * @param connected True if DLCI is connected, false otherwise.
 * @param user_data Arbitrary user data.
 */
typedef void (*uart_mux_attach_cb_t)(const struct device *mux,
				     int dlci_address,
				     bool connected, void *user_data);

/** @brief UART mux driver API structure. */
__subsystem struct uart_mux_driver_api {
	/**
	 * The uart_driver_api must be placed in first position in this
	 * struct so that we are compatible with uart API. Note that currently
	 * not all of the UART API functions are implemented.
	 */
	struct uart_driver_api uart_api;

	/**
	 * Attach the mux to this UART. The API will call the callback after
	 * the DLCI is created or not.
	 */
	int (*attach)(const struct device *mux, const struct device *uart,
		      int dlci_address, uart_mux_attach_cb_t cb,
		      void *user_data);
};

/**
 * @brief Attach physical/real UART to UART muxing device.
 *
 * @param mux UART mux device structure.
 * @param uart Real UART device structure.
 * @param dlci_address DLCI id for the virtual muxing channel
 * @param cb Callback is called when the DLCI is ready and connected
 * @param user_data Caller supplied optional data
 *
 * @retval 0 No errors, the attachment was successful
 * @retval <0 Error
 */
static inline int uart_mux_attach(const struct device *mux,
				  const struct device *uart,
				  int dlci_address, uart_mux_attach_cb_t cb,
				  void *user_data)
{
	const struct uart_mux_driver_api *api =
		(const struct uart_mux_driver_api *)mux->api;

	return api->attach(mux, uart, dlci_address, cb, user_data);
}

/**
 * @brief Get UART related to a specific DLCI channel
 *
 * @param dlci_address DLCI address, value >0 and <63
 *
 * @return UART device if found, NULL otherwise
 */
__syscall const struct device *uart_mux_find(int dlci_address);

/**
 * @brief Allocate muxing UART device.
 *
 * @details This will return next available uart mux driver that will mux the
 * data when read or written. This device corresponds to one DLCI channel.
 * User must first call this to allocate the DLCI and then call the attach
 * function to fully enable the muxing.
 *
 * @retval device New UART device that will automatically mux data sent to it.
 * @retval NULL if error
 */
const struct device *uart_mux_alloc(void);

/**
 * @typedef uart_mux_cb_t
 * @brief Callback used while iterating over UART muxes
 *
 * @param uart Pointer to UART device where the mux is running
 * @param dev Pointer to UART mux device
 * @param dlci_address DLCI channel id this UART is muxed
 * @param user_data A valid pointer to user data or NULL
 */
typedef void (*uart_mux_cb_t)(const struct device *uart,
			      const struct device *dev,
			      int dlci_address, void *user_data);

/**
 * @brief Go through all the UART muxes and call callback
 * for each of them
 *
 * @param cb User-supplied callback function to call
 * @param user_data User specified data
 */
void uart_mux_foreach(uart_mux_cb_t cb, void *user_data);

/**
 * @brief Disable the mux.
 *
 * @details Disable does not re-instate whatever ISRs and configs were present
 * before the mux was enabled. This must be done by the user.
 *
 * @param dev UART mux device pointer
 */
void uart_mux_disable(const struct device *dev);

/**
 * @brief Enable the mux.
 *
 * @details Enables the correct ISRs for the UART mux.
 *
 * @param dev UART mux device pointer
 */
void uart_mux_enable(const struct device *dev);

#ifdef __cplusplus
}
#endif

#include <syscalls/uart_mux.h>

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_UART_MUX_H_ */
