/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Peripheral Sensor Interface (PSI5) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PSI5_H_
#define ZEPHYR_INCLUDE_DRIVERS_PSI5_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PSI5 Interface
 * @defgroup psi5_interface PSI5 Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief PSI5 frame structure
 */
struct psi5_frame {
	union {
		/* PSI5 recevice data frame */
		struct {
			uint32_t data;
			uint32_t timestamp;
			uint8_t crc;
			uint8_t slot_number;
		} data_frame;

		/* PSI5 serial message channel (SMC) data frame */
		struct {
			uint16_t data;
			uint8_t id;
			uint8_t crc;
			uint8_t slot_number;
		} smc_data_frame;
	};
};

/**
 * @brief PSI5 status
 */
enum psi5_status {
	PSI5_TX_DONE,
	PSI5_RX_DATA_FRAME,
	PSI5_RX_SMC_DATA_FRAME,
	PSI5_TX_ERR,
	PSI5_RX_ERR,
};

/**
 * @brief Defines the application callback handler function signature for sending.
 *
 * @param dev        Pointer to the device structure for the driver instance.
 * @param channel    The hardware channel of the driver instance.
 * @param status     PSI5 status.
 * @param user_data  User data provided when the frame was sent.
 */
typedef void (*psi5_tx_callback_t)(const struct device *dev, uint8_t channel,
				   enum psi5_status status, void *user_data);

/**
 * @brief Defines the application callback handler function signature for receiving.
 *
 * @param dev        Pointer to the device structure for the driver instance.
 * @param channel    The hardware channel of the driver instance.
 * @param frame      Received frame.
 * @param status     PSI5 status.
 * @param user_data  User data provided when the filter was added.
 */
typedef void (*psi5_rx_callback_t)(const struct device *dev, uint8_t channel,
				   struct psi5_frame *frame, enum psi5_status status,
				   void *user_data);

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Callback API upon starting sync PSI5
 * See @a psi5_start_sync() for argument description
 */
typedef int (*psi5_start_sync_t)(const struct device *dev, uint8_t channel);

/**
 * @brief Callback API upon stopping sync PSI5
 * See @a psi5_stop_sync() for argument description
 */
typedef int (*psi5_stop_sync_t)(const struct device *dev, uint8_t channel);

/**
 * @brief Callback API upon sending PSI5 frame
 * See @a psi5_send() for argument description
 */
typedef int (*psi5_send_t)(const struct device *dev, uint8_t channel, const uint64_t data,
			   k_timeout_t timeout, psi5_tx_callback_t callback, void *user_data);

/**
 * @brief Callback API upon adding RX callback
 * See @a psi5_add_rx_callback() for argument description
 */
typedef int (*psi5_add_rx_callback_t)(const struct device *dev, uint8_t channel,
				      psi5_rx_callback_t callback, void *user_data);

__subsystem struct psi5_driver_api {
	psi5_start_sync_t start_sync;
	psi5_stop_sync_t stop_sync;
	psi5_send_t send;
	psi5_add_rx_callback_t add_rx_callback;
};

/** @endcond */

/**
 * @brief Start the sync pulse generator on a specific channel
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The hardware channel of the driver instance.
 * @retval 0 if successful.
 * @retval -EINVAL if an invalid channel is given.
 * @retval -EALREADY if the device is already started.
 * @retval -EIO General input/output error, failed to start device.
 */
__syscall int psi5_start_sync(const struct device *dev, uint8_t channel);

static inline int z_impl_psi5_start_sync(const struct device *dev, uint8_t channel)
{
	const struct psi5_driver_api *api = (const struct psi5_driver_api *)dev->api;

	if (api->start_sync) {
		return api->start_sync(dev, channel);
	}

	return -ENOSYS;
}

/**
 * @brief Stop the sync pulse generator on a specific channel
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The hardware channel of the driver instance.
 * @retval 0 if successful.
 * @retval -EINVAL if an invalid channel is given.
 * @retval -EALREADY if the device is already stopped.
 * @retval -EIO General input/output error, failed to stop device.
 */
__syscall int psi5_stop_sync(const struct device *dev, uint8_t channel);

static inline int z_impl_psi5_stop_sync(const struct device *dev, uint8_t channel)
{
	const struct psi5_driver_api *api = (const struct psi5_driver_api *)dev->api;

	if (api->stop_sync) {
		return api->stop_sync(dev, channel);
	}

	return -ENOSYS;
}

/**
 * @brief Transmitting PSI5 data on a specific channel
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param channel   The hardware channel of the driver instance.
 * @param data      PSI5 data to transmit.
 * @param timeout   Timeout waiting for ready to transmit new data.
 * @param callback  Optional callback for when the frame was sent or a
 *                  transmission error occurred. If ``NULL``, this function is
 *                  blocking until frame is sent.
 * @param user_data User data to pass to callback function.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if an invalid channel is given.
 * @retval -ENOTSUP if an unsupported parameter was passed to the function.
 * @retval -ENETDOWN if PSI5 is in stopped state.
 * @retval -EIO if a general transmit error occurred.
 * @retval -EAGAIN on timeout.
 */
__syscall int psi5_send(const struct device *dev, uint8_t channel, const uint64_t data,
			k_timeout_t timeout, psi5_tx_callback_t callback, void *user_data);

static inline int z_impl_psi5_send(const struct device *dev, uint8_t channel, const uint64_t data,
				   k_timeout_t timeout, psi5_tx_callback_t callback,
				   void *user_data)
{
	const struct psi5_driver_api *api = (const struct psi5_driver_api *)dev->api;

	if (api->send) {
		return api->send(dev, channel, data, timeout, callback, user_data);
	}

	return -ENOSYS;
}

/**
 * @brief Add a callback function to handle messages received for a specific channel
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param channel   The hardware channel of the driver instance.
 * @param callback  This function is called by PSI5 driver whenever a frame is received.
 * @param user_data User data to pass to callback function.
 * @retval 0 if successful.
 * @retval -EINVAL if an invalid channel is given.
 */
__syscall int psi5_add_rx_callback(const struct device *dev, uint8_t channel,
				   psi5_rx_callback_t callback, void *user_data);

static inline int z_impl_psi5_add_rx_callback(const struct device *dev, uint8_t channel,
					      psi5_rx_callback_t callback, void *user_data)
{
	const struct psi5_driver_api *api = (const struct psi5_driver_api *)dev->api;

	if (api->add_rx_callback) {
		return api->add_rx_callback(dev, channel, callback, user_data);
	}

	return -ENOSYS;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/psi5.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_PSI5_H_ */
