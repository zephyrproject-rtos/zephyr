/*
 * Copyright 2024 NXP
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
 * @brief SENT frame structure
 */
struct sent_frame {
	union {
		/* SENT slow message frame */
		struct {
			uint16_t data;
			uint8_t id;
			uint8_t crc;
			uint32_t timestamp;
		} smsg;

		/* SENT fast message frame */
		struct {
			uint32_t data;
			uint32_t timestamp;
			uint8_t crc;
		} fmsg;
	};
};

/**
 * @brief SENT state
 */
enum sent_state {
	SENT_STATE_SMSG_RECEIVED,
	SENT_STATE_FMSG_RECEIVED,
	SENT_STATE_SMSG_ERR,
	SENT_STATE_FMSG_ERR,
};

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Defines the application callback handler function signature for receiving.
 *
 * @param dev        Pointer to the device structure for the driver instance.
 * @param channel    The hardware channel of the driver instance.
 * @param frame      Received frame.
 * @param state      State of SENT.
 * @param user_data  User data provided when the filter was added.
 */
typedef void (*sent_rx_callback_t)(const struct device *dev, uint8_t channel,
				   struct sent_frame *frame, enum sent_state state,
				   void *user_data);

/**
 * @brief Callback API upon starting SENT
 * See @a sent_start() for argument description
 */
typedef int (*sent_start_t)(const struct device *dev, uint8_t channel);

/**
 * @brief Callback API upon stopping SENT
 * See @a sent_stop() for argument description
 */
typedef int (*sent_stop_t)(const struct device *dev, uint8_t channel);

/**
 * @brief Callback API upon adding RX callback
 * See @a sent_add_rx_callback() for argument description
 */
typedef int (*sent_add_rx_callback_t)(const struct device *dev, uint8_t channel,
				      sent_rx_callback_t callback, void *user_data);

__subsystem struct sent_driver_api {
	sent_start_t start;
	sent_stop_t stop;
	sent_add_rx_callback_t add_rx_callback;
};

/** @endcond */

/**
 * @brief Start SENT
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The hardware channel of the driver instance.
 * @retval 0 if successful.
 * @retval -EINVAL if an invalid channel is given.
 * @retval -EALREADY if the device is already started.
 * @retval -EIO General input/output error, failed to start device.
 */
__syscall int sent_start(const struct device *dev, uint8_t channel);

static inline int z_impl_sent_start(const struct device *dev, uint8_t channel)
{
	const struct sent_driver_api *api = (const struct sent_driver_api *)dev->api;

	if (api->start) {
		return api->start(dev, channel);
	}

	return -ENOSYS;
}

/**
 * @brief Stop SENT
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The hardware channel of the driver instance.
 * @retval 0 if successful.
 * @retval -EINVAL if an invalid channel is given.
 * @retval -EALREADY if the device is already stopped.
 * @retval -EIO General input/output error, failed to stop device.
 */
__syscall int sent_stop(const struct device *dev, uint8_t channel);

static inline int z_impl_sent_stop(const struct device *dev, uint8_t channel)
{
	const struct sent_driver_api *api = (const struct sent_driver_api *)dev->api;

	if (api->stop) {
		return api->stop(dev, channel);
	}

	return -ENOSYS;
}

/**
 * @name Receiving SENT frames
 *
 * @{
 */

/**
 * @brief Add a callback function for receiving SENT frames
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param channel   The hardware channel of the driver instance.
 * @param callback  This function is called by SENT driver whenever a frame is received.
 * @param user_data User data to pass to callback function.
 * @retval 0 if successful.
 * @retval -EINVAL if an invalid channel is given.
 */
__syscall int sent_add_rx_callback(const struct device *dev, uint8_t channel,
				   sent_rx_callback_t callback, void *user_data);

static inline int z_impl_sent_add_rx_callback(const struct device *dev, uint8_t channel,
					      sent_rx_callback_t callback, void *user_data)
{
	const struct sent_driver_api *api = (const struct sent_driver_api *)dev->api;

	if (api->add_rx_callback) {
		return api->add_rx_callback(dev, channel, callback, user_data);
	}

	return -ENOSYS;
}

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/sent.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENT_H_ */
