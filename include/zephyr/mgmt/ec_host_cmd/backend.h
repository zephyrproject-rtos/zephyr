/*
 * Copyright (c) 2020 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for Host Command backends that respond to host commands
 * @ingroup ec_host_cmd_backend
 */

#ifndef ZEPHYR_INCLUDE_MGMT_EC_HOST_CMD_BACKEND_H_
#define ZEPHYR_INCLUDE_MGMT_EC_HOST_CMD_BACKEND_H_

/**
 * @brief Interface to EC Host Command backends
 * @defgroup ec_host_cmd_backend Backends
 * @ingroup ec_host_cmd_interface
 * @{
 */

#include <zephyr/sys/__assert.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief EC Host Command Backend
 */
struct ec_host_cmd_backend {
	/** API provided by the backend. */
	const struct ec_host_cmd_backend_api *api;
	/** Context for the backend. */
	void *ctx;
};

/**
 * @brief Context for host command backend and handler to pass rx data.
 */
struct ec_host_cmd_rx_ctx {
	/**
	 * Buffer to hold received data. The buffer is provided by the handler if
	 * CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_SIZE > 0. Otherwise, the backend should provide
	 * the buffer on its own and overwrites @a buf pointer and @a len_max
	 * in the init function.
	 */
	uint8_t *buf;
	/** Number of bytes written to @a buf by backend. */
	size_t len;
	/** Maximum number of bytes to receive with one request packet. */
	size_t len_max;
};

/**
 * @brief Context for host command backend and handler to pass tx data
 */
struct ec_host_cmd_tx_buf {
	/**
	 * Data to write to the host. The buffer is provided by the handler if
	 * CONFIG_EC_HOST_CMD_HANDLER_TX_BUFFER_SIZE > 0. Otherwise, the backend should provide
	 * the buffer on its own and overwrites @a buf pointer and @a len_max
	 * in the init function.
	 */
	void *buf;
	/** Number of bytes to write from @a buf. */
	size_t len;
	/** Maximum number of bytes to send with one response packet. */
	size_t len_max;
};

/**
 * @brief Initialize a host command backend
 *
 * This routine initializes a host command backend. It includes initialization
 * a device used to communication and setting up buffers.
 * This function is called by the ec_host_cmd_init function.
 *
 * @param[in]     backend Pointer to the backend structure for the driver instance.
 * @param[in,out] rx_ctx  Pointer to the receive context object. These objects are used to receive
 *                        data from the driver when the host sends data. The buf member can be
 *                        assigned by the backend.
 * @param[in,out] tx      Pointer to the transmit buffer object. The buf and len_max members can be
 *                        assigned by the backend. These objects are used to send data by the
 *                        backend with the ec_host_cmd_backend_api_send function.
 *
 * @retval 0 if successful
 */
typedef int (*ec_host_cmd_backend_api_init)(const struct ec_host_cmd_backend *backend,
					    struct ec_host_cmd_rx_ctx *rx_ctx,
					    struct ec_host_cmd_tx_buf *tx);

/**
 * @brief Sends data to the host
 *
 * Sends data from tx buf that was passed via ec_host_cmd_backend_api_init
 * function.
 *
 * @param backend Pointer to the backed to send data.
 *
 * @retval 0 if successful.
 */
typedef int (*ec_host_cmd_backend_api_send)(const struct ec_host_cmd_backend *backend);

struct ec_host_cmd_backend_api {
	ec_host_cmd_backend_api_init init;
	ec_host_cmd_backend_api_send send;
};

/**
 * @brief Get the eSPI Host Command backend pointer
 *
 * Get the eSPI pointer backend and pass a pointer to eSPI device instance that will be used for
 * the Host Command communication.
 *
 * @param dev Pointer to eSPI device instance.
 *
 * @retval The eSPI backend pointer.
 */
struct ec_host_cmd_backend *ec_host_cmd_backend_get_espi(const struct device *dev);

/**
 * @brief Get the SHI NPCX Host Command backend pointer
 *
 * @retval the SHI NPCX backend pointer
 */
struct ec_host_cmd_backend *ec_host_cmd_backend_get_shi_npcx(void);

/**
 * @brief Get the SHI ITE Host Command backend pointer
 *
 * @retval the SHI ITE backend pointer
 */
struct ec_host_cmd_backend *ec_host_cmd_backend_get_shi_ite(void);

/**
 * @brief Get the UART Host Command backend pointer
 *
 * Get the UART pointer backend and pass a pointer to UART device instance that will be used for
 * the Host Command communication.
 *
 * @param dev Pointer to UART device instance.
 *
 * @retval The UART backend pointer.
 */
struct ec_host_cmd_backend *ec_host_cmd_backend_get_uart(const struct device *dev);

/**
 * @brief Get the SPI Host Command backend pointer
 *
 * Get the SPI pointer backend and pass a chip select pin that will be used for the Host Command
 * communication.
 *
 * @param cs Chip select pin..
 *
 * @retval The SPI backend pointer.
 */
struct ec_host_cmd_backend *ec_host_cmd_backend_get_spi(struct gpio_dt_spec *cs);

/**
 * @brief Signal event over USB
 *
 * Signal event using USB interrupt endpoint. It informs host that there is a pending event that has
 * to be handled. The function performs remote wake-up if needed.
 */
void ec_host_cmd_backend_usb_trigger_event(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MGMT_EC_HOST_CMD_EC_HOST_CMD_BACKEND_H_ */
