/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CAN_TRANSCEIVER_H_
#define ZEPHYR_INCLUDE_DRIVERS_CAN_TRANSCEIVER_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CAN Transceiver Driver APIs
 * @defgroup can_transceiver CAN Transceiver
 * @ingroup io_interfaces
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
typedef int (*can_transceiver_enable_t)(const struct device *dev);

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
 * @retval 0 If successful.
 * @retval -EIO General input/output error, failed to enable device.
 */
static inline int can_transceiver_enable(const struct device *dev)
{
	const struct can_transceiver_driver_api *api =
		(const struct can_transceiver_driver_api *)dev->api;

	return api->enable(dev);
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
	const struct can_transceiver_driver_api *api =
		(const struct can_transceiver_driver_api *)dev->api;

	return api->disable(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAN_TRANSCEIVER_H_ */
