/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for watchdog drivers.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_WATCHDOG_H_
#define ZEPHYR_INCLUDE_DRIVERS_WATCHDOG_H_

/**
 * @brief Watchdog Interface
 * @defgroup watchdog_interface Watchdog Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <sys/util.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pause watchdog timer when CPU is in sleep state.
 */
#define WDT_OPT_PAUSE_IN_SLEEP		BIT(0)

/**
 * @brief Pause watchdog timer when CPU is halted by the debugger.
 */
#define WDT_OPT_PAUSE_HALTED_BY_DBG	BIT(1)

/**
 * @brief Watchdog reset flag bit field mask shift.
 */
#define WDT_FLAG_RESET_SHIFT		(0)
/**
 * @brief Watchdog reset flag bit field mask.
 */
#define WDT_FLAG_RESET_MASK		(0x3 << WDT_FLAG_RESET_SHIFT)

/**
 * @name Watchdog Reset Behavior.
 * Reset behavior after timeout.
 * @{
 */
/** No reset */
#define WDT_FLAG_RESET_NONE		(0 << WDT_FLAG_RESET_SHIFT)
/** CPU core reset */
#define WDT_FLAG_RESET_CPU_CORE		(1 << WDT_FLAG_RESET_SHIFT)
/** Global SoC reset */
#define WDT_FLAG_RESET_SOC		(2 << WDT_FLAG_RESET_SHIFT)
/**
 * @}
 */

/**
 * @brief Watchdog timeout window.
 *
 * Each installed timeout needs feeding within the specified time window,
 * otherwise the watchdog will trigger. If the watchdog instance does not
 * support window timeouts then min value must be equal to 0.
 *
 * @param min Lower limit of watchdog feed timeout in milliseconds.
 * @param max Upper limit of watchdog feed timeout in milliseconds.
 *
 * @note If specified values can not be precisely set they are always
 *	 rounded up.
 */
struct wdt_window {
	uint32_t min;
	uint32_t max;
};

/** Watchdog callback. */
typedef void (*wdt_callback_t)(const struct device *dev, int channel_id);

/**
 * @brief Watchdog timeout configuration struct.
 *
 * @param window Timing parameters of watchdog timeout.
 * @param callback Timeout callback. Passing NULL means that no callback
 *		   will be run.
 * @param next Pointer to the next timeout configuration. This pointer is used
 *	       for watchdogs with staged timeouts functionality. Value must be
 *	       NULL for single stage timeout.
 * @param flags Bit field with following parts:
 *
 *     reset        [ 0 : 1 ]   - perform specified reset after timeout/callback
 */
struct wdt_timeout_cfg {
	struct wdt_window window;
	wdt_callback_t callback;
#ifdef CONFIG_WDT_MULTISTAGE
	struct wdt_timeout_cfg *next;
#endif
	uint8_t flags;
};

/**
 * @typedef wdt_api_setup
 * @brief Callback API for setting up watchdog instance.
 * See wdt_setup() for argument descriptions
 */
typedef int (*wdt_api_setup)(const struct device *dev, uint8_t options);

/**
 * @typedef wdt_api_disable
 * @brief Callback API for disabling watchdog instance.
 * See wdt_disable() for argument descriptions
 */
typedef int (*wdt_api_disable)(const struct device *dev);

/**
 * @typedef wdt_api_install_timeout
 * @brief Callback API for installing new timeout.
 * See wdt_install_timeout() for argument descriptions
 */
typedef int (*wdt_api_install_timeout)(const struct device *dev,
				       const struct wdt_timeout_cfg *cfg);

/**
 * @typedef wdt_api_feed
 * @brief Callback API for feeding specified watchdog timeout.
 * See (wdt_feed) for argument descriptions
 */
typedef int (*wdt_api_feed)(const struct device *dev, int channel_id);

/** @cond INTERNAL_HIDDEN */
__subsystem struct wdt_driver_api {
	wdt_api_setup setup;
	wdt_api_disable disable;
	wdt_api_install_timeout install_timeout;
	wdt_api_feed feed;
};
/**
 * @endcond
 */

/**
 * @brief Set up watchdog instance.
 *
 * This function is used for configuring global watchdog settings that
 * affect all timeouts. It should be called after installing timeouts.
 * After successful return, all installed timeouts are valid and must be
 * serviced periodically by calling wdt_feed().
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param options Configuration options as defined by the WDT_OPT_* constants
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If any of the set options is not supported.
 * @retval -EBUSY If watchdog instance has been already setup.
 */
__syscall int wdt_setup(const struct device *dev, uint8_t options);

static inline int z_impl_wdt_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_driver_api *api =
		(const struct wdt_driver_api *)dev->api;

	return api->setup(dev, options);
}

/**
 * @brief Disable watchdog instance.
 *
 * This function disables the watchdog instance and automatically uninstalls all
 * timeouts. To set up a new watchdog, install timeouts and call wdt_setup()
 * again. Not all watchdogs can be restarted after they are disabled.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval -EFAULT If watchdog instance is not enabled.
 * @retval -EPERM If watchdog can not be disabled directly by application code.
 */
__syscall int wdt_disable(const struct device *dev);

static inline int z_impl_wdt_disable(const struct device *dev)
{
	const struct wdt_driver_api *api =
		(const struct wdt_driver_api *)dev->api;

	return api->disable(dev);
}

/**
 * @brief Install new timeout.
 *
 * This function must be used before wdt_setup(). Changes applied here
 * have no effects until wdt_setup() is called.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cfg Pointer to timeout configuration structure.
 *
 * @retval channel_id If successful, a non-negative value indicating the index
 *                    of the channel to which the timeout was assigned. This
 *                    value is supposed to be used as the parameter in calls to
 *                    wdt_feed().
 * @retval -EBUSY If timeout can not be installed while watchdog has already
 *		  been setup.
 * @retval -ENOMEM If no more timeouts can be installed.
 * @retval -ENOTSUP If any of the set flags is not supported.
 * @retval -EINVAL If any of the window timeout value is out of possible range.
 *		   This value is also returned if watchdog supports only one
 *		   timeout value for all timeouts and the supplied timeout
 *		   window differs from windows for alarms installed so far.
 */
static inline int wdt_install_timeout(const struct device *dev,
				      const struct wdt_timeout_cfg *cfg)
{
	const struct wdt_driver_api *api =
		(const struct wdt_driver_api *) dev->api;

	return api->install_timeout(dev, cfg);
}

/**
 * @brief Feed specified watchdog timeout.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel_id Index of the fed channel.
 *
 * @retval 0 If successful.
 * @retval -EAGAIN If completing the feed operation would stall the
 *                 caller, for example due to an in-progress watchdog
 *                 operation such as a previous @c wdt_feed().
 * @retval -EINVAL If there is no installed timeout for supplied channel.
 */
__syscall int wdt_feed(const struct device *dev, int channel_id);

static inline int z_impl_wdt_feed(const struct device *dev, int channel_id)
{
	const struct wdt_driver_api *api =
		(const struct wdt_driver_api *)dev->api;

	return api->feed(dev, channel_id);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/watchdog.h>

#endif /* _ZEPHYR_WATCHDOG_H__ */
