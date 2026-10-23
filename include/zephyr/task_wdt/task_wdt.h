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

#ifndef ZEPHYR_INCLUDE_TASK_WDT_TASK_WDT_H_
#define ZEPHYR_INCLUDE_TASK_WDT_TASK_WDT_H_

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

/** Task watchdog channel information. */
struct task_wdt_channel_info {
	/** Period in milliseconds used to reset the timeout */
	uint32_t reload_period;
	/** Time in milliseconds left until the channel times out */
	uint32_t time_left;
	/**
	 * Thread owning the channel, or NULL if not known. Always NULL unless
	 * @kconfig{CONFIG_TASK_WDT_THREAD_INFO} is enabled.
	 */
	k_tid_t thread;
};

/** Reason of a task watchdog channel fault. */
enum task_wdt_fault_reason {
	/** The channel was not fed within its reload period */
	TASK_WDT_FAULT_TIMEOUT,
	/** The channel was fed before its minimum feed interval elapsed */
	TASK_WDT_FAULT_TOO_EARLY,
};

/** Task watchdog channel fault. */
struct task_wdt_fault {
	/** Channel which reported the fault */
	int channel_id;
	/** Reason of the fault */
	enum task_wdt_fault_reason reason;
	/**
	 * Thread owning the channel, or NULL if not known. Always NULL unless
	 * @kconfig{CONFIG_TASK_WDT_THREAD_INFO} is enabled.
	 */
	k_tid_t thread;
};

/** Task watchdog fault callback, which is told the reason of the fault. */
typedef void (*task_wdt_fault_callback_t)(const struct task_wdt_fault *fault, void *user_data);

/** Cause of a reset triggered by the task watchdog. */
enum task_wdt_reset_reason {
	/** A task watchdog channel timed out */
	TASK_WDT_RESET_TIMEOUT,
	/** The hardware watchdog fallback reset the system */
	TASK_WDT_RESET_HW_FALLBACK,
	/** A task watchdog channel was fed before its minimum feed interval elapsed */
	TASK_WDT_RESET_TOO_EARLY,
};

/** Size of the thread name kept in @ref task_wdt_reset_info, including the terminator */
#define TASK_WDT_RESET_INFO_THREAD_NAME_SIZE 16

/** Information about the last reset triggered by the task watchdog. */
struct task_wdt_reset_info {
	/** Cause of the reset */
	enum task_wdt_reset_reason reason;
	/** Channel which timed out, or -1 if not known */
	int channel_id;
	/** Number of consecutive resets triggered by the task watchdog */
	uint32_t fault_count;
	/** System uptime in milliseconds when the reset was triggered, 0 if not known */
	uint64_t uptime_ms;
	/** Name of the thread owning the channel (truncated), empty if not known */
	char thread_name[TASK_WDT_RESET_INFO_THREAD_NAME_SIZE];
};

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
 * @brief Install new timeout with a fault callback.
 *
 * Same as task_wdt_add(), but the callback is told the reason of the fault.
 * Timeouts are reported from the kernel timer interrupt, faults detected by
 * task_wdt_feed() are reported from the thread feeding the channel.
 *
 * @param reload_period Period in milliseconds used to reset the timeout
 * @param callback Function to be called when the channel reports a fault.
 *                 Pass NULL to use system reset handler.
 * @param user_data User data to associate with the watchdog channel.
 *
 * @retval channel_id If successful, a non-negative value indicating the index
 *                    of the channel to which the timeout was assigned.
 * @retval -EINVAL If the reload_period is invalid.
 * @retval -ENOMEM If no more timeouts can be installed.
 * @retval -ENOTSUP If @kconfig{CONFIG_TASK_WDT_FAULT_REASONS} is disabled.
 */
int task_wdt_add_ext(uint32_t reload_period, task_wdt_fault_callback_t callback, void *user_data);

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

