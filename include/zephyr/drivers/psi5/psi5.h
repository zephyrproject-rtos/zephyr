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
 * @brief Interfaces for Peripheral Sensor Interface (PSI5).
 * @defgroup psi5_interface PSI5
 * @since 4.2
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief PSI5 frame type
 */
enum psi5_frame_type {
	/** Serial message frame with 4-bit message ID */
	PSI5_SERIAL_FRAME_4_BIT_ID,
	/** Serial message frame with 8-bit message ID */
	PSI5_SERIAL_FRAME_8_BIT_ID,
	/** Data frame */
	PSI5_DATA_FRAME
};

/**
 * @brief PSI5 frame structure
 */
struct psi5_frame {
	/** Type of PSI5 frame */
	enum psi5_frame_type type;

	union {
		/** Message data */
		uint32_t data;

		/**
		 * @brief Serial message
		 */
		struct {
			/** Serial message ID */
			uint8_t id;
			/** Serial message data */
			uint16_t data;
		} serial;
	};

	/** Timestamp of when the frame was captured */
	uint32_t timestamp;
	/** CRC checksum for message integrity validation */
	uint8_t crc;
	/** Slot Number */
	uint8_t slot_number;
};

/**
 * @brief Defines the application callback handler function signature for sending.
 *
 * @param dev        Pointer to the device structure for the driver instance.
 * @param channel    The hardware channel of the driver instance.
 * @param status     PSI5 status (0: transmission completed successfully,
 *                   -EIO: transmission error occurred).
 * @param user_data  User data provided when the frame was sent.
 */
typedef void (*psi5_tx_callback_t)(const struct device *dev, uint8_t channel, int status,
				   void *user_data);

/**
 * @brief Defines the application callback handler function signature for receiving frame.
 *
 * @param dev        Pointer to the device structure for the driver instance.
 * @param channel    The hardware channel of the driver instance.
 * @param num_frame  Number of received frame.
 * @param user_data  User data provided when receiving frame.
 */
typedef void (*psi5_rx_frame_callback_t)(const struct device *dev, uint8_t channel,
					 uint32_t num_frame, void *user_data);

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
 * @brief Configuration structure for RX callback
 */
struct psi5_rx_callback_config {
	/** Callback function invoked on frame reception */
	psi5_rx_frame_callback_t callback;
	/** Pointer to the buffer for storing received frames */
	struct psi5_frame *frame;
	/** Maximum number of frames to store */
	uint32_t max_num_frame;
	/** Pointer to user data passed to the callback */
	void *user_data;
};

/**
 * @brief Composite configuration structure for RX callback registration
 */
struct psi5_rx_callback_configs {
	/** Configuration for the serial message callback */
	struct psi5_rx_callback_config *serial_frame;
	/** Configuration for the data message callback */
	struct psi5_rx_callback_config *data_frame;
};

/**
 * @brief Callback API upon adding RX callback
 * See @a psi5_register_callback() for argument description
 */
typedef int (*psi5_register_callback_t)(const struct device *dev, uint8_t channel,
					struct psi5_rx_callback_configs callback_configs);

__subsystem struct psi5_driver_api {
	psi5_start_sync_t start_sync;
	psi5_stop_sync_t stop_sync;
	psi5_send_t send;
	psi5_register_callback_t register_callback;
};

/** @endcond */

/**
 * @brief Start the sync pulse generator on a specific channel
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The hardware channel of the driver instance.
 * @retval 0 successful.
 * @retval -EINVAL invalid channel.
 * @retval -EALREADY device is already started.
 * @retval -EIO general input/output error, failed to start device.
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
 * @retval 0 successful.
 * @retval -EINVAL invalid channel.
 * @retval -EALREADY device is already started.
 * @retval -EIO general input/output error, failed to stop device.
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
 * The channel must be configured to synchronous mode and can only begin transmission after
 * the sync pulse generator has started.
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
 * @retval 0 successful.
 * @retval -EINVAL invalid channel.
 * @retval -ENOTSUP unsupported parameter was passed to the function.
 * @retval -ENETDOWN stopped state.
 * @retval -EIO general transmit error occurred.
 * @retval -EAGAIN timeout.
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
 * The callback must be registered before the sync pulse generator started when the channel
 * is configured to synchronous mode.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The hardware channel of the driver instance.
 * @param callback_configs The callback configurations.
 * @retval 0 successful.
 * @retval -EINVAL invalid channel.
 */
__syscall int psi5_register_callback(const struct device *dev, uint8_t channel,
				     struct psi5_rx_callback_configs callback_configs);

static inline int z_impl_psi5_register_callback(const struct device *dev, uint8_t channel,
						struct psi5_rx_callback_configs callback_configs)
{
	const struct psi5_driver_api *api = (const struct psi5_driver_api *)dev->api;

	if (api->register_callback) {
		return api->register_callback(dev, channel, callback_configs);
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
