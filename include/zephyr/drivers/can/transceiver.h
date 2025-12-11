/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup can_transceiver
 * @brief Header file for CAN transceiver driver API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CAN_TRANSCEIVER_H_
#define ZEPHYR_INCLUDE_DRIVERS_CAN_TRANSCEIVER_H_

#include <zephyr/drivers/can.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaces for CAN transceivers
 * @defgroup can_transceiver CAN Transceiver
 * @since 3.1
 * @version 0.1.0
 * @ingroup can_interface
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */

/**
 * @brief Callback API upon enabling CAN transceiver
 * See @a can_transceiver_enable() for argument description
 */
typedef int (*can_transceiver_enable_t)(const struct device *dev, can_mode_t mode);

/**
 * @brief Callback API upon disabling CAN transceiver
 * See @a can_transceiver_disable() for argument description
 */
typedef int (*can_transceiver_disable_t)(const struct device *dev);

__subsystem struct can_transceiver_driver_api {
	can_transceiver_enable_t enable;
	can_transceiver_disable_t disable;
};

/** @endcond */

/**
 * @brief Enable CAN transceiver
 *
 * Enable the CAN transceiver.
 *
 * @note The CAN transceiver is controlled by the CAN controller driver and
 *       should not normally be controlled by the application.
 *
 * @see can_start()
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mode Operation mode.
 * @retval 0 If successful.
 * @retval -EIO General input/output error, failed to enable device.
 */
static inline int can_transceiver_enable(const struct device *dev, can_mode_t mode)
{
	return DEVICE_API_GET(can_transceiver, dev)->enable(dev, mode);
}

/**
 * @brief Disable CAN transceiver
 *
 * Disable the CAN transceiver.

 * @note The CAN transceiver is controlled by the CAN controller driver and
 *       should not normally be controlled by the application.
 *
 * @see can_stop()
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @retval 0 If successful.
 * @retval -EIO General input/output error, failed to disable device.
 */
static inline int can_transceiver_disable(const struct device *dev)
{
	return DEVICE_API_GET(can_transceiver, dev)->disable(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAN_TRANSCEIVER_H_ */
