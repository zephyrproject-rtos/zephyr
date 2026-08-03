/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the Omron D7S seismic sensor
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_D7S_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_D7S_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup d7s_interface Omron D7S
 * @ingroup sensor_interface_ext
 * @{
 */

/** Sensor operating mode, reported by @ref SENSOR_ATTR_D7S_STATE. */
enum d7s_state {
	/** Normal Mode, idle. */
	D7S_STATE_NORMAL_STANDBY = 0,
	/** Normal Mode, an earthquake is being processed. */
	D7S_STATE_NORMAL_BUSY = 1,
	/** Initial Installation Mode. */
	D7S_STATE_INITIAL_INSTALL = 2,
	/** Offset Acquisition Mode. */
	D7S_STATE_OFFSET_ACQUISITION = 3,
	/** Self-Diagnostic Mode. */
	D7S_STATE_SELF_DIAGNOSTIC = 4,
};

/** Axis pair used for the SI calculation. */
enum d7s_axis_mode {
	/** Fixed to the Y and Z axes. */
	D7S_AXIS_MODE_YZ = 0,
	/** Fixed to the X and Z axes. */
	D7S_AXIS_MODE_XZ = 1,
	/** Fixed to the X and Y axes. */
	D7S_AXIS_MODE_XY = 2,
	/** Recalculated whenever Normal Mode starts. */
	D7S_AXIS_MODE_AUTO = 3,
	/** Recalculated on entry to Initial Installation Mode. */
	D7S_AXIS_MODE_SWITCH_AT_INSTALL = 4,
};

/** Earthquake shutoff judgement threshold. */
enum d7s_shutoff_threshold {
	/** Assert at a seismic intensity equivalent to 5 Upper or higher. */
	D7S_SHUTOFF_THRESHOLD_HIGH = 0,
	/** Assert at a lower seismic intensity. */
	D7S_SHUTOFF_THRESHOLD_LOW = 1,
};

/** Latched event flags, reported by @ref SENSOR_ATTR_D7S_EVENT. */
enum d7s_event {
	/** Shutoff output was asserted by the earthquake judgement. */
	D7S_EVENT_SHUTOFF = BIT(0),
	/** Shutoff output was asserted by collapse detection. */
	D7S_EVENT_COLLAPSE = BIT(1),
	/** The last self-diagnostic failed. */
	D7S_EVENT_SELFTEST_ERROR = BIT(2),
	/** The last offset acquisition failed. */
	D7S_EVENT_OFFSET_ERROR = BIT(3),
};

/** Memory area selector for @ref SENSOR_ATTR_D7S_CLEAR. */
enum d7s_clear {
	/** Earthquake records. */
	D7S_CLEAR_EARTHQUAKE = BIT(0),
	/** Self-diagnostic results. */
	D7S_CLEAR_SELFTEST = BIT(1),
	/** Most recent offset data. */
	D7S_CLEAR_RECENT_OFFSET = BIT(2),
	/** Initial installation data. */
	D7S_CLEAR_INSTALL_OFFSET = BIT(3),
};

/**
 * @brief D7S specific sensor channels.
 */
enum sensor_channel_d7s {
	/**
	 * Spectral intensity of the most recent earthquake, in kine (cm/s).
	 *
	 * Resolution is 0.1 kine.
	 */
	SENSOR_CHAN_D7S_SI = SENSOR_CHAN_PRIV_START,
	/**
	 * Peak ground acceleration of the most recent earthquake, in gal
	 * (cm/s²).
	 *
	 * This is the peak of the two-axis synthesised acceleration over the
	 * whole event, not an instantaneous per-axis reading, which is why it
	 * is not reported through @ref SENSOR_CHAN_ACCEL_X and friends.
	 * Resolution is 0.1 gal.
	 */
	SENSOR_CHAN_D7S_PGA,
};

/**
 * @brief D7S specific sensor attributes.
 *
 * All attributes must be set on the <tt>SENSOR_CHAN_ALL</tt> channel.
 */
enum sensor_attribute_d7s {
	/**
	 * Operating mode. Read-only, one of @ref d7s_state.
	 */
	SENSOR_ATTR_D7S_STATE = SENSOR_ATTR_PRIV_START,
	/**
	 * Axis pair used for the SI calculation, one of @ref d7s_axis_mode.
	 */
	SENSOR_ATTR_D7S_AXIS_MODE,
	/**
	 * Earthquake shutoff judgement threshold, one of
	 * @ref d7s_shutoff_threshold.
	 */
	SENSOR_ATTR_D7S_SHUTOFF_THRESHOLD,
	/**
	 * Event flags latched since the last read, a mask of @ref d7s_event.
	 *
	 * Reading this attribute consumes the flags and releases the shutoff
	 * output. Read-only.
	 */
	SENSOR_ATTR_D7S_EVENT,
	/**
	 * Enter Self-Diagnostic Mode. Write-only, the value is ignored.
	 *
	 * The sensor returns to Normal Mode on its own once the diagnostic
	 * finishes; the result is reported through
	 * @ref D7S_EVENT_SELFTEST_ERROR.
	 */
	SENSOR_ATTR_D7S_SELFTEST,
	/**
	 * Enter Initial Installation Mode. Write-only, the value is ignored.
	 *
	 * The sensor must be stationary until it returns to Normal Mode.
	 */
	SENSOR_ATTR_D7S_INSTALL,
	/**
	 * Clear stored data. Write-only, a mask of @ref d7s_clear.
	 */
	SENSOR_ATTR_D7S_CLEAR,
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_D7S_H_ */
