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
	SAEJ2716_SERIAL_FRAME,
	SAEJ2716_FAST_FRAME
};

/**
 * @brief SAEJ2716 frame structure
 */
struct saej2716_frame {
	enum saej2716_frame_type type;

	union {
		/* ID-Data field for serial frame */
		struct {
			uint16_t id;
			uint16_t data;
		} serial;

		/* Data field for fast frame */
		struct {
			uint32_t data;
		} fast;
	};

	uint32_t timestamp;
	uint8_t crc;
};

/**
 * @brief SAEJ2716 status
 */
enum saej2716_status {
	SAEJ2716_RX_SERIAL_FRAME,
	SAEJ2716_RX_FAST_FRAME,
	SAEJ2716_RX_ERR_SERIAL_FRAME,
	SAEJ2716_RX_ERR_FAST_FRAME,
};

/**
 * @brief Defines the application callback handler function signature for receiving serial frame.
 *
 * @param dev        Pointer to the device structure for the driver instance.
 * @param channel    The hardware channel of the driver instance.
 * @param frame      Received frame.
 * @param status     SAEJ2716 frame status.
 * @param user_data  User data provided when the filter was added.
 */
typedef void (*saej2716_rx_serial_frame_callback_t)(const struct device *dev, uint8_t channel,
						    struct saej2716_frame *frame,
						    enum saej2716_status status, void *user_data);

/**
 * @brief Defines the application callback handler function signature for receiving fast frame.
 *
 * @param dev        Pointer to the device structure for the driver instance.
 * @param channel    The hardware channel of the driver instance.
 * @param frame      Received frame.
 * @param status     SAEJ2716 frame status.
 * @param user_data  User data provided when the filter was added.
 */
typedef void (*saej2716_rx_fast_frame_callback_t)(const struct device *dev, uint8_t channel,
						  struct saej2716_frame *frame,
						  enum saej2716_status status, void *user_data);

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
 * @brief Callback API upon adding RX callback
 * See @a saej2716_add_rx_callback() for argument description
 */
typedef int (*saej2716_add_rx_callback_t)(const struct device *dev, uint8_t channel,
					  saej2716_rx_serial_frame_callback_t serial_callback,
					  saej2716_rx_fast_frame_callback_t fast_callback,
					  void *user_data);

__subsystem struct saej2716_driver_api {
	saej2716_start_rx_t start_rx;
	saej2716_stop_rx_t stop_rx;
	saej2716_add_rx_callback_t add_rx_callback;
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
 * @param dev       Pointer to the device structure for the driver instance.
 * @param channel   The hardware channel of the driver instance.
 * @param callback  This function is called by SAEJ2716 driver whenever a frame is received.
 * @param user_data User data to pass to callback function.
 * @retval 0 if successful.
 * @retval -EINVAL if an invalid channel is given.
 */
__syscall int saej2716_add_rx_callback(const struct device *dev, uint8_t channel,
				       saej2716_rx_serial_frame_callback_t serial_callback,
				       saej2716_rx_fast_frame_callback_t fast_callback,
				       void *user_data);

static inline int
z_impl_saej2716_add_rx_callback(const struct device *dev, uint8_t channel,
				saej2716_rx_serial_frame_callback_t serial_callback,
				saej2716_rx_fast_frame_callback_t fast_callback, void *user_data)
{
	const struct saej2716_driver_api *api = (const struct saej2716_driver_api *)dev->api;

	if (api->add_rx_callback) {
		return api->add_rx_callback(dev, channel, serial_callback, fast_callback,
					    user_data);
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