/**
 * @brief Set the minimum interval between two feeds of a channel.
 *
 * Feeding the channel again sooner than @p min_period milliseconds after the
 * previous feed reports a @ref TASK_WDT_FAULT_TOO_EARLY fault, which detects a
 * thread stuck in a loop feeding its channel. The fault is reported by
 * task_wdt_feed() before it returns, and the channel is fed anyway. Channels
 * added with task_wdt_add() call their callback, or reset the system, as for a
 * timeout.
 *
 * @param channel_id Index of the channel as returned by task_wdt_add().
 * @param min_period Minimum interval in milliseconds, 0 to disable the check.
 *                   Must be lower than the reload period of the channel.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If there is no installed timeout for supplied channel or
 *                 @p min_period is not lower than its reload period.
 * @retval -ENOTSUP If @kconfig{CONFIG_TASK_WDT_FAULT_REASONS} is disabled.
 */
int task_wdt_window_set(int channel_id, uint32_t min_period);

/**
 * @brief Set the thread owning a task watchdog channel.
 *
 * By default, the owner of a channel is the thread which fed it last. Use this
 * function if the channel is fed by a different thread than the supervised
 * one. The owner is reported when the channel times out.
 *
 * @param channel_id Index of the channel as returned by task_wdt_add().
 * @param thread Thread owning the channel. Pass NULL to go back to using the
 *               thread which fed the channel last.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If there is no installed timeout for supplied channel.
 * @retval -ENOTSUP If @kconfig{CONFIG_TASK_WDT_THREAD_INFO} is disabled.
 */
int task_wdt_channel_thread_set(int channel_id, k_tid_t thread);

/**
 * @brief Get information about a task watchdog channel.
 *
 * @param channel_id Index of the channel as returned by task_wdt_add().
 * @param info Filled in with the channel information.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If there is no installed timeout for supplied channel.
 */
int task_wdt_channel_info_get(int channel_id, struct task_wdt_channel_info *info);

/**
 * @brief Get the cause of the last reset if it was triggered by the task watchdog.
 *
 * A record is kept if a channel without a callback times out, or if the
 * channel callback does not return (e.g. because it resets the system). If the
 * hardware watchdog fallback reset the system before the task watchdog could
 * write a record, this is reported as @ref TASK_WDT_RESET_HW_FALLBACK (requires
 * @kconfig{CONFIG_TASK_WDT_RESET_INFO_HW_FALLBACK}).
 *
 * @param info Filled in with the reset information.
 *
 * @retval 0 If the last reset was triggered by the task watchdog.
 * @retval -ENOENT If the last reset was not triggered by the task watchdog.
 * @retval -EINVAL If @p info is NULL.
 * @retval -ENOTSUP If @kconfig{CONFIG_TASK_WDT_RESET_INFO} is disabled.
 */
int task_wdt_reset_info_get(struct task_wdt_reset_info *info);

/**
 * @brief Clear the task watchdog reset information.
 *
 * Forget the cause of the last reset and restart counting consecutive task
 * watchdog resets from zero. The counter is also restarted after any reset
 * which was not triggered by the task watchdog.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If @kconfig{CONFIG_TASK_WDT_RESET_INFO} is disabled.
 * @retval -Errno Negative errno if the retention backend failed.
 */
int task_wdt_reset_info_clear(void);

/**
 * @brief Pause all channels before changing system power mode.
 *
 * Stop the internal timer and feed the hardware watchdog a last time
 * (if enabled). It is expected that the system enters a low-power
 * mode quite fast after this call.
 *
 * @note To pause the hardware watchdog (if this is supported),
 * enable @kconfig{CONFIG_TASK_WDT_HW_FALLBACK_PAUSE_IN_SLEEP} in your
 * configuration.
 */
void task_wdt_suspend(void);

/**
 * @brief Resume all channels execution.
 *
 * Resume the internal timer and feed all channels. Also feed the hardware
 * watchdog (if enabled) to let enough time to the application to resume
 * feeding the channels by itself.
 */
void task_wdt_resume(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_TASK_WDT_TASK_WDT_H_ */
