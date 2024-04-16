/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>

#include <stdint.h>

/**
 * @brief Sensor emulator backend API
 * @defgroup sensor_emulator_backend Sensor emulator backend API
 * @ingroup io_interfaces
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in public documentation.
 */

/**
 * @brief Collection of function pointers implementing a common backend API for sensor emulators
 */
__subsystem struct emul_sensor_backend_api {
	/** Sets a given fractional value for a given sensor channel. */
	int (*set_channel)(const struct emul *target, enum sensor_channel ch, q31_t value,
			   int8_t shift);
	/** Retrieve a range of sensor values to use with test. */
	int (*get_sample_range)(const struct emul *target, enum sensor_channel ch, q31_t *lower,
				q31_t *upper, q31_t *epsilon, int8_t *shift);
};
/**
 * @endcond
 */

/**
 * @brief Check if a given sensor emulator supports the backend API
 *
 * @param target Pointer to emulator instance to query
 *
 * @return True if supported, false if unsupported or if \p target is NULL.
 */
static inline bool emul_sensor_backend_is_supported(const struct emul *target)
{
	return target && target->backend_api;
}

/**
 * @brief Set an expected value for a given channel on a given sensor emulator
 *
 * @param target Pointer to emulator instance to operate on
 * @param ch Sensor channel to set expected value for
 * @param value Expected value in fixed-point format using standard SI unit for sensor type
 * @param shift Shift value (scaling factor) applied to \p value
 *
 * @return 0 if successful
 * @return -ENOTSUP if no backend API or if channel not supported by emul
 * @return -ERANGE if provided value is not in the sensor's supported range
 */
static inline int emul_sensor_backend_set_channel(const struct emul *target, enum sensor_channel ch,
						  q31_t value, int8_t shift)
{
	if (!target || !target->backend_api) {
		return -ENOTSUP;
	}

	struct emul_sensor_backend_api *api = (struct emul_sensor_backend_api *)target->backend_api;

	if (api->set_channel) {
		return api->set_channel(target, ch, value, shift);
	}
	return -ENOTSUP;
}

/**
 * @brief Query an emulator for a channel's supported sample value range and tolerance
 *
 * @param target Pointer to emulator instance to operate on
 * @param ch The channel to request info for. If \p ch is unsupported, return `-ENOTSUP`
 * @param[out] lower Minimum supported sample value in SI units, fixed-point format
 * @param[out] upper Maximum supported sample value in SI units, fixed-point format
 * @param[out] epsilon Tolerance to use comparing expected and actual values to account for rounding
 *             and sensor precision issues. This can usually be set to the minimum sample value step
 *             size. Uses SI units and fixed-point format.
 * @param[out] shift The shift value (scaling factor) associated with \p lower, \p upper, and
 *             \p epsilon.
 *
 * @return 0 if successful
 * @return -ENOTSUP if no backend API or if channel not supported by emul
 *
 */
static inline int emul_sensor_backend_get_sample_range(const struct emul *target,
						       enum sensor_channel ch, q31_t *lower,
						       q31_t *upper, q31_t *epsilon, int8_t *shift)
{
	if (!target || !target->backend_api) {
		return -ENOTSUP;
	}

	struct emul_sensor_backend_api *api = (struct emul_sensor_backend_api *)target->backend_api;

	if (api->get_sample_range) {
		return api->get_sample_range(target, ch, lower, upper, epsilon, shift);
	}
	return -ENOTSUP;
}

/**
 * @}
 */
