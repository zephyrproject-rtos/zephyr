/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
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

/* ---------- Modes ---------- */

/** @brief Operating mode of a clock monitor instance. */
enum clock_monitor_mode {
	/** Monitor disabled / idle. */
	CLOCK_MONITOR_MODE_DISABLED = 0,
	/** Continuous high/low threshold window check: the frequency of the
	 *  monitored clock is continuously compared against programmable
	 *  upper and lower bounds and events are raised when it leaves the
	 *  window.
	 */
	CLOCK_MONITOR_MODE_WINDOW,
	/** Single-shot frequency measurement reporting the measured value
	 *  in Hz. Periodic sampling is the application's responsibility:
	 *  call @ref clock_monitor_measure in a loop with whatever cadence
	 *  is needed.
	 */
	CLOCK_MONITOR_MODE_METER,
	/** Stuck/lost-only detection: the monitor only reports that the
	 *  monitored clock has stopped, with no window or frequency readout.
	 */
	CLOCK_MONITOR_MODE_LOST_ONLY,
};

/* ---------- Events (bitmask) ---------- */

/** Monitored frequency exceeded the upper threshold. */
#define CLOCK_MONITOR_EVT_FREQ_HIGH     BIT(0)
/** Monitored frequency fell below the lower threshold. */
#define CLOCK_MONITOR_EVT_FREQ_LOW      BIT(1)
/** Monitored clock stopped or stuck. */
#define CLOCK_MONITOR_EVT_CLOCK_LOST    BIT(2)
/** Reference clock stopped or stuck. Not supported by all back-ends. */
#define CLOCK_MONITOR_EVT_REF_LOST      BIT(3)
/** Single-shot frequency measurement completed. */
#define CLOCK_MONITOR_EVT_MEASURE_DONE  BIT(4)

/* ---------- Capability descriptor ---------- */

/**
 * @brief Static capability descriptor for a clock monitor instance.
 *
 * Holds the information portable code needs at runtime to decide
 * which code path to take.
 */
struct clock_monitor_capabilities {
	/** Bitmask of supported @ref clock_monitor_mode values
	 *  (use `BIT(CLOCK_MONITOR_MODE_x)` to test).
	 */
	uint32_t supported_modes;
	/** Bitmask of supported `CLOCK_MONITOR_EVT_*` flags. */
	uint32_t supported_events;
};

/**
 * @brief Common config prefix, must be the first field of a back-end's
 *        private `dev->config` struct.
 *
 * Every back-end driver places this at offset 0 of its own config, so
 * that @ref clock_monitor_caps can downcast `dev->config` and fetch the
 * capability pointer as a zero-cost inline, mirroring Zephyr's
 * `counter_config_info` convention in counter.h.
 */
struct clock_monitor_common_config {
	/** Pointer to the static capability descriptor. */
	const struct clock_monitor_capabilities *caps;
};

/**
 * @brief Return the static capabilities of a clock monitor.
 *
 * Capabilities are immutable after driver init; this is a plain pointer
 * fetch, not a syscall.
 */
static inline const struct clock_monitor_capabilities *
clock_monitor_caps(const struct device *dev)
{
	const struct clock_monitor_common_config *cfg = dev->config;

	return cfg->caps;
}

/* ---------- Per-mode configurations ---------- */

/** @brief Configuration for @ref CLOCK_MONITOR_MODE_WINDOW. */
struct clock_monitor_window_cfg {
	/** Nominal expected frequency of the monitored clock (Hz). */
	uint32_t expected_hz;
	/** Acceptable deviation in parts-per-million (± value). */
	uint32_t tolerance_ppm;
	/** Measurement window duration in ns, 0 = driver selects min. */
	uint32_t window_ns;
	/** Events to enable; must be a subset of supported_events. */
	uint32_t event_mask;
};

/** @brief Configuration for @ref CLOCK_MONITOR_MODE_METER. */
struct clock_monitor_meter_cfg {
	/** Measurement window duration in ns. */
	uint32_t window_ns;
	/** Events to enable; must be a subset of supported_events. */
	uint32_t event_mask;
};

/* ---------- Event data & callback ---------- */

