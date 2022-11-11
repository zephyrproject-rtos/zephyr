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
	int32_t (*list_voltages)(const struct device *dev,
				 unsigned int selector);
	int (*is_supported_voltage)(const struct device *dev, int32_t min_uv,
				    int32_t max_uv);
	int (*set_voltage)(const struct device *dev, int32_t min_uv,
			   int32_t max_uv);
	int32_t (*get_voltage)(const struct device *dev);
	int (*set_current_limit)(const struct device *dev, int32_t min_ua,
				 int32_t max_ua);
	int (*get_current_limit)(const struct device *dev);
	int (*set_mode)(const struct device *dev, uint32_t mode);
	int (*set_mode_voltage)(const struct device *dev, uint32_t mode,
				int32_t min_uv, int32_t max_uv);
	int32_t (*get_mode_voltage)(const struct device *dev, uint32_t mode);
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
 * @brief Obtain the number of supported voltage levels.
 *
 * Selectors are numbered starting at zero, and typically correspond to
 * bitfields in hardware registers.
 *
 * @param dev Regulator device instance.
 *
 * @retval selectors Number of selectors, if successful.
 * @retval -ENOSYS If function is not implemented.
 * @retval -errno In case of any other error.
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
 * @brief Obtain the number of supported modes.
 *
 * @param dev Regulator device instance.
 *
 * @return Number of supported modes.
 *
 * @see regulator_set_mode()
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
 * @brief Obtain supported voltages.
 *
 * @param dev Regulator device instance.
 * @param selector Voltage selector.
 *
 * @return voltage Voltage level in microvolts.
 * @retval 0 If selector code can't be used.
 */
static inline int32_t regulator_list_voltages(const struct device *dev,
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
 * @brief Check if a voltage range is supported.
 *
 * @param dev Regulator device instance.
 * @param min_uv Minimum voltage in microvolts.
 * @param max_uv maximum voltage in microvolts.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If function is not implemented.
 * @retval -errno In case of any other error.
 */
static inline int regulator_is_supported_voltage(const struct device *dev,
						 int32_t min_uv, int32_t max_uv)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->is_supported_voltage == NULL) {
		return -ENOSYS;
	}

	return api->is_supported_voltage(dev, min_uv, max_uv);
}

/**
 * @brief Set output voltage.
 *
 * @note The output voltage will be configured to the closest supported output
 * voltage. regulator_get_voltage() can be used to obtain the actual configured
 * voltage.
 *
 * @param dev Regulator device instance.
 * @param min_uv Minimum acceptable voltage in microvolts.
 * @param max_uv Maximum acceptable voltage in microvolts.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If function is not implemented.
 * @retval -errno In case of any other error.
 */
static inline int regulator_set_voltage(const struct device *dev,
					int32_t min_uv, int32_t max_uv)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->set_voltage == NULL) {
		return -ENOSYS;
	}

	return api->set_voltage(dev, min_uv, max_uv);
}

/**
 * @brief Obtain output voltage.
 *
 * @param dev Regulator device instance.
 *
 * @return Voltage level in microvolts.
 */
static inline int32_t regulator_get_voltage(const struct device *dev)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->get_voltage == NULL) {
		return 0;
	}

	return api->get_voltage(dev);
}

/**
 * @brief Set output current limit.
 *
 * @param dev Regulator device instance.
 * @param min_ua Minimum acceptable current limit in microamps.
 * @param max_ua Maximum acceptable current limit in microamps.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If function is not implemented.
 * @retval -errno In case of any other error.
 */
static inline int regulator_set_current_limit(const struct device *dev,
					      int32_t min_ua, int32_t max_ua)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->set_current_limit == NULL) {
		return -ENOSYS;
	}

	return api->set_current_limit(dev, min_ua, max_ua);
}

/**
 * @brief Get output current limit.
 *
 * @param dev Regulator device instance.
 *
 * @retval current Current limit in microamperes
 * @retval -ENOSYS If function is not implemented.
 * @retval -errno In case of any other error.
 */
static inline int32_t regulator_get_current_limit(const struct device *dev)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->get_current_limit == NULL) {
		return -ENOSYS;
	}

	return api->get_current_limit(dev);
}

/**
 * @brief Set mode.
 *
 * Regulators can support multiple modes in order to permit different voltage
 * configuration or better power savings. This API will apply a mode for
 * the regulator.
 *
 * @param dev Regulator device instance.
 * @param mode Mode to select for this regulator. Only modes present in the
 * `regulator-allowed-modes` devicetree property are permitted.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If function is not implemented.
 * @retval -errno In case of any other error.
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
 * @brief Set target voltage for a regulator mode.
 *
 * @note Mode does not need to be the active mode. The regulator can be switched
 * to that mode with regulator_set_mode().
 *
 * @param dev Regulator device instance.
 * @param mode Regulator mode.
 * @param min_uv Minimum acceptable voltage in microvolts.
 * @param max_uv Maximum acceptable voltage in microvolts.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If function is not implemented.
 * @retval -errno In case of any other error.
 */
static inline int regulator_set_mode_voltage(const struct device *dev,
					     uint32_t mode, int32_t min_uv,
					     int32_t max_uv)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->set_mode_voltage == NULL) {
		return -ENOSYS;
	}

	return api->set_mode_voltage(dev, mode, min_uv, max_uv);
}

/**
 * @brief Get target voltage for a regulator mode
 *
 * This API can be used to read voltages from a regulator mode other than the
 * default. The given mode does not need to be active.
 *
 * @param dev Regulator device instance.
 * @param mode Regulator mode.
 *
 * @return Voltage level in microvolts.
 */
static inline int32_t regulator_get_mode_voltage(const struct device *dev,
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
 * @brief Disable regulator for a given mode.
 *
 * @param dev Regulator device instance.
 * @param mode Regulator mode.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If function is not implemented.
 * @retval -errno In case of any other error.
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
 * @brief Enable regulator for a given mode.
 *
 * @param dev Regulator device instance.
 * @param mode Regulator mode.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If function is not implemented.
 * @retval -errno In case of any other error.
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
