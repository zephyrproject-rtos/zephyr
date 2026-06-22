/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <limits.h>
#include <stddef.h>

#include <zephyr/cpu_freq/pstate.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "cpu_freq_thermal_cap.h"

LOG_MODULE_REGISTER(cpu_freq_thermal_cap, CONFIG_CPU_FREQ_LOG_LEVEL);

#if !DT_HAS_COMPAT_STATUS_OKAY(zephyr_cpu_freq_thermal_cap)
#error "CONFIG_CPU_FREQ_THERMAL_CAP requires a zephyr,cpu-freq-thermal-cap node"
#endif

#define CPU_FREQ_THERMAL_CAP_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_cpu_freq_thermal_cap)
#define CPU_FREQ_THERMAL_CAP_PSTATE_ROOT DT_PATH(performance_states)

#define CPU_FREQ_THERMAL_CAP_PSTATE_GET(_node_id) PSTATE_DT_GET(_node_id)

#define CPU_FREQ_THERMAL_CAP_TRIP_INIT(_node_id)					\
	{										\
		.temperature_mc = DT_PROP(_node_id, temperature_millicelsius),		\
		.hysteresis_mc = DT_PROP_OR(_node_id, hysteresis_millicelsius, 0),	\
		.cap_pstate = PSTATE_DT_GET(DT_PHANDLE(_node_id, cap_pstate)),		\
	}

BUILD_ASSERT(DT_NODE_EXISTS(CPU_FREQ_THERMAL_CAP_PSTATE_ROOT),
	     "cpu_freq thermal cap: performance_states node missing in devicetree");
BUILD_ASSERT(DT_CHILD_NUM_STATUS_OKAY(CPU_FREQ_THERMAL_CAP_NODE) > 0,
	     "cpu_freq thermal cap: at least one trip child is required");

struct cpu_freq_thermal_trip {
	int32_t temperature_mc;
	int32_t hysteresis_mc;
	const struct pstate *cap_pstate;
	size_t cap_index;
	bool active;
};

static const struct device *const thermal_sensor =
	DEVICE_DT_GET(DT_PHANDLE(CPU_FREQ_THERMAL_CAP_NODE, sensor));

static const enum sensor_channel thermal_sensor_channel =
	(DT_ENUM_IDX(CPU_FREQ_THERMAL_CAP_NODE, sensor_channel) == 0) ?
		SENSOR_CHAN_DIE_TEMP : SENSOR_CHAN_AMBIENT_TEMP;

static const struct pstate *const cpu_freq_pstates[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(CPU_FREQ_THERMAL_CAP_PSTATE_ROOT,
		CPU_FREQ_THERMAL_CAP_PSTATE_GET, (,))
};

static struct cpu_freq_thermal_trip thermal_trips[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(CPU_FREQ_THERMAL_CAP_NODE,
		CPU_FREQ_THERMAL_CAP_TRIP_INIT, (,))
};

static size_t fail_safe_index;
static uint8_t consecutive_failures;
static atomic_t current_cap_index;
static atomic_t trip_active;

static void thermal_cap_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(cpu_freq_thermal_cap_work, thermal_cap_work_handler);

/**
 * Apply the configured fail-safe cap after repeated sensor failures.
 *
 * The fail-safe cap keeps the CPU frequency constraint conservative until
 * a later successful sample recomputes the cap from the sensor temperature.
 */
static void thermal_cap_apply_fail_safe(void)
{
	atomic_set(&current_cap_index, (atomic_val_t)fail_safe_index);
	atomic_set(&trip_active, 1);
	LOG_WRN("Thermal sensor failure limit reached; applying fail-safe P-state index %zu",
		fail_safe_index);
}

/**
 * Handle a failed thermal sensor sample.
 */
static void thermal_cap_handle_error(int error)
{
	if (consecutive_failures < UINT8_MAX) {
		consecutive_failures++;
	}

	LOG_WRN("Thermal sensor sample failed: %d; consecutive failures: %u", error,
		(unsigned int)consecutive_failures);

#if IS_ENABLED(CONFIG_CPU_FREQ_THERMAL_CAP_FAIL_SAFE)
	if (consecutive_failures >= CONFIG_CPU_FREQ_THERMAL_CAP_SENSOR_FAILURE_LIMIT) {
		thermal_cap_apply_fail_safe();
	}
#endif
}

/**
 * Find the thermal cap table index for a P-state.
 *
 * The thermal cap uses P-state indices as an ordering key: larger indices are
 * treated as lower-performance, more restrictive P-states.
 */
static int thermal_cap_pstate_index(const struct pstate *state, size_t *index)
{
	for (size_t i = 0; i < ARRAY_SIZE(cpu_freq_pstates); i++) {
		if (cpu_freq_pstates[i] == state) {
			*index = i;
			return 0;
		}
	}

	return -ENOENT;
}

/**
 * Update trip states and the cached cap from a temperature sample.
 *
 * This applies trip hysteresis and records the most restrictive cap among all
 * active trips for the CPU frequency path to consume later.
 */
