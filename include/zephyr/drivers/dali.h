/*
 * Copyright (c) 2026 by Sven Hädrich <sven.haedrich@sevenlab.de>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup dali_interface
 * @brief Digital Addressable Lighting Interface (DALI) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DALI_H_
#define ZEPHYR_INCLUDE_DRIVERS_DALI_H_

/**
 * @defgroup dali_interface DALI
 * @since 4.4
 * @version 0.1.0
 * @ingroup io_interfaces
 * @brief DALI interface
 *
 * The DALI API provides support for low-level communication according to
 * IEC 62386.
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/__assert.h>
#include <errno.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DALI frame types
 */
enum dali_event_type {
	DALI_FRAME_CORRUPT,     /**< corrupt frame (transmit/receive) */
	DALI_FRAME_BACKWARD,    /**< backward 8bit frame (transmit/receive) */
	DALI_FRAME_GEAR,        /**< forward 16bit gear frame (transmit/receive) */
	DALI_FRAME_DEVICE,      /**< forward 24bit device frame (transmit/receive) */
	DALI_FRAME_FIRMWARE,    /**< forward 32bit firmware frame (transmit/receive) */
	DALI_EVENT_BUS_FAILURE, /**< detected a bus failure (receive only) */
	DALI_EVENT_BUS_IDLE,    /**< detected that bus is idle again after failure (receive only) */
};

/**
 * @brief DALI frame structure
 */
struct dali_frame {
	uint32_t data;                   /**< LSB aligned payload */
	enum dali_event_type event_type; /**< event type of frame */
};

/**
 * @brief Defines the application callback handler function signature for receiving a DALI frame.
 *
 * @param dev       Pointer to device structure for the driver instance.
 * @param frame     Received DALI frame.
 * @param user_data User data provided when the callback function was set.
 */
typedef void (*dali_receive_callback_t)(const struct device *dev, struct dali_frame frame,
					void *user_data);

/**
 * @brief Defines the application callback handler function signature for transmitting a DALI frame.
 *
 * @param dev       Pointer to device structure for the driver instance.
 * @param error     Error code for the transaction @see dali_transmit
 * @param user_data User data provided when the frame was sent.
 */
typedef void (*dali_transmit_callback_t)(const struct device *dev, int error, void *user_data);

/**
 * @def_driverbackendgroup{DALI,dali_interface}
 * @{
 */

/**
 * @typedef dali_api_set_receive_callback_t
 * @brief API for receiving DALI frames
 * @see dali_set_receive_callback() for argument description
 */
typedef int (*dali_api_set_receive_callback_t)(const struct device *dev,
					       dali_receive_callback_t callback, void *user_data);

/**
 * @typedef dali_api_transmit_t
 * @brief API for transmitting DALI frames
 * @see dali_transmit() for argument description
 */
typedef int (*dali_api_transmit_t)(const struct device *dev, const struct dali_frame *frame,
				   dali_transmit_callback_t callback, void *user_data);

/**
 * @driver_ops{DALI}
 */
__subsystem struct dali_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief dali_set_receive_callback
	 */
	dali_api_set_receive_callback_t set_receive_callback;
	/**
	 * @driver_ops_mandatory @copybrief dali_transmit
	 */
	dali_api_transmit_t transmit;
};
/**
 * @}
 */

/**
 * @brief Set a callback function that receives frames from the DALI bus.
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
 * @param callback  This function is called by the DALI driver whenever a frame
 *                  is received or a relevant event detected.
 * @param user_data User data to pass to callback function.
 *
 * @retval 0        on success
 * @retval -ENOSYS  if function not implemented
 */
__syscall int dali_set_receive_callback(const struct device *dev, dali_receive_callback_t callback,
					void *user_data);

static inline int z_impl_dali_set_receive_callback(const struct device *dev,
						   dali_receive_callback_t callback,
						   void *user_data)
{
	return DEVICE_API_GET(dali, dev)->set_receive_callback(dev, callback, user_data);
}

/**
 * @brief Transmit a frame on the DALI bus.
 *
 * Immediately start transmitting a DALI frame to the DALI bus.
 * In case of an error, the driver will not automatically attempt to re-transmit the frame.
 *
 * @param dev       Pointer to the DALI device structure for the driver instance.
 * @param frame     DALI frame to transmit
 * @param callback  Optional callback for when the frame was successfully transmitted
 *                  or a transmission error is detected.
 *                  The callback function is called in the driver´s workqueue
 *                  context. For the meaning of the callback´s
 *                  `err` parameter refer to the return codes for `dali_transmit`.
 * @param user_data User data to pass to callback function.
 *
 * @retval 0          on success
 * @retval -EBUSY     if a frame transmission is active
 * @retval -EINVAL    if invalid parameter was passed to function
 * @retval -ECANCELED if transmission yielded to a dominant frame
 * @retval -ECOMM     if collision occurred during transmission
 * @retval -EAGAIN    if bus is not available
 * @retval -ENOSYS    if function not implemented
 */
__syscall int dali_transmit(const struct device *dev, const struct dali_frame *frame,
			    dali_transmit_callback_t callback, void *user_data);

static inline int z_impl_dali_transmit(const struct device *dev, const struct dali_frame *frame,
				       dali_transmit_callback_t callback, void *user_data)
{
	return DEVICE_API_GET(dali, dev)->transmit(dev, frame, callback, user_data);
}

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/dali.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_DALI_H_ */
