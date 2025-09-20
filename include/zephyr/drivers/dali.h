/*
 * Copyright (c) 2025 by Sven Hädrich <sven.haedrich@sevenlab.de>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Digital Addressable Lighting Interface (DALI) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DALI_H_
#define ZEPHYR_INCLUDE_DRIVERS_DALI_H_

#include <zephyr/device.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/__assert.h>
#include <errno.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DALI interface
 * @{
 */

/**
 * @name DALI frame definitions
 * @{
 */

/**
 * @brief DALI frame types
 */
enum dali_event_type {
	DALI_EVENT_NONE,        /**< no event (write/receive) */
	DALI_FRAME_CORRUPT,     /**< corrupt frame (write/receive)*/
	DALI_FRAME_BACKWARD,    /**< backward frame (write/receive) */
	DALI_FRAME_GEAR,        /**< forward 16bit gear frame (write/receive)*/
	DALI_FRAME_DEVICE,      /**< forward 24bit device frame (write/receive) */
	DALI_FRAME_FIRMWARE,    /**< forward 32bit firmware frame (write/receive) */
	DALI_EVENT_BUS_FAILURE, /**< detected a bus failure (receive) */
	DALI_EVENT_BUS_IDLE,    /**< detected that bus is idle again after failure (receive) */
};

/**
 * @brief DALI frame structure
 */
struct dali_frame {
	uint32_t data;                   /**< LSB aligned payload */
	enum dali_event_type event_type; /**<  event type of frame */
};

/** @} */

/**
 * @brief DALI callback function typedefs
 * @{
 */

/**
 * @brief Callback API upon receiving a DALI frame.
 * See @a dali_receive() for argument description.
 */
typedef void (*dali_rx_callback_t)(const struct device *dev, struct dali_frame frame,
				   void *user_data);

/**
 * @brief Callback API upon transmitting a DALI frame.
 * See @a dali_send() for argument description.
 */
typedef void (*dali_tx_callback_t)(const struct device *dev, int error, void *user_data);

/** @} */

__subsystem struct dali_driver_api {
	int (*receive)(const struct device *dev, dali_rx_callback_t callback, void *user_data);
	int (*send)(const struct device *dev, const struct dali_frame *frame,
		    dali_tx_callback_t callback, void *user_data);
	void (*abort)(const struct device *dev);
};

/**
 * @brief Set a callback function for frames from the DALI bus.
 *
 * Set a callback function for the DALI driver. Whenever a DALI frame
 * is fully received, or a relevant bus event is detected, the callback
 * function is called in the driver´s workqueue context.
 * Only a single callback function per driver instance is supported.
 *
 * @note The callback function is invoked at the end of the stop condition.
 *       Interframe timings need to consider this.
 *
 * @param dev       Pointer to the DALI device structure for the driver instance.
 * @param callback  This funtion is called by the DALI driver whenever a frame
 *                  is received or a relevant event detected.
 * @param user_data User data to pass to callback function.
 *
 * @retval 0        success (always)
 */
static inline int dali_receive(const struct device *dev, dali_rx_callback_t callback,
			       void *user_data)
{
	const struct dali_driver_api *api = (const struct dali_driver_api *)dev->api;

	return api->receive(dev, callback, user_data);
}

/**
 * @brief Send a frame on the DALI bus.
 *
 * Immediately start sending a DALI frame to the DALI bus.
 *
 * @param dev       Pointer to the DALI device structure for the driver instance.
 * @param frame     DALI frame to transmit
 * @param callback  Optional callback for when the frame was sent or a
 *                  transmission error is detected. If ``NULL``, this function
 *                  is blocking until frame is completely sent.
 *                  The callback function is called in the driver´s workqueue
 *                  context. For the meaning of the callback´s
 *                  `err` parameter refer to the return codes for `dali_send`.
 * @param user_data User data to pass to callback function.
 *
 * @retval 0          if successful.
 * @retval -EBUSY     if a frame is in transit
 * @retval -EINVAL    if invalid parameter was passed to function
 * @retval -ECANCELED if transmission yielded to dominant frame
 * @retval -ECOMM     if collision occured during transmission
 * @retval -EAGAIN    if bus is not available
 */
static inline int dali_send(const struct device *dev, const struct dali_frame *frame,
			    dali_tx_callback_t callback, void *user_data)
{
	const struct dali_driver_api *api = (const struct dali_driver_api *)dev->api;

	return api->send(dev, frame, callback, user_data);
}

/**
 * @brief Abort sending.
 *
 * This will abort all pending or ongoing DALI frame transmissions. Transmission
 * will be aborted, regardless of bit timings, at the shortest possible time.
 *
 * @note This can result in corrupt a frame.
 *
 * @param[in] dev         DALI device
 */
static inline void dali_abort(const struct device *dev)
{
	const struct dali_driver_api *api = (const struct dali_driver_api *)dev->api;

	api->abort(dev);
}

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DALI_H_ */
