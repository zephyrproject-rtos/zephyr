/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for watchdog drivers
 */

#ifndef _WDT_H_
#define _WDT_H_

/**
 * @brief Watchdog Interface
 *
 * @details A watchdog is a device that resets the system unless it is
 * periodically reloaded.  Usage:
 *
 * @code
 * struct device *wdt = device_get_binding("WATCHDOG_0");
 * struct wdt_config config = {
 *	   .mode = WDT_MODE_RESET,
 *	   .timeout = WDT_2_26_CYCLES,
 * };
 *
 * wdt_enable(wdt);
 * wdt_set_config(wdt, &config);
 * ...
 * wdt_reload(dev)
 *
 * @endcode
 *
 * @defgroup wdt_interface Watchdog Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <device.h>
#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WDT_MODE		(BIT(1))
#define WDT_MODE_OFFSET		(1)
#define WDT_TIMEOUT_MASK	(0xF)

/** @brief Watchdog operating mode */
enum wdt_mode {
	/** Reset when the watchdog times out */
	WDT_MODE_RESET = 0,
	/**
	 * Interrupt some time before the watchdog times out and, if
	 * not reloaded, reset when the watchdog times out
	 */
	WDT_MODE_INTERRUPT_RESET
};

/**
 * @brief Clock cycles before timing out.
 *
 * The timeout is based on an abstract watchdog with a 32 MHz clock
 * and a power-of-two prescaler.  This enum selects the prescaler,
 * where a name like @p WDT_2_23_CYCLES means divide the 32 MHz clock by
 * two to the power of 23 giving a timeout of 2**23 / 32e6 = 0.26 s.
 *
 */
enum wdt_clock_timeout_cycles {
	/** Approximately 2 ms */
	WDT_2_16_CYCLES,
	WDT_2_17_CYCLES,
	WDT_2_18_CYCLES,
	WDT_2_19_CYCLES,
	WDT_2_20_CYCLES,
	WDT_2_21_CYCLES,
	WDT_2_22_CYCLES,
	/** Approximately 260 ms */
	WDT_2_23_CYCLES,
	/** Approximately 520 ms */
	WDT_2_24_CYCLES,
	/** Approximately 1.0 s */
	WDT_2_25_CYCLES,
	/** Approximately 2.1 s */
	WDT_2_26_CYCLES,
	WDT_2_27_CYCLES,
	WDT_2_28_CYCLES,
	WDT_2_29_CYCLES,
	WDT_2_30_CYCLES,
	/** Approximately 67 s */
	WDT_2_31_CYCLES
};


/**
 * @brief Watchdog configuration struct.
 */
struct wdt_config {
	/** One of wdt_clock_timeout_cycles */
	u32_t timeout;
	/** Watchdog operating mode */
	enum wdt_mode mode;
	/**
	 * Optional function to call before the watchdog times
	 * out
	 */
	void (*interrupt_fn)(struct device *dev);
};

typedef void (*wdt_api_enable)(struct device *dev);
typedef void (*wdt_api_disable)(struct device *dev);
typedef int (*wdt_api_set_config)(struct device *dev,
				  struct wdt_config *config);
typedef void (*wdt_api_get_config)(struct device *dev,
				   struct wdt_config *config);
typedef void (*wdt_api_reload)(struct device *dev);

/** @brief Driver API structure. */
struct wdt_driver_api {
	wdt_api_enable enable;
	wdt_api_disable disable;
	wdt_api_get_config get_config;
	wdt_api_set_config set_config;
	wdt_api_reload reload;
};

/**
 * @brief Enable the watchdog.
 *
 * @details Enable the watchdog and start the timeout.  Reload the
 * watchdog by calling wdt_reload().
 *
 * @param dev watchdog device structure.
 *
 * @return N/A
 */
static inline void wdt_enable(struct device *dev)
{
	const struct wdt_driver_api *api = dev->driver_api;

	api->enable(dev);
}

/**
 * @brief Disable the watchdog.
 *
 * @param dev watchdog device structure.
 *
 * @return N/A
 */
static inline void wdt_disable(struct device *dev)
{
	const struct wdt_driver_api *api = dev->driver_api;

	api->disable(dev);
}

/**
 * @brief Get the current watchdog configuration.
 *
 * @param dev watchdog device structure.
 * @param[out] on return contains the current config.
 * @return N/A
 */
static inline void wdt_get_config(struct device *dev,
				  struct wdt_config *config)
{
	const struct wdt_driver_api *api = dev->driver_api;

	api->get_config(dev, config);
}

/**
 * @brief Configure the watchdog.
 *
 * @details Configure the watchdog including setting the timeout,
 * mode, and callback.	Call wdt_enable() before calling this
 * function.
 *
 * @param dev watchdog device structure.
 * @param[in] config configuration to apply.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @a config is invalid.
 */
static inline int wdt_set_config(struct device *dev,
				 struct wdt_config *config)
{
	const struct wdt_driver_api *api = dev->driver_api;

	return api->set_config(dev, config);
}

/**
 * @brief Reload the watchdog.
 *
 * @param dev watchdog device structure.
 *
 * @return N/A
 */
static inline void wdt_reload(struct device *dev)
{
	const struct wdt_driver_api *api = dev->driver_api;

	api->reload(dev);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
