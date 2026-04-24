/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup clock_monitor_interface
 * @brief Public Clock Monitor driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_MONITOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_MONITOR_H_

/**
 * @brief Clock Monitor Interface
 * @defgroup clock_monitor_interface Clock Monitor
 * @since 4.5
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Operating mode of a clock monitor instance.
 *
 * Value 0 is intentionally unnamed: a zero-initialized configuration is
 * rejected by @ref clock_monitor_configure with `-ENOTSUP`.
 */
enum clock_monitor_mode {
	/** Continuous high/low threshold window check: the frequency of the
	 *  monitored clock is continuously compared against programmable
	 *  upper and lower bounds and events are raised when it leaves the
	 *  window.
	 */
	CLOCK_MONITOR_MODE_WINDOW = 1,
	/** One frequency measurement per @ref clock_monitor_start,
	 *  reported in Hz through the configure-time callback. The device
	 *  automatically returns to the configured (stopped) state before
	 *  the completion callback runs; no @ref clock_monitor_stop is
	 *  needed on the happy path. To repeat the measurement, call
	 *  @ref clock_monitor_start again from the callback.
	 */
	CLOCK_MONITOR_MODE_MEASURE,
};

/** @brief Clock monitor event flags, OR-combined into `uint32_t` masks. */
enum {
	/** Monitored frequency exceeded the upper threshold. */
	CLOCK_MONITOR_EVT_FREQ_HIGH    = BIT(0),
	/** Monitored frequency fell below the lower threshold. */
	CLOCK_MONITOR_EVT_FREQ_LOW     = BIT(1),
	/** Monitored clock stopped or stuck. */
	CLOCK_MONITOR_EVT_CLOCK_LOST   = BIT(2),
	/** Frequency measurement completed. */
	CLOCK_MONITOR_EVT_MEASURE_DONE = BIT(3),
};

/** @brief Configuration for @ref CLOCK_MONITOR_MODE_WINDOW. */
struct clock_monitor_window_cfg {
	/** Nominal expected frequency of the monitored clock (Hz). */
	uint32_t expected_hz;
	/** Acceptable deviation in parts-per-million (± value). */
	uint32_t tolerance_ppm;
	/** Measurement window duration in ns. Must be > 0. */
	uint32_t window_ns;
};

/** @brief Configuration for @ref CLOCK_MONITOR_MODE_MEASURE. */
struct clock_monitor_measure_cfg {
	/** Measurement window duration in ns. Must be > 0. */
	uint32_t window_ns;
};

/** @brief Event payload delivered to the user callback. */
struct clock_monitor_event_data {
	/** Bitmask of latched `CLOCK_MONITOR_EVT_*` flags. */
	uint32_t events;
	/** Result of the measurement that completed with this event (Hz);
	 *  valid when `events & CLOCK_MONITOR_EVT_MEASURE_DONE`.
	 */
	uint32_t measured_hz;
};

/**
 * @brief Callback invoked on clock monitor events.
 *
 * May be called in ISR context. Keep the callback short; defer heavy work
 * to a thread / work queue.
 *
 * @param dev       Clock monitor device.
 * @param evt       Event payload. Lifetime ends when the callback returns.
 * @param user_data Opaque pointer copied from `clock_monitor_config`.
 */
typedef void (*clock_monitor_callback_t)(const struct device *dev,
					 const struct clock_monitor_event_data *evt,
					 void *user_data);

/**
 * @brief Top-level configuration handed to @ref clock_monitor_configure.
 *
 * The callback (plus its user_data) is installed atomically with the rest
 * of the configuration and is the only event delivery path. Passing
 * `callback = NULL` disables event delivery; in MEASURE mode the most
 * recent result remains retrievable by polling @ref
 * clock_monitor_get_rate.
 */
