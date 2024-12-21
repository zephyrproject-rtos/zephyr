/*
 * Copyright 2024 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UART_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_UART_EMUL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/slist.h>
#include <zephyr/types.h>

/**
 * @file
 *
 * @brief Public APIs for the UART device emulation drivers.
 */

/**
 * @brief UART Emulation Interface
 * @defgroup uart_emul_interface UART Emulation Interface
 * @ingroup io_emulators
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

struct uart_emul_device_api;

/**
 * @brief Define the emulation callback function signature
 *
 * @param dev UART device instance
 * @param size Number of available bytes in TX buffer
 * @param target pointer to emulation context
 */
typedef void (*uart_emul_device_tx_data_ready_t)(const struct device *dev, size_t size,
						 const struct emul *target);

/** Node in a linked list of emulators for UART devices */
struct uart_emul {
	sys_snode_t node;
	/** Target emulator - REQUIRED for all emulated bus nodes of any type */
	const struct emul *target;
	/** API provided for this device */
	const struct uart_emul_device_api *api;
};

/** Definition of the emulator API */
struct uart_emul_device_api {
	uart_emul_device_tx_data_ready_t tx_data_ready;
};

/**
 * Register an emulated device on the controller
 *
 * @param dev Device that will use the emulator
 * @param emul UART emulator to use
 * @return 0 indicating success
 */
int uart_emul_register(const struct device *dev, struct uart_emul *emul);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_UART_EMUL_H_ */
