/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_LIN_TRANSCEIVER_H_P
#define ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_LIN_TRANSCEIVER_H_P

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief LIN transceiver driver API
 *
 * This file contains definitions and API for LIN transceiver communication.
 */

#include <zephyr/drivers/lin.h>
#include <zephyr/device.h>

typedef int (*lin_transceiver_enable_t)(const struct device *dev, uint8_t flags);

typedef int (*lin_transceiver_disable_t)(const struct device *dev);

__subsystem struct lin_transceiver_driver_api {
	lin_transceiver_enable_t enable;
	lin_transceiver_disable_t disable;
};

/**
 * @brief Enable LIN transceiver
 *
 * Enable the LIN transceiver.
 *
 * @note The LIN transceiver is controlled by the LIN controller driver and
 *       should not normally be controlled by the application.
 *
 * @see lin_start()
 *
 * @param dev Pointer to the device structure for the LIN transceiver.
 * @param flags Flags
 * @retval 0 if successful.
 * @retval -EALREADY if the transceiver is already enabled.
 * @retval -EIO General input/output error, failed to enable transceiver.
 */
static inline int lin_transceiver_enable(const struct device *dev, uint8_t flags)
{
	return DEVICE_API_GET(lin_transceiver, dev)->enable(dev, flags);
}

/**
 * @brief Disable LIN transceiver
 *
 * Disable the LIN transceiver.
 *
 * @note The LIN transceiver is controlled by the LIN controller driver and
 *       should not normally be controlled by the application.
 *
 * @see lin_stop()
 *
 * @param dev Pointer to the device structure for the LIN transceiver.
 * @retval 0 if successful.
 * @retval -EALREADY if the transceiver is already disabled.
 * @retval -EIO General input/output error, failed to disable transceiver.
 */
static inline int lin_transceiver_disable(const struct device *dev)
{
	return DEVICE_API_GET(lin_transceiver, dev)->disable(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_LIN_TRANSCEIVER_H_P */
