/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup watchdog_interface
 * @brief Main header file for watchdog driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_WATCHDOG_H_
#define ZEPHYR_INCLUDE_DRIVERS_WATCHDOG_H_

/**
 * @brief Interfaces for watchdog devices.
 * @defgroup watchdog_interface Watchdog
 * @since 1.0
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Watchdog options
 * @anchor WDT_OPT
 * @{
 */

/** @brief Pause watchdog timer when CPU is in sleep state. */
#define WDT_OPT_PAUSE_IN_SLEEP		BIT(0)

/** @brief Pause watchdog timer when CPU is halted by the debugger. */
#define WDT_OPT_PAUSE_HALTED_BY_DBG	BIT(1)

/** @} */

/**
 * @name Watchdog behavior flags
 * @anchor WDT_FLAGS
 * @{
 */

/** @cond INTERNAL_HIDDEN */
/** @brief Watchdog reset flag bit field mask shift. */
#define WDT_FLAG_RESET_SHIFT		(0)
/** @brief Watchdog reset flag bit field mask. */
#define WDT_FLAG_RESET_MASK		(0x3 << WDT_FLAG_RESET_SHIFT)
/** @endcond */

/** Reset: none */
#define WDT_FLAG_RESET_NONE		(0 << WDT_FLAG_RESET_SHIFT)
/** Reset: CPU core */
#define WDT_FLAG_RESET_CPU_CORE		(1 << WDT_FLAG_RESET_SHIFT)
/** Reset: SoC */
#define WDT_FLAG_RESET_SOC		(2 << WDT_FLAG_RESET_SHIFT)

/** @} */

/**
 * @brief Watchdog timeout window.
 *
 * Each installed timeout needs feeding within the specified time window,
 * otherwise the watchdog will trigger. If the watchdog instance does not
 * support window timeouts then min value must be equal to 0.
 *
 * @note If specified values can not be precisely set they are always rounded
 * up.
 */
struct wdt_window {
	/** Lower limit of watchdog feed timeout in milliseconds. */
	uint32_t min;
	/** Upper limit of watchdog feed timeout in milliseconds. */
	uint32_t max;
};

/**
 * @brief Watchdog callback.
 *
 * @param dev Watchdog device instance.
 * @param channel_id Channel identifier.
 */
typedef void (*wdt_callback_t)(const struct device *dev, int channel_id);

/** @brief Watchdog timeout configuration. */
struct wdt_timeout_cfg {
	/** Timing parameters of watchdog timeout. */
	struct wdt_window window;
	/** Timeout callback (can be `NULL`). */
	wdt_callback_t callback;
#if defined(CONFIG_WDT_MULTISTAGE) || defined(__DOXYGEN__)
	/**
	 * Pointer to the next timeout configuration.
	 *
	 * This field is only available if @kconfig{CONFIG_WDT_MULTISTAGE} is
	 * enabled (watchdogs with staged timeouts functionality). Value must be
	 * `NULL` for single stage timeout.
	 */
	struct wdt_timeout_cfg *next;
#endif
	/** Flags (see @ref WDT_FLAGS). */
	uint8_t flags;
};

/**
 * @def_driverbackendgroup{Watchdog,watchdog_interface}
 * @ingroup watchdog_interface
 * @{
 */

/**
 * @brief Callback API to set up a watchdog instance.
 *
 * See wdt_setup() for argument description.
 */
typedef int (*wdt_api_setup)(const struct device *dev, uint8_t options);

/**
 * @brief Callback API to disable a watchdog instance.
 *
 * See wdt_disable() for argument description.
 */
typedef int (*wdt_api_disable)(const struct device *dev);

/**
 * @brief Callback API to install a new timeout.
 *
 * See wdt_install_timeout() for argument description.
 */
typedef int (*wdt_api_install_timeout)(const struct device *dev,
				       const struct wdt_timeout_cfg *cfg);

/**
 * @brief Callback API to feed a specified watchdog timeout.
 *
 * See wdt_feed() for argument description.
 */
