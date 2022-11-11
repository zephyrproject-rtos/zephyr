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
int regulator_count_voltages(const struct device *dev);

/**
 * @brief Return the number of supported regulator modes
 * Returns the number of supported regulator modes. Many regulators will only
 * support one mode. Regulator modes can be set and selected with
 * regulator_set_mode
 *
 * @param dev: Regulator device to count supported regulator modes for
 * @return number of supported modes
 */
int regulator_count_modes(const struct device *dev);

/**
 * @brief Return supported voltage
 * Returns a voltage that can be passed to @ref regulator_set_voltage(), zero
 * if the selector code can't be used, or a negative errno.
 *
 * @param dev: Regulator device to get voltage for.
 * @param selector: voltage selector code.
 * @return voltage level in uV, or zero if selector code can't be used.
 */
int regulator_list_voltages(const struct device *dev, unsigned int selector);

/**
 * @brief Check if a voltage range can be supported.
 *
 * @param dev: Regulator to check range against.
 * @param min_uV: Minimum voltage in microvolts
 * @param max_uV: maximum voltage in microvolts
 * @returns boolean or negative error code.
 */
int regulator_is_supported_voltage(const struct device *dev, int min_uV, int max_uV);

/**
 * @brief Set regulator output voltage.
 * Sets a regulator to the closest supported output voltage.
 * @param dev: Regulator to set voltage
 * @param min_uV: Minimum acceptable voltage in microvolts
 * @param max_uV: Maximum acceptable voltage in microvolts
 */
int regulator_set_voltage(const struct device *dev, int min_uV, int max_uV);

/**
 * @brief Get regulator output voltage.
 * Returns the current regulator voltage in microvolts
 *
 * @param dev: Regulator to query
 * @return voltage level in uV
 */
int regulator_get_voltage(const struct device *dev);

/**
 * @brief Set regulator output current limit
 * Sets current sink to desired output current.
 * @param dev: Regulator to set output current level
 * @param min_uA: minimum microamps
 * @param max_uA: maximum microamps
 * @return 0 on success, or errno on error
 */
int regulator_set_current_limit(const struct device *dev, int min_uA, int max_uA);

/**
 * @brief Get regulator output current.
 * Note the current limit must have been set for this call to succeed.
 * @param dev: Regulator to query
 * @return current limit in uA, or errno
 */
int regulator_get_current_limit(const struct device *dev);

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
int regulator_set_mode(const struct device *dev, uint32_t mode);

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
int regulator_set_mode_voltage(const struct device *dev, uint32_t mode,
	uint32_t min_uV, uint32_t max_uV);

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
int regulator_get_mode_voltage(const struct device *dev, uint32_t mode);

/**
 * @brief Disable regulator for a given mode
 * Part of the extended regulator consumer API.
 * Disables the regulator in a given mode. Does not implement the
 * onoff service, as this is incompatible with multiple mode operation
 * @param dev: regulator to disable
 * @param mode: mode to change regulator state in
 */
int regulator_mode_disable(const struct device *dev, uint32_t mode);

/**
 * @brief Enable regulator for a given mode
 * Part of the extended regulator consumer API.
 * Enables the regulator in a given mode. Does not implement the
 * onoff service, as this is incompatible with multiple mode operation
 * @param dev: regulator to enable
 * @param mode: mode to change regulator state in
 */
int regulator_mode_enable(const struct device *dev, uint32_t mode);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_REGULATOR_H_ */
