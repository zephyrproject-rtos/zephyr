/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for device state retention
 */

#ifndef ZEPHYR_INCLUDE_RETENTION_DEVICE_STATE_
#define ZEPHYR_INCLUDE_RETENTION_DEVICE_STATE_

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Device State Retention API
 * @defgroup device_state_retention_api Device State Retention API
 * @since 3.7
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

typedef bool (*device_state_retention_check_reinit_api)(const struct device *dev, size_t index);
typedef void (*device_state_retention_set_init_done_api)(const struct device *dev, size_t index,
							 bool value);

struct device_state_retention_api {
	device_state_retention_check_reinit_api check_reinit;
	device_state_retention_set_init_done_api set_init_done;
};

/**
 * @brief		Checks if the init or reinit function should be called for a device.
 *
 * @param dev		Device state retention device to use.
 * @param index         Index of device for which the check should be executed.
 *
 * @retval true		The reinit function should be called instead of the init.
 */
bool device_state_retention_check_reinit(const struct device *dev, size_t index);

/**
 * @brief		Store that the init for a device was successfully done.
 *
 * @param dev		Device state retention device to use.
 * @param index         Index of device for which the information should be stored.
 * @param value         True if the init was successful.
 */
void device_state_retention_set_init_done(const struct device *dev, size_t index, bool value);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RETENTION_DEVICE_STATE_ */
