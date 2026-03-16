/*
 * Copyright (c) 2023 Victor Chavez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the Bluetooth log backend API
 * @ingroup log_backend_ble
 */

#ifndef ZEPHYR_LOG_BACKEND_BLE_H_
#define ZEPHYR_LOG_BACKEND_BLE_H_

/**
 * @brief Bluetooth log backend API
 * @defgroup log_backend_ble Bluetooth log backend API
 * @ingroup log_backend
 * @{
 */

#include <stdbool.h>
/**
 * @brief Raw adv UUID data to add the Bluetooth backend for the use with apps
 *        such as the NRF Toolbox
 *
 */

#define LOGGER_BACKEND_BLE_ADV_UUID_DATA                                                           \
	0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40,  \
		0x6E

/**
 * @brief Hook for application to know when the Bluetooth backend
 *        is enabled or disabled.
 * @param backend_status True if the backend is enabled or false if disabled
 * @param ctx User context
 *
 */
typedef void (*logger_backend_ble_hook)(bool backend_status, void *ctx);

/**
 * @brief Allows application to add a hook for the status of the Bluetooth
 *        logger backend.
 * @details The Bluetooth logger backend is enabled or disabled auomatically by
 *          the subscription of the notification characteristic of this Bluetooth
 *          Logger backend service.
 *
 * @param hook The hook that will be called when the status of the backend changes
 * @param ctx User context for whenever the hook is called
 */
void logger_backend_ble_set_hook(logger_backend_ble_hook hook, void *ctx);

/** @} */

#endif /* ZEPHYR_LOG_BACKEND_BLE_H_ */