struct clock_monitor_config {
	/** Operating mode to activate. */
	enum clock_monitor_mode mode;
	/** Per-mode parameters; selected by @p mode. */
	union {
		/** Honored when mode is WINDOW. */
		struct clock_monitor_window_cfg window;
		/** Honored when mode is MEASURE. */
		struct clock_monitor_measure_cfg measure;
	};
	/** Optional callback for asynchronous event delivery. */
	clock_monitor_callback_t callback;
	/** Opaque user pointer passed to @p callback. */
	void *user_data;
};

/**
 * @def_driverbackendgroup{CLOCK_MONITOR,clock_monitor_interface}
 * @{
 */

/** @brief Callback API for configuring a clock monitor. */
typedef int (*clock_monitor_api_configure)(const struct device *dev,
					   const struct clock_monitor_config *cfg);

/** @brief Callback API for starting the configured mode. */
typedef int (*clock_monitor_api_start)(const struct device *dev);

/** @brief Callback API for stopping the clock monitor (ISR-safe). */
typedef int (*clock_monitor_api_stop)(const struct device *dev);

/** @brief Callback API for polling the most recent measured rate. */
typedef int (*clock_monitor_api_get_rate)(const struct device *dev,
					  uint32_t *rate_hz);

/** @brief Callback API for switching the reference / target clock inputs. */
typedef int (*clock_monitor_api_set_source)(const struct device *dev,
					    uint32_t reference,
					    uint32_t target);

/**
 * @driver_ops{CLOCK_MONITOR}
 */
__subsystem struct clock_monitor_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief clock_monitor_configure
	 */
	clock_monitor_api_configure      configure;
	/**
	 * @driver_ops_mandatory @copybrief clock_monitor_start
	 */
	clock_monitor_api_start          start;
	/**
	 * @driver_ops_mandatory @copybrief clock_monitor_stop
	 */
	clock_monitor_api_stop           stop;
	/**
	 * @driver_ops_optional @copybrief clock_monitor_get_rate
	 */
	clock_monitor_api_get_rate       get_rate;
	/**
	 * @driver_ops_optional @copybrief clock_monitor_set_source
	 */
	clock_monitor_api_set_source     set_source;
};

/** @} */

/**
 * @brief Apply a monitor configuration.
 *
 * Must be called when the monitor is stopped. Installs `cfg->callback`
 * (may be NULL) atomically with the rest of the configuration.
 * Reconfiguration is done by calling this function again with a new
 * mode/parameter set; there is no separate teardown operation.
 *
 * @param dev Clock monitor device.
 * @param cfg Configuration to apply.
 *
 * @retval 0        success
 * @retval -EINVAL  malformed configuration
 * @retval -ENOTSUP back-end does not support the requested mode
 * @retval -EBUSY   monitor is running, or another configure() is in
 *                  progress
 * @retval -EIO     reference clock query failed or hardware init failed
 */
__syscall int clock_monitor_configure(const struct device *dev,
				      const struct clock_monitor_config *cfg);

static inline int z_impl_clock_monitor_configure(const struct device *dev,
						 const struct clock_monitor_config *cfg)
{
	return DEVICE_API_GET(clock_monitor, dev)->configure(dev, cfg);
}

/**
 * @brief Start operation of the configured mode.
 *
 * The device must already have been configured via @ref
 * clock_monitor_configure. The behavior follows the installed mode:
 *
 * - @ref CLOCK_MONITOR_MODE_WINDOW begins continuous threshold
 *   checking. Threshold crossings are delivered through the
 *   configure-time callback until @ref clock_monitor_stop.
 * - @ref CLOCK_MONITOR_MODE_MEASURE begins one measurement. The
 *   completion invokes the configure-time callback with
 *   @ref CLOCK_MONITOR_EVT_MEASURE_DONE and the measured value in
 *   `evt->measured_hz` (or @ref CLOCK_MONITOR_EVT_CLOCK_LOST with 0 if
 *   the monitored clock is stuck). The device automatically returns to
 *   the configured (stopped) state before the callback runs.
 *
 * This function is ISR-safe and may be called from the event callback:
 * repeating sampling is achieved by calling it again from the callback
 * (the same idiom as re-arming a counter alarm from its callback).
 *
 * The API provides no blocking wait or timeout for measurements: wait
 * for the callback (e.g. on a semaphore it gives) with an
 * application-chosen timeout, and call @ref clock_monitor_stop to abort
 * an in-flight measurement on the timeout path. Alternatively poll
 * @ref clock_monitor_get_rate.
 *
 * @param dev Clock monitor device.
 *
 * @retval 0       success
 * @retval -EINVAL device not configured
 * @retval -EBUSY  already running
 */
