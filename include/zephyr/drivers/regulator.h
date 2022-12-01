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
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/** @brief Driver-specific API functions to support regulator control. */
__subsystem struct regulator_driver_api {
	int (*enable)(const struct device *dev);
	int (*disable)(const struct device *dev);
	unsigned int (*count_voltages)(const struct device *dev);
	int (*list_voltage)(const struct device *dev, unsigned int idx,
			    int32_t *volt_uv);
	int (*set_voltage)(const struct device *dev, int32_t min_uv,
			   int32_t max_uv);
	int (*get_voltage)(const struct device *dev, int32_t *volt_uv);
	int (*set_current_limit)(const struct device *dev, int32_t min_ua,
				 int32_t max_ua);
	int (*get_current_limit)(const struct device *dev, int32_t *curr_ua);
	int (*set_mode)(const struct device *dev, uint32_t mode);
};

/**
 * @brief Common regulator data.
 *
 * This structure **must** be placed first in the driver's data structure.
 */
struct regulator_common_data {
	/** Lock */
	struct k_mutex lock;
	/** Reference count */
	int refcnt;
};

/**
 * @brief Initialize common regulator data.
 *
 * This function **must** be called when driver is initialized.
 *
 * @param dev Regulator device instance.
 */
void regulator_common_data_init(const struct device *dev);

/** @endcond */

/**
 * @brief Enable a regulator.
 *
 * Reference-counted request that a regulator be turned on. A regulator is
 * considered "on" when it has reached a stable/usable state.
 *
 * @param dev Regulator device instance
 *
 * @retval 0 If regulator has been successfully enabled.
 * @retval -errno Negative errno in case of failure.
 */
int regulator_enable(const struct device *dev);

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
 * @param dev Regulator device instance.
 *
 * @retval 0 If regulator has been successfully disabled.
 * @retval -errno Negative errno in case of failure.
 */
int regulator_disable(const struct device *dev);

/**
 * @brief Obtain the number of supported voltage levels.
 *
 * Each voltage level supported by a regulator gets an index, starting from
 * zero. The total number of supported voltage levels can be used together with
 * regulator_list_voltage() to list all supported voltage levels.
 *
 * @param dev Regulator device instance.
 *
 * @return Number of supported voltages.
 */
static inline unsigned int regulator_count_voltages(const struct device *dev)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->count_voltages == NULL) {
		return 0U;
	}

	return api->count_voltages(dev);
}

/**
 * @brief Obtain the value of a voltage given an index.
 *
 * Each voltage level supported by a regulator gets an index, starting from
 * zero. Together with regulator_count_voltages(), this function can be used
 * to iterate over all supported voltages.
 *
 * @param dev Regulator device instance.
 * @param idx Voltage index.
 * @param[out] volt_uv Where voltage for the given @p index will be stored, in
 * microvolts.
 *
 * @retval 0 If @p index corresponds to a supported voltage.
 * @retval -EINVAL If @p index does not correspond to a supported voltage.
 */
static inline int regulator_list_voltage(const struct device *dev,
					 unsigned int idx, int32_t *volt_uv)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->list_voltage == NULL) {
		return -EINVAL;
	}

	return api->list_voltage(dev, idx, volt_uv);
}

/**
 * @brief Check if a voltage within a window is supported.
 *
 * @param dev Regulator device instance.
 * @param min_uv Minimum voltage in microvolts.
 * @param max_uv maximum voltage in microvolts.
 *
 * @retval true If voltage is supported.
 * @retval false If voltage is not supported.
 */
bool regulator_is_supported_voltage(const struct device *dev, int32_t min_uv,
				    int32_t max_uv);

/**
 * @brief Set the output voltage.
 *
 * The output voltage will be configured to the closest supported output
 * voltage. regulator_get_voltage() can be used to obtain the actual configured
 * voltage. The voltage will be applied to the active or selected mode.
 *
 * @param dev Regulator device instance.
 * @param min_uv Minimum acceptable voltage in microvolts.
 * @param max_uv Maximum acceptable voltage in microvolts.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If the given voltage window is not valid.
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
 * @param[out] volt_uv Where configured output voltage will be stored.
 *
 * @retval 0 If successful
 * @retval -ENOSYS If function is not implemented.
 * @retval -errno In case of any other error.
 */
static inline int regulator_get_voltage(const struct device *dev,
					int32_t *volt_uv)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->get_voltage == NULL) {
		return -ENOSYS;
	}

	return api->get_voltage(dev, volt_uv);
}

/**
 * @brief Set output current limit.
 *
 * The output current limit will be configured to the closest supported output
 * current limit. regulator_get_current_limit() can be used to obtain the actual
 * configured current limit.
 *
 * @param dev Regulator device instance.
 * @param min_ua Minimum acceptable current limit in microamps.
 * @param max_ua Maximum acceptable current limit in microamps.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If the given current limit window is not valid.
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
 * @param[out] curr_ua Where output current limit will be stored.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If function is not implemented.
 * @retval -errno In case of any other error.
 */
static inline int regulator_get_current_limit(const struct device *dev,
					      int32_t *curr_ua)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->get_current_limit == NULL) {
		return -ENOSYS;
	}

	return api->get_current_limit(dev, curr_ua);
}

/**
 * @brief Set mode.
 *
 * Regulators can support multiple modes in order to permit different voltage
 * configuration or better power savings. This API will apply a mode for
 * the regulator.
 *
 * Some regulators may only allow setting mode externally, but still allow
 * configuring the parameters such as the output voltage. For such devices, this
 * function will return -EPERM, indicating mode can't be changed. However, all
 * future calls to e.g. regulator_set_voltage() will apply to the selected mode.
 *
 * Some regulators may apply a mode to all of its regulators simultaneously.
 *
 * @param dev Regulator device instance.
 * @param mode Mode to select for this regulator. Only modes present in the
 * `regulator-allowed-modes` devicetree property are permitted.
 *
 * @retval 0 If successful.
 * @retval -EPERM If mode can not be changed.
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

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_REGULATOR_H_ */
