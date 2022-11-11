/*
 * Copyright (c) 2019-2020 Peter Bigot Consulting, LLC
 * Copyright (c) 2021 NXP
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_REGULATOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_REGULATOR_H_

/**
 * @brief Regulator Interface
 * @defgroup regulator_interface Regulator Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/onoff.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/** @brief Driver-specific API functions to support regulator control. */
__subsystem struct regulator_driver_api {
	int (*enable)(const struct device *dev, struct onoff_client *cli);
	int (*disable)(const struct device *dev);
	int (*count_voltages)(const struct device *dev);
	int (*count_modes)(const struct device *dev);
	int (*list_voltages)(const struct device *dev, unsigned int selector);
	int (*is_supported_voltage)(const struct device *dev, int min_uV,
				    int max_uV);
	int (*set_voltage)(const struct device *dev, int min_uV, int max_uV);
	int (*get_voltage)(const struct device *dev);
	int (*set_current_limit)(const struct device *dev, int min_uA,
				 int max_uA);
	int (*get_current_limit)(const struct device *dev);
	int (*set_mode)(const struct device *dev, uint32_t mode);
	int (*set_mode_voltage)(const struct device *dev, uint32_t mode,
				uint32_t min_uV, uint32_t max_uV);
	int (*get_mode_voltage)(const struct device *dev, uint32_t mode);
	int (*mode_disable)(const struct device *dev, uint32_t mode);
	int (*mode_enable)(const struct device *dev, uint32_t mode);
};

/** @endcond */

/**
 * @brief Enable a regulator.
 *
 * Reference-counted request that a regulator be turned on. This is an
 * asynchronous operation; if successfully initiated the result will be
 * communicated through the @p cli parameter.
 *
 * A regulator is considered "on" when it has reached a stable/usable state.
 *
 * @funcprops \isr_ok \pre_kernel_ok
 *
 * @param dev Regulator device instance
 * @param cli On-off client instance. This is used to notify the caller when the
 * attempt to turn on the regulator has completed (can be `NULL`).
 *
 * @retval 0 If enable request has been successfully initiated.
 * @retval -errno Negative errno in case of failure (can be from onoff_request()
 * or individual regulator drivers).
 */
static inline int regulator_enable(const struct device *dev,
				   struct onoff_client *cli)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	return api->enable(dev, cli);
}

/**
 * @brief Disable a regulator.
 *
 * Release a regulator after a previous regulator_enable() completed
 * successfully.
 *
 * If the release removes the last dependency on the regulator it will begin a
 * transition to its "off" state. There is currently no mechanism to notify when
 * the regulator has completely turned off.
 *
 * This must be invoked at most once for each successful regulator_enable().
 *
 * @funcprops \isr_ok
 *
 * @param dev Regulator device instance.
 *
 * @retval 0 If enable request has been successfully initiated.
 * @retval -errno Negative errno in case of failure (can be from onoff_release()
 * or individual regulator drivers).
 */
static inline int regulator_disable(const struct device *dev)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	return api->disable(dev);
}

/**
 * @brief Return the number of supported voltage levels
 * Returns the number of selectors, or negative errno. Selectors are
 * numbered starting at zero, and typically correspond to bitfields
 * in hardware registers.
 *
 * @param dev: Regulator device to count voltage levels for.
 * @return number of selectors, or negative errno.
 */
static inline int regulator_count_voltages(const struct device *dev)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->count_voltages == NULL) {
		return -ENOSYS;
	}

	return api->count_voltages(dev);
}

/**
 * @brief Return the number of supported regulator modes
 * Returns the number of supported regulator modes. Many regulators will only
 * support one mode. Regulator modes can be set and selected with
 * regulator_set_mode
 *
 * @param dev: Regulator device to count supported regulator modes for
 * @return number of supported modes
 */
static inline int regulator_count_modes(const struct device *dev)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->count_modes == NULL) {
		return 0;
	}

	return api->count_modes(dev);
}

/**
 * @brief Return supported voltage
 * Returns a voltage that can be passed to @ref regulator_set_voltage(), zero
 * if the selector code can't be used, or a negative errno.
 *
 * @param dev: Regulator device to get voltage for.
 * @param selector: voltage selector code.
 * @return voltage level in uV, or zero if selector code can't be used.
 */
static inline int regulator_list_voltages(const struct device *dev,
					  unsigned int selector)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->list_voltages == NULL) {
		return 0;
	}

	return api->list_voltages(dev, selector);
}

/**
 * @brief Check if a voltage range can be supported.
 *
 * @param dev: Regulator to check range against.
 * @param min_uV: Minimum voltage in microvolts
 * @param max_uV: maximum voltage in microvolts
 * @returns boolean or negative error code.
 */