/** @brief Event payload delivered to the user callback. */
struct clock_monitor_event_data {
	/** Bitmask of latched `CLOCK_MONITOR_EVT_*` flags. */
	uint32_t events;
	/** Last measured frequency (Hz); valid when
	 *  `events & CLOCK_MONITOR_EVT_MEASURE_DONE`.
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
 * of the configuration. Pass `callback = NULL` to disable async callback
 * delivery; events are still latched and observable via @ref
 * clock_monitor_get_events.
 */
struct clock_monitor_config {
	/** Operating mode to activate. */
	enum clock_monitor_mode mode;
	/** Window-mode parameters; honored when mode is WINDOW. */
	struct clock_monitor_window_cfg window;
	/** Meter-mode parameters; honored when mode is METER. */
	struct clock_monitor_meter_cfg  meter;
	/** Optional callback for asynchronous event delivery. */
	clock_monitor_callback_t callback;
	/** Opaque user pointer passed to @p callback. */
	void                    *user_data;
};

/* ---------- API function typedefs ---------- */

/** @brief Callback API for configuring a clock monitor. */
typedef int (*clock_monitor_api_configure)(const struct device *dev,
					   const struct clock_monitor_config *cfg);

/** @brief Callback API for starting continuous frequency checking. */
typedef int (*clock_monitor_api_check)(const struct device *dev);

/** @brief Callback API for stopping the clock monitor. */
typedef int (*clock_monitor_api_stop)(const struct device *dev);

/** @brief Callback API for performing a one-shot frequency measurement. */
typedef int (*clock_monitor_api_measure)(const struct device *dev,
					 uint32_t *rate_hz,
					 k_timeout_t timeout);

/** @brief Callback API for reading and clearing latched event flags. */
typedef int (*clock_monitor_api_get_events)(const struct device *dev,
					    uint32_t *events);

/** @brief Back-end driver vtable (5 entries). */
__subsystem struct clock_monitor_driver_api {
	/** @copybrief clock_monitor_configure */
	clock_monitor_api_configure      configure;
	/** @copybrief clock_monitor_check */
	clock_monitor_api_check          check;
	/** @copybrief clock_monitor_stop */
	clock_monitor_api_stop           stop;
	/** @copybrief clock_monitor_measure */
	clock_monitor_api_measure        measure;
	/** @copybrief clock_monitor_get_events */
	clock_monitor_api_get_events     get_events;
};

/* ---------- Syscalls + inline wrappers ---------- */

/**
 * @brief Apply a monitor configuration.
 *
 * Must be called when the monitor is stopped. Installs `cfg->callback`
 * (may be NULL) atomically with the rest of the configuration. Switching
 * mode implicitly stops any ongoing measurement.
 *
 * @retval 0        success
 * @retval -EINVAL  unsupported mode / event / window
 * @retval -EBUSY   monitor is running
 * @retval -ENOSYS  back-end does not implement configure()
 */
__syscall int clock_monitor_configure(const struct device *dev,
				      const struct clock_monitor_config *cfg);

static inline int z_impl_clock_monitor_configure(const struct device *dev,
						 const struct clock_monitor_config *cfg)
{
	const struct clock_monitor_driver_api *api =
		DEVICE_API_GET(clock_monitor, dev);

	if (api->configure == NULL) {
		return -ENOSYS;
	}
	return api->configure(dev, cfg);
}

/**
 * @brief Begin continuous frequency checking.
 *
 * Valid in @ref CLOCK_MONITOR_MODE_WINDOW (or, for back-ends that
 * advertise it, @ref CLOCK_MONITOR_MODE_LOST_ONLY). The device must
 * already have been configured via @ref clock_monitor_configure. For
 * METER mode use @ref clock_monitor_measure instead.
 */
__syscall int clock_monitor_check(const struct device *dev);

static inline int z_impl_clock_monitor_check(const struct device *dev)
{
	const struct clock_monitor_driver_api *api =
		DEVICE_API_GET(clock_monitor, dev);

	if (api->check == NULL) {
		return -ENOSYS;
	}
	return api->check(dev);
}

/** @brief Stop the monitor. */
__syscall int clock_monitor_stop(const struct device *dev);

static inline int z_impl_clock_monitor_stop(const struct device *dev)
{
	const struct clock_monitor_driver_api *api =
		DEVICE_API_GET(clock_monitor, dev);

	if (api->stop == NULL) {
		return -ENOSYS;
	}
	return api->stop(dev);
}

/**
 * @brief Single-shot frequency measurement; calling thread sleeps until
 *        the measurement completes.
 *
 * The device must already be configured in @ref CLOCK_MONITOR_MODE_METER
 * via @ref clock_monitor_configure. This call triggers one measurement
 * and waits for its completion event. The wait is event-driven — the
 * calling thread is suspended and resumed by the back-end's completion
 * mechanism (typically the hardware meter completion interrupt), not by
 * CPU polling.
 *
 * Periodic sampling is the application's responsibility: call this
 * function in a loop with whatever cadence (e.g. `k_sleep` between
 * iterations) the use case requires. The driver does not auto-rearm
 * the hardware between measurements.
 *
 * @param dev      Device, configured in METER mode.
 * @param rate_hz  Out: measured frequency in Hz.
 * @param timeout  Max wait; `K_FOREVER` allowed.
 *
 * @retval 0       measurement complete
 * @retval -EINVAL device not configured in METER mode
 * @retval -EAGAIN timeout elapsed before completion
 * @retval -EIO    monitored clock lost before the measurement completed
 * @retval -ENOSYS back-end does not implement measure()
 */
__syscall int clock_monitor_measure(const struct device *dev,
				    uint32_t *rate_hz,
				    k_timeout_t timeout);

static inline int z_impl_clock_monitor_measure(const struct device *dev,
					       uint32_t *rate_hz,
					       k_timeout_t timeout)
{
	const struct clock_monitor_driver_api *api =
		DEVICE_API_GET(clock_monitor, dev);

	if (api->measure == NULL) {
		return -ENOSYS;
	}
	return api->measure(dev, rate_hz, timeout);
}

/**
 * @brief Read and clear the latched event flags.
 *
 * Returns the set of events that have occurred since the previous call
 * (or since the monitor was configured) and atomically clears them, so
 * the same events will not be reported again.
 *
 * Intended for polling-mode usage and for draining events that are not
 * delivered via the async callback (e.g. events not included in
 * `cfg->window.event_mask`).
 *
 * @param dev    Device.
 * @param events Out: bitmask of `CLOCK_MONITOR_EVT_*` flags.
 *
 * @retval 0       success
 * @retval -ENOSYS unsupported
 */
__syscall int clock_monitor_get_events(const struct device *dev,
				       uint32_t *events);

static inline int z_impl_clock_monitor_get_events(const struct device *dev,
						  uint32_t *events)
{
	const struct clock_monitor_driver_api *api =
		DEVICE_API_GET(clock_monitor, dev);

	if (api->get_events == NULL) {
		return -ENOSYS;
	}
	return api->get_events(dev, events);
}

/* ---------- Devicetree helpers ---------- */

/**
 * @brief Static descriptor obtainable from devicetree.
 *
 * Holds the `struct device *` plus `tolerance_ppm`. The expected
 * monitored frequency is not carried here: back-end drivers that know
 * the monitored clock (e.g. through a `clocks`-phandle binding) derive
 * it from @ref clock_control_get_rate at configure time when the caller
 * leaves `clock_monitor_window_cfg.expected_hz` at 0.
 */
struct clock_monitor_dt_spec {
	/** Clock monitor device reference. */
	const struct device *dev;
	/** Tolerance in parts-per-million from devicetree. */
	uint32_t tolerance_ppm;
};

/**
 * @brief Build a @ref clock_monitor_dt_spec from a devicetree node.
 *
 * Uses the optional DT property `tolerance-ppm`; missing property
 * defaults to 0.
 */
#define CLOCK_MONITOR_DT_SPEC_GET(node_id)                                       \
	{                                                                        \
		.dev           = DEVICE_DT_GET(node_id),                         \
		.tolerance_ppm = DT_PROP_OR(node_id, tolerance_ppm, 0),          \
	}

/** Instance-scoped variant for `DT_DRV_COMPAT` driver code. */
#define CLOCK_MONITOR_DT_SPEC_INST_GET(inst) \
	CLOCK_MONITOR_DT_SPEC_GET(DT_DRV_INST(inst))

/** @brief `true` if the underlying device is ready. */
static inline bool clock_monitor_is_ready_dt(const struct clock_monitor_dt_spec *spec)
{
	return device_is_ready(spec->dev);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/clock_monitor.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_MONITOR_H_ */