__syscall int clock_monitor_start(const struct device *dev);

static inline int z_impl_clock_monitor_start(const struct device *dev)
{
	return DEVICE_API_GET(clock_monitor, dev)->start(dev);
}

/**
 * @brief Stop the monitor.
 *
 * Returns the device to the configured (stopped) state and aborts any
 * in-flight measurement. Idempotent: stopping an already-stopped device
 * returns 0.
 *
 * This function is ISR-safe and may be called from the event callback
 * (a benign no-op there in MEASURE mode, where the auto-disarm has
 * already stopped the device).
 *
 * @param dev Clock monitor device.
 *
 * @retval 0       success
 */
__syscall int clock_monitor_stop(const struct device *dev);

static inline int z_impl_clock_monitor_stop(const struct device *dev)
{
	return DEVICE_API_GET(clock_monitor, dev)->stop(dev);
}

/**
 * @brief Poll the most recent completed measurement.
 *
 * Returns the result of the most recent measurement completed since the
 * last @ref clock_monitor_configure. The value is retained: it is NOT
 * cleared by reads, and reading has no hardware side effects. This is
 * the polling alternative to the configure-time callback and works from
 * user mode, where callbacks may not be installed.
 *
 * @param dev           Device, configured in MEASURE mode.
 * @param[out] rate_hz  Measured frequency in Hz. Written only on
 *                      success.
 *
 * @retval 0       `*rate_hz` holds the most recent measured value
 * @retval -EAGAIN measurement in flight, or none completed yet
 * @retval -EIO    monitored clock lost (stuck) since configure()
 * @retval -ENOSYS back-end has no measurement hardware
 */
__syscall int clock_monitor_get_rate(const struct device *dev,
				     uint32_t *rate_hz);

static inline int z_impl_clock_monitor_get_rate(const struct device *dev,
						uint32_t *rate_hz)
{
	const struct clock_monitor_driver_api *api =
		DEVICE_API_GET(clock_monitor, dev);

	if (api->get_rate == NULL) {
		return -ENOSYS;
	}
	return api->get_rate(dev, rate_hz);
}

/**
 * @brief Switch the reference / target clock inputs at runtime.
 *
 * The @p reference and @p target arguments are opaque back-end cookies.
 * Each back-end exposes its accepted encodings through a dt-bindings
 * header (e.g. `<zephyr/dt-bindings/clock_monitor/<vendor>-<ip>.h>`)
 * so that devicetree properties and runtime calls share a single set
 * of constants.
 *
 * @param dev        Clock monitor device.
 * @param reference  Reference clock cookie, or sentinel.
 * @param target     Target clock cookie, or sentinel.
 *
 * @retval 0        success
 * @retval -ENOTSUP back-end's hardware cannot switch sources at runtime
 * @retval -EINVAL  one or both cookies are unknown to this back-end
 * @retval -EBUSY   monitor running or measurement in progress
 * @retval -ENOSYS  back-end does not implement set_source()
 */
__syscall int clock_monitor_set_source(const struct device *dev,
				       uint32_t reference, uint32_t target);

static inline int z_impl_clock_monitor_set_source(const struct device *dev,
						  uint32_t reference,
						  uint32_t target)
{
	const struct clock_monitor_driver_api *api =
		DEVICE_API_GET(clock_monitor, dev);

	if (api->set_source == NULL) {
		return -ENOSYS;
	}
	return api->set_source(dev, reference, target);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/clock_monitor.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_MONITOR_H_ */