static inline int regulator_is_supported_voltage(const struct device *dev,
						 int min_uV, int max_uV)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->is_supported_voltage == NULL) {
		return -ENOSYS;
	}

	return api->is_supported_voltage(dev, min_uV, max_uV);
}

/**
 * @brief Set regulator output voltage.
 * Sets a regulator to the closest supported output voltage.
 * @param dev: Regulator to set voltage
 * @param min_uV: Minimum acceptable voltage in microvolts
 * @param max_uV: Maximum acceptable voltage in microvolts
 */
static inline int regulator_set_voltage(const struct device *dev, int min_uV,
					int max_uV)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->set_voltage == NULL) {
		return -ENOSYS;
	}

	return api->set_voltage(dev, min_uV, max_uV);
}

/**
 * @brief Get regulator output voltage.
 * Returns the current regulator voltage in microvolts
 *
 * @param dev: Regulator to query
 * @return voltage level in uV
 */
static inline int regulator_get_voltage(const struct device *dev)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->get_voltage == NULL) {
		return 0;
	}

	return api->get_voltage(dev);
}

/**
 * @brief Set regulator output current limit
 * Sets current sink to desired output current.
 * @param dev: Regulator to set output current level
 * @param min_uA: minimum microamps
 * @param max_uA: maximum microamps
 * @return 0 on success, or errno on error
 */
static inline int regulator_set_current_limit(const struct device *dev,
					      int min_uA, int max_uA)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->set_current_limit == NULL) {
		return -ENOSYS;
	}

	return api->set_current_limit(dev, min_uA, max_uA);
}

/**
 * @brief Get regulator output current.
 * Note the current limit must have been set for this call to succeed.
 * @param dev: Regulator to query
 * @return current limit in uA, or errno
 */
static inline int regulator_get_current_limit(const struct device *dev)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->get_current_limit == NULL) {
		return -ENOSYS;
	}

	return api->get_current_limit(dev);
}

/**
 * @brief Select mode of regulator
 * Regulators can support multiple modes in order to permit different voltage
 * configuration or better power savings. This API will apply a mode for
 * the regulator.
 * @param dev: regulator to switch mode for
 * @param mode: Mode to select for this regulator. Only modes present
 * in the regulator-allowed-modes property are permitted.
 * @return 0 on success, or errno on error
 */
static inline int regulator_set_mode(const struct device *dev, uint32_t mode)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->set_mode == NULL) {
		return -ENOSYS;
	}

	return api->set_mode(dev, mode);
}

/**
 * @brief Set target voltage for regulator mode
 * Part of the extended regulator consumer API.
 * sets the target voltage for a given regulator mode. This mode does
 * not need to be the active mode. This API can be used to configure
 * voltages for a mode, then the regulator can be switched to that mode
 * with the regulator_set_mode api.
 * @param dev: regulator to set voltage for
 * @param mode: target mode to configure voltage for
 * @param min_uV: minimum voltage acceptable, in uV
 * @param max_uV: maximum voltage acceptable, in uV
 * @return 0 on success, or errno on error
 */
static inline int regulator_set_mode_voltage(const struct device *dev,
					     uint32_t mode, uint32_t min_uV,
					     uint32_t max_uV)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->set_mode_voltage == NULL) {
		return -ENOSYS;
	}

	return api->set_mode_voltage(dev, mode, min_uV, max_uV);
}

/**
 * @brief Get target voltage for regulator mode
 * Part of the extended regulator consumer API.
 * gets the target voltage for a given regulator mode. This mode does
 * not need to be the active mode. This API can be used to read voltages
 * from a regulator mode other than the default.
 * @param dev: regulator to query voltage from
 * @param mode: target mode to query voltage from
 * @return voltage level in uV
 */
static inline int regulator_get_mode_voltage(const struct device *dev,
					     uint32_t mode)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->get_mode_voltage == NULL) {
		return 0;
	}

	return api->get_mode_voltage(dev, mode);
}

/**
 * @brief Disable regulator for a given mode
 * Part of the extended regulator consumer API.
 * Disables the regulator in a given mode. Does not implement the
 * onoff service, as this is incompatible with multiple mode operation
 * @param dev: regulator to disable
 * @param mode: mode to change regulator state in
 */
static inline int regulator_mode_disable(const struct device *dev,
					 uint32_t mode)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->mode_disable == NULL) {
		return -ENOSYS;
	}

	return api->mode_disable(dev, mode);
}

/**
 * @brief Enable regulator for a given mode
 * Part of the extended regulator consumer API.
 * Enables the regulator in a given mode. Does not implement the
 * onoff service, as this is incompatible with multiple mode operation
 * @param dev: regulator to enable
 * @param mode: mode to change regulator state in
 */
static inline int regulator_mode_enable(const struct device *dev,
					uint32_t mode)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->mode_enable == NULL) {
		return -ENOSYS;
	}

	return api->mode_enable(dev, mode);
}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_REGULATOR_H_ */