static void thermal_cap_update(int32_t temperature_mc)
{
	size_t cap_index = 0U;
	bool any_active = false;

	for (size_t i = 0; i < ARRAY_SIZE(thermal_trips); i++) {
		struct cpu_freq_thermal_trip *trip = &thermal_trips[i];
		int64_t release_mc = (int64_t)trip->temperature_mc - trip->hysteresis_mc;

		if (trip->active && temperature_mc <= release_mc) {
			trip->active = false;
		} else if (!trip->active && temperature_mc >= trip->temperature_mc) {
			trip->active = true;
		}

		/* If the trip is active, include its cap_index in the final cap calculation.
		 * Because performance decreases as the index increases, MAX() represents
		 * selecting the stronger (more restrictive) constraint.
		 */
		if (trip->active) {
			any_active = true;
			cap_index = MAX(cap_index, trip->cap_index);
		}
	}

	atomic_set(&current_cap_index, (atomic_val_t)cap_index);
	atomic_set(&trip_active, any_active ? 1 : 0);
}

/**
 * Sample the thermal sensor and refresh the cached cap.
 *
 * Sensor drivers may block, so this helper is called from thermal cap work
 * instead of the CPU frequency timer path.
 */
int cpu_freq_thermal_cap_sample(void)
{
	struct sensor_value temperature;
	int64_t temperature_mc;
	int ret;

	if (!device_is_ready(thermal_sensor)) {
		ret = -ENODEV;
		goto error;
	}

	ret = sensor_sample_fetch(thermal_sensor);
	if (ret != 0) {
		goto error;
	}

	ret = sensor_channel_get(thermal_sensor, thermal_sensor_channel, &temperature);
	if (ret != 0) {
		goto error;
	}

	temperature_mc = sensor_value_to_milli(&temperature);
	if ((temperature_mc < INT32_MIN) || (temperature_mc > INT32_MAX)) {
		ret = -ERANGE;
		goto error;
	}

	consecutive_failures = 0U;
	thermal_cap_update((int32_t)temperature_mc);

	return 0;

error:
	thermal_cap_handle_error(ret);
	return ret;
}

/**
 * Input: The requested P-state selected by the CPU frequency policy.
 * Output: The P-state after being constrained by the thermal cap.
 */
const struct pstate *cpu_freq_thermal_cap_apply(const struct pstate *state)
{
	size_t requested_index;
	atomic_val_t cap_index;

	if (state == NULL) {
		LOG_WRN("Thermal cap received a NULL requested P-state");
		return NULL;
	}

	if (thermal_cap_pstate_index(state, &requested_index) != 0) {
		LOG_ERR("Requested P-state %p is not in the CPUFreq P-state table", state);
		return state;
	}

	cap_index = atomic_get(&current_cap_index);
	if ((cap_index < 0) || ((size_t)cap_index >= ARRAY_SIZE(cpu_freq_pstates))) {
		LOG_ERR("Invalid thermal cap index %d", (int)cap_index);
		return state;
	}

	return cpu_freq_pstates[MAX(requested_index, (size_t)cap_index)];
}

/**
 * Select the next thermal sampling timeout.
 *
 * When there is no active trip, use the normal polling interval.
 * When there is an active trip, use the trip-active polling interval, which is usually
 * shorter, so that cap changes and release decisions are reflected promptly.
 */
static k_timeout_t thermal_cap_next_timeout(void)
{
	if (atomic_get(&trip_active) != 0) {
		return K_MSEC(DT_PROP(CPU_FREQ_THERMAL_CAP_NODE, trip_active_polling_delay_ms));
	}

	return K_MSEC(DT_PROP(CPU_FREQ_THERMAL_CAP_NODE, polling_delay_ms));
}

/**
 * Periodic thermal cap work handler.
 */
static void thermal_cap_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	(void)cpu_freq_thermal_cap_sample();
	k_work_schedule(&cpu_freq_thermal_cap_work, thermal_cap_next_timeout());
}

/**
 * Initialize the CPU frequency thermal cap runtime state.
 */
static int thermal_cap_init(void)
{
	int ret;

	/* Convert the cap-pstate phandle of each trip to an array index. */
	for (size_t i = 0; i < ARRAY_SIZE(thermal_trips); i++) {
		ret = thermal_cap_pstate_index(thermal_trips[i].cap_pstate,
					       &thermal_trips[i].cap_index);
		if (ret != 0) {
			LOG_ERR("Thermal trip %zu references an unknown cap P-state", i);
			return ret;
		}
	}

	/* Use the lowest-performance P-state as the sensor failure fail-safe cap. */
	fail_safe_index = ARRAY_SIZE(cpu_freq_pstates) - 1U;

	k_work_schedule(&cpu_freq_thermal_cap_work, K_NO_WAIT);

	LOG_INF("CPU frequency thermal cap initialized with %zu trip point(s)",
		ARRAY_SIZE(thermal_trips));

	return 0;
}

SYS_INIT(thermal_cap_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