typedef int (*wdt_api_feed)(const struct device *dev, int channel_id);

/**
 * @driver_ops{Watchdog}
 */
__subsystem struct wdt_driver_api {
	/** @driver_ops_mandatory @copybrief wdt_setup */
	wdt_api_setup setup;
	/** @driver_ops_mandatory @copybrief wdt_disable */
	wdt_api_disable disable;
	/** @driver_ops_mandatory @copybrief wdt_install_timeout */
	wdt_api_install_timeout install_timeout;
	/** @driver_ops_mandatory @copybrief wdt_feed */
	wdt_api_feed feed;
};

/** @} */

/**
 * @brief Set up watchdog instance.
 *
 * This function is used for configuring global watchdog settings that
 * affect all timeouts. It should be called after installing timeouts.
 * After successful return, all installed timeouts are valid and must be
 * serviced periodically by calling wdt_feed().
 *
 * @param dev Watchdog device instance.
 * @param options Configuration options (see @ref WDT_OPT).
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If any of the set options is not supported.
 * @retval -EBUSY If watchdog instance has been already setup.
 * @retval -errno In case of any other failure.
 */
__syscall int wdt_setup(const struct device *dev, uint8_t options);

static inline int z_impl_wdt_setup(const struct device *dev, uint8_t options)
{
	return DEVICE_API_GET(wdt, dev)->setup(dev, options);
}

/**
 * @brief Disable watchdog instance.
 *
 * This function disables the watchdog instance and automatically uninstalls all
 * timeouts. To set up a new watchdog, install timeouts and call wdt_setup()
 * again. Not all watchdogs can be restarted after they are disabled.
 *
 * @param dev Watchdog device instance.
 *
 * @retval 0 If successful.
 * @retval -EFAULT If watchdog instance is not enabled.
 * @retval -EPERM If watchdog can not be disabled directly by application code.
 * @retval -errno In case of any other failure.
 */
__syscall int wdt_disable(const struct device *dev);

static inline int z_impl_wdt_disable(const struct device *dev)
{
	return DEVICE_API_GET(wdt, dev)->disable(dev);
}

/**
 * @brief Install a new timeout.
 *
 * @note This function must be used before wdt_setup(). Changes applied here
 * have no effects until wdt_setup() is called.
 *
 * @param dev Watchdog device instance.
 * @param[in] cfg Timeout configuration.
 *
 * @retval channel_id If successful, a non-negative value indicating the index
 * of the channel to which the timeout was assigned. This value is supposed to
 * be used as the parameter in calls to wdt_feed().
 * @retval -EBUSY If timeout can not be installed while watchdog has already
 * been setup.
 * @retval -ENOMEM If no more timeouts can be installed.
 * @retval -ENOTSUP If any of the set flags is not supported.
 * @retval -EINVAL If any of the window timeout value is out of possible range.
 * This value is also returned if watchdog supports only one timeout value for
 * all timeouts and the supplied timeout window differs from windows for alarms
 * installed so far.
 * @retval -errno In case of any other failure.
 */
static inline int wdt_install_timeout(const struct device *dev,
				      const struct wdt_timeout_cfg *cfg)
{
	return DEVICE_API_GET(wdt, dev)->install_timeout(dev, cfg);
}

/**
 * @brief Feed specified watchdog timeout.
 *
 * @param dev Watchdog device instance.
 * @param channel_id Channel index.
 *
 * @retval 0 If successful.
 * @retval -EAGAIN If completing the feed operation would stall the caller, for
 * example due to an in-progress watchdog operation such as a previous
 * wdt_feed() call.
 * @retval -EINVAL If there is no installed timeout for supplied channel.
 * @retval -errno In case of any other failure.
 */
__syscall int wdt_feed(const struct device *dev, int channel_id);

static inline int z_impl_wdt_feed(const struct device *dev, int channel_id)
{
	return DEVICE_API_GET(wdt, dev)->feed(dev, channel_id);
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/syscalls/watchdog.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_WATCHDOG_H_ */
