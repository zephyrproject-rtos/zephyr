/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Single Edge Nibble Transmission (SENT) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENT_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENT_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SENT Interface
 * @defgroup sent_interface SENT Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief SENT frame type
 */
enum sent_frame_type {
	SENT_SHORT_SERIAL_FRAME,
	SENT_ENHANCED_SERIAL_FRAME_4_BIT_ID,
	SENT_ENHANCED_SERIAL_FRAME_8_BIT_ID,
	SENT_FAST_FRAME
};

/**
 * @brief SENT frame structure
 */
struct sent_frame {
	enum sent_frame_type type;

	union {
		struct {
			uint8_t id;
			uint16_t data;
		} serial;

		struct {
			uint32_t data;
		} fast;
	};

	uint32_t timestamp;
	uint8_t crc;
};

/**
 * @brief Defines the application callback handler function signature for receiving frame.
 *
 * @param dev        Pointer to the device structure for the driver instance.
 * @param channel    The hardware channel of the driver instance.
 * @param num_frame  Number of received frame.
 * @param user_data  User data provided when receiving frame.
 */
typedef void (*sent_rx_frame_callback_t)(const struct device *dev, uint8_t channel,
					 uint32_t num_frame, void *user_data);

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Callback API upon starting receive frame
 * See @a sent_start_listening() for argument description
 */
typedef int (*sent_start_listening_t)(const struct device *dev, uint8_t channel);

/**
 * @brief Callback API upon stopping receive frame
 * See @a sent_stop_listening() for argument description
 */
typedef int (*sent_stop_listening_t)(const struct device *dev, uint8_t channel);

/**
 * @brief RX Callback configuration
 */
struct sent_rx_callback_config {
	sent_rx_frame_callback_t callback;
	struct sent_frame *frame;
	uint32_t max_num_frame;
	void *user_data;
};

struct sent_rx_callback_configs {
	struct sent_rx_callback_config *serial;
	struct sent_rx_callback_config *fast;
};

/**
 * @brief Callback API upon adding RX callback
 * See @a sent_register_callback() for argument description
 */
typedef int (*sent_register_callback_t)(const struct device *dev, uint8_t channel,
					struct sent_rx_callback_configs callback_configs);

__subsystem struct sent_driver_api {
	sent_start_listening_t start_listening;
	sent_stop_listening_t stop_listening;
	sent_register_callback_t register_callback;
};

/** @endcond */

/**
 * @brief Enable a specific channel to start receiving from the bus
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The hardware channel of the driver instance.
 * @retval 0 if successful.
 * @retval -EINVAL if an invalid channel is given.
 * @retval -EALREADY if the device is already started.
 * @retval -EIO General input/output error, failed to start device.
 */
__syscall int sent_start_listening(const struct device *dev, uint8_t channel);

static inline int z_impl_sent_start_listening(const struct device *dev, uint8_t channel)
{
	const struct sent_driver_api *api = (const struct sent_driver_api *)dev->api;

	if (api->start_listening) {
		return api->start_listening(dev, channel);
	}

	return -ENOSYS;
}

/**
 * @brief Disable a specific channel to stop receiving from the bus
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The hardware channel of the driver instance.
 * @retval 0 if successful.
 * @retval -EINVAL if an invalid channel is given.
 * @retval -EALREADY if the device is already stopped.
 * @retval -EIO General input/output error, failed to stop device.
 */
__syscall int sent_stop_listening(const struct device *dev, uint8_t channel);

static inline int z_impl_sent_stop_listening(const struct device *dev, uint8_t channel)
{
	const struct sent_driver_api *api = (const struct sent_driver_api *)dev->api;

	if (api->stop_listening) {
		return api->stop_listening(dev, channel);
	}

	return -ENOSYS;
}

/**
 * @brief Add a callback function to handle messages received for a specific channel
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The hardware channel of the driver instance.
 * @param callback_configs The callback configurations.
 * @retval 0 if successful.
 * @retval -EINVAL if an invalid channel is given.
 */
__syscall int sent_register_callback(const struct device *dev, uint8_t channel,
				     struct sent_rx_callback_configs callback_configs);

static inline int z_impl_sent_register_callback(const struct device *dev, uint8_t channel,
						struct sent_rx_callback_configs callback_configs)
{
	const struct sent_driver_api *api = (const struct sent_driver_api *)dev->api;

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

#include <zephyr/syscalls/sent.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENT_H_ */
