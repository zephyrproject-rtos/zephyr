/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_os.h
 *
 * @brief MQTT Client depends on certain OS specific functionality. The needed
 *        methods are mapped here and should be implemented based on OS in use.
 *
 * @details Memory management, mutex, logging and wall clock are the needed
 *          functionality for MQTT module. The needed interfaces are defined
 *          in the OS. OS specific port of the interface shall be provided.
 *
 */

#ifndef MQTT_OS_H_
#define MQTT_OS_H_

#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/mutex.h>

#include <zephyr/net/net_core.h>

#include "mqtt_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Initialize the mutex for the module, if any.
 *
 * @details This method is called during module initialization @ref mqtt_init.
 */
static inline void mqtt_mutex_init(struct mqtt_client *client)
{
	sys_mutex_init(&client->internal.mutex);
}

/**@brief Acquire lock on the module specific mutex, if any.
 *
 * @details This is assumed to be a blocking method until the acquisition
 *          of the mutex succeeds.
 */
static inline void mqtt_mutex_lock(struct mqtt_client *client)
{
	int ret = sys_mutex_lock(&client->internal.mutex, K_FOREVER);

	__ASSERT(ret == 0, "sys_mutex_lock failed with %d", ret);
	(void)ret;
}

/**@brief Release the lock on the module specific mutex, if any.
 */
static inline void mqtt_mutex_unlock(struct mqtt_client *client)
{
	int ret = sys_mutex_unlock(&client->internal.mutex);

	__ASSERT(ret == 0, "sys_mutex_unlock failed with %d", ret);
	(void)ret;
}

/**@brief Method to get the sys tick or a wall clock in millisecond resolution.
 *
 * @retval Current wall clock or sys tick value in milliseconds.
 */
static inline uint32_t mqtt_sys_tick_in_ms_get(void)
{
	return k_uptime_get_32();
}

/**@brief Method to get elapsed time in milliseconds since the last activity.
 *
 * @param[in] last_activity The value since elapsed time is requested.
 *
 * @retval Time elapsed since last_activity time.
 */
static inline uint32_t mqtt_elapsed_time_in_ms_get(uint32_t last_activity)
{
	int32_t diff = k_uptime_get_32() - last_activity;

	if (diff < 0) {
		return 0;
	}

	return diff;
}

#ifdef __cplusplus
}
#endif

#endif /* MQTT_OS_H_ */
