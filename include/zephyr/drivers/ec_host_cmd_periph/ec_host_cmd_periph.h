/*
 * Copyright (c) 2020 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for Host Command Peripherals that respond to host commands
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HOST_CMD_PERIPH_H_
#define ZEPHYR_INCLUDE_DRIVERS_HOST_CMD_PERIPH_H_

#include <zephyr/sys/__assert.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Host Command Peripherals API
 * @defgroup ec_host_cmd_periph Host Command Peripherals API
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Context for host command peripheral and framework to pass rx data
 */
struct ec_host_cmd_periph_rx_ctx {
	/** Buffer written to by device (when dev_owns) and read from by
	 *  command framework and handler (when handler_owns). Buffer is owned
	 *  by devices and lives as long as device is valid. Device will never
	 *  read from this buffer (for security reasons).
	 */
	uint8_t *buf;
	/** Number of bytes written to @a buf by device (when dev_owns). */
	size_t *len;
	/** Device will take when it needs to write to @a buf and @a size. */
	struct k_sem *dev_owns;
	/** Handler will take so it can read @a buf and @a size */
	struct k_sem *handler_owns;
};

/**
 * @brief Context for host command peripheral and framework to pass tx data
 */
struct ec_host_cmd_periph_tx_buf {
	/** Data to write to the host */
	void *buf;
	/** Number of bytes to write from @a buf */
	size_t len;
};

typedef int (*ec_host_cmd_periph_api_init)(
	const struct device *dev, struct ec_host_cmd_periph_rx_ctx *rx_ctx);

typedef int (*ec_host_cmd_periph_api_send)(
	const struct device *dev,
	const struct ec_host_cmd_periph_tx_buf *tx_buf);

__subsystem struct ec_host_cmd_periph_api {
	ec_host_cmd_periph_api_init init;
	ec_host_cmd_periph_api_send send;
};

/**
 * @brief Initialize a host command device
 *
 * This routine initializes a host command device, prior to its first use. The
 * receive context object are an output of this function and are valid
 * for the lifetime of this device. The RX context is used by the client to
 * receive data from the host.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param rx_ctx [out] The receiving context object that are valid for the
 *               lifetime of the device. These objects are used to receive data
 *               from the driver when the host send data.
 *
 * @retval 0 if successful
 */
static inline int
ec_host_cmd_periph_init(const struct device *dev,
			       struct ec_host_cmd_periph_rx_ctx *rx_ctx)
{
	const struct ec_host_cmd_periph_api *api =
		(const struct ec_host_cmd_periph_api *)dev->api;

	return api->init(dev, rx_ctx);
}

/**
 * @brief Sends the specified data to the host
 *
 * Sends the data specified in @a tx_buf to the host over the host communication
 * bus.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param tx_buf The data to transmit to the host.
 *
 * @retval 0 if successful
 */
static inline int ec_host_cmd_periph_send(
	const struct device *dev,
	const struct ec_host_cmd_periph_tx_buf *tx_buf)
{
	const struct ec_host_cmd_periph_api *api =
		(const struct ec_host_cmd_periph_api *)dev->api;

	return api->send(dev, tx_buf);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_HOST_CMD_PERIPH_H_ */
