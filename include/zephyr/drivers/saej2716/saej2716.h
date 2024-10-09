/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SAEJ2716 Single Edge Nibble Transmission (SENT) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SAEJ2716_H_
#define ZEPHYR_INCLUDE_DRIVERS_SAEJ2716_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SAEJ2716 Interface
 * @defgroup saej2716_interface SAEJ2716 Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief SAEJ2716 frame type
 */
enum saej2716_frame_type {
	SAEJ2716_SHORT_SERIAL_FRAME,
	SAEJ2716_ENHANCED_SERIAL_FRAME_4_BIT_ID,
	SAEJ2716_ENHANCED_SERIAL_FRAME_8_BIT_ID,
	SAEJ2716_FAST_FRAME
};

/**
 * @brief SAEJ2716 frame structure
 */
struct saej2716_frame {
	enum saej2716_frame_type type;

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
typedef void (*saej2716_rx_frame_callback_t)(const struct device *dev, uint8_t channel,
					     uint32_t num_frame, void *user_data);

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Callback API upon starting receive SAEJ2716
 * See @a saej2716_start_rx() for argument description
 */
typedef int (*saej2716_start_rx_t)(const struct device *dev, uint8_t channel);

/**
 * @brief Callback API upon stopping receive SAEJ2716
 * See @a saej2716_stop_rx() for argument description
 */
typedef int (*saej2716_stop_rx_t)(const struct device *dev, uint8_t channel);

/**
 * @brief RX Callback configuration
 */
struct saej2716_rx_callback_config {
	saej2716_rx_frame_callback_t callback;
	struct saej2716_frame *frame;
	uint32_t max_num_frame;
	void *user_data;
};

struct saej2716_rx_callback_configs {
	struct saej2716_rx_callback_config *serial;
	struct saej2716_rx_callback_config *fast;
};

/**
 * @brief Callback API upon adding RX callback
 * See @a saej2716_register_callback() for argument description
 */
typedef int (*saej2716_register_callback_t)(const struct device *dev, uint8_t channel,
					    struct saej2716_rx_callback_configs callback_configs);

__subsystem struct saej2716_driver_api {
	saej2716_start_rx_t start_rx;
	saej2716_stop_rx_t stop_rx;
	saej2716_register_callback_t register_callback;
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
__syscall int saej2716_start_rx(const struct device *dev, uint8_t channel);

static inline int z_impl_saej2716_start_rx(const struct device *dev, uint8_t channel)
{
	const struct saej2716_driver_api *api = (const struct saej2716_driver_api *)dev->api;

	if (api->start_rx) {
		return api->start_rx(dev, channel);
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
__syscall int saej2716_stop_rx(const struct device *dev, uint8_t channel);

static inline int z_impl_saej2716_stop_rx(const struct device *dev, uint8_t channel)
{
	const struct saej2716_driver_api *api = (const struct saej2716_driver_api *)dev->api;

	if (api->stop_rx) {
		return api->stop_rx(dev, channel);
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
__syscall int saej2716_register_callback(const struct device *dev, uint8_t channel,
					 struct saej2716_rx_callback_configs callback_configs);

static inline int
z_impl_saej2716_register_callback(const struct device *dev, uint8_t channel,
				  struct saej2716_rx_callback_configs callback_configs)
{
	const struct saej2716_driver_api *api = (const struct saej2716_driver_api *)dev->api;

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

#include <zephyr/syscalls/saej2716.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_SAEJ2716_H_ */
