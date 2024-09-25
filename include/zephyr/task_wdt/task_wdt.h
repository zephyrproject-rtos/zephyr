/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Task watchdog header file
 *
 * This header file declares prototypes for the task watchdog APIs.
 *
 * The task watchdog can be used to monitor correct operation of individual
 * threads. It can be used together with a hardware watchdog as a fallback.
 */

#ifndef TASK_WDT_H_
#define TASK_WDT_H_

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

/**
 * @brief Task Watchdog APIs
 * @defgroup task_wdt_api Task Watchdog APIs
 * @since 2.5
 * @version 0.8.0
 * @ingroup os_services
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Task watchdog callback. */
typedef void (*task_wdt_callback_t)(int channel_id, void *user_data);

/**
 * @brief Initialize task watchdog.
 *
 * This function sets up necessary kernel timers and the hardware watchdog (if
 * desired as fallback). It has to be called before task_wdt_add() and
 * task_wdt_feed().
 *
 * @param hw_wdt Pointer to the hardware watchdog device used as fallback.
 *               Pass NULL if no hardware watchdog fallback is desired.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If assigning a hardware watchdog is not supported.
 * @retval -Errno Negative errno if the fallback hw_wdt is used and the
 *                install timeout API fails.  See wdt_install_timeout()
 *                API for possible return values.
 */
int task_wdt_init(const struct device *hw_wdt);

/**
 * @brief Install new timeout.
 *
 * Adds a new timeout to the list of task watchdog channels.
 *
 * @param reload_period Period in milliseconds used to reset the timeout
 * @param callback Function to be called when watchdog timer expired. Pass
 *                 NULL to use system reset handler.
 * @param user_data User data to associate with the watchdog channel.
 *
 * @retval channel_id If successful, a non-negative value indicating the index
 *                    of the channel to which the timeout was assigned. This
 *                    ID is supposed to be used as the parameter in calls to
 *                    task_wdt_feed().
 * @retval -EINVAL If the reload_period is invalid.
 * @retval -ENOMEM If no more timeouts can be installed.
 */
int task_wdt_add(uint32_t reload_period, task_wdt_callback_t callback,
		 void *user_data);

/**
 * @brief Delete task watchdog channel.
 *
 * Deletes the specified channel from the list of task watchdog channels. The
 * channel is now available again for other tasks via task_wdt_add() function.
 *
 * @param channel_id Index of the channel as returned by task_wdt_add().
 *
 * @retval 0 If successful.
 * @retval -EINVAL If there is no installed timeout for supplied channel.
 */
int task_wdt_delete(int channel_id);

/**
 * @brief Feed specified watchdog channel.
 *
 * This function loops through all installed task watchdogs and updates the
 * internal kernel timer used as for the software watchdog with the next due
 * timeout.
 *
 * @param channel_id Index of the fed channel as returned by task_wdt_add().
 *
 * @retval 0 If successful.
 * @retval -EINVAL If there is no installed timeout for supplied channel.
 */
int task_wdt_feed(int channel_id);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* TASK_WDT_H_ */
