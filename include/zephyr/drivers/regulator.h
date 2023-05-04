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
#include <zephyr/devicetree.h>
#ifdef CONFIG_REGULATOR_THREAD_SAFE_REFCNT
#include <zephyr/kernel.h>
#endif
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque type to store regulator DVS states */
typedef uint8_t regulator_dvs_state_t;

/** Opaque type to store regulator modes */
typedef uint8_t regulator_mode_t;

/** Opaque bit map for regulator error flags (see @ref REGULATOR_ERRORS) */
typedef uint8_t regulator_error_flags_t;

/**
 * @name Regulator error flags.
 * @anchor REGULATOR_ERRORS
 * @{
 */

/** Voltage is too high. */
#define REGULATOR_ERROR_OVER_VOLTAGE BIT(0)
/** Current is too high. */
#define REGULATOR_ERROR_OVER_CURRENT BIT(1)
/** Temperature is too high. */
#define REGULATOR_ERROR_OVER_TEMP    BIT(2)

/** @} */

/** @cond INTERNAL_HIDDEN */

typedef int (*regulator_dvs_state_set_t)(const struct device *dev,
					 regulator_dvs_state_t state);

/** @brief Driver-specific API functions to support parent regulator control. */
__subsystem struct regulator_parent_driver_api {
	regulator_dvs_state_set_t dvs_state_set;
};

typedef int (*regulator_enable_t)(const struct device *dev);
typedef int (*regulator_disable_t)(const struct device *dev);
typedef unsigned int (*regulator_count_voltages_t)(const struct device *dev);
typedef int (*regulator_list_voltage_t)(const struct device *dev,
					unsigned int idx, int32_t *volt_uv);
typedef int (*regulator_set_voltage_t)(const struct device *dev, int32_t min_uv,
				       int32_t max_uv);
typedef int (*regulator_get_voltage_t)(const struct device *dev,
				       int32_t *volt_uv);
typedef int (*regulator_set_current_limit_t)(const struct device *dev,
					     int32_t min_ua, int32_t max_ua);
typedef int (*regulator_get_current_limit_t)(const struct device *dev,
					     int32_t *curr_ua);
typedef int (*regulator_set_mode_t)(const struct device *dev,
				    regulator_mode_t mode);
typedef int (*regulator_get_mode_t)(const struct device *dev,
				    regulator_mode_t *mode);
typedef int (*regulator_get_error_flags_t)(
	const struct device *dev, regulator_error_flags_t *flags);

/** @brief Driver-specific API functions to support regulator control. */
__subsystem struct regulator_driver_api {
	regulator_enable_t enable;
	regulator_disable_t disable;
	regulator_count_voltages_t count_voltages;
	regulator_list_voltage_t list_voltage;
	regulator_set_voltage_t set_voltage;
	regulator_get_voltage_t get_voltage;
	regulator_set_current_limit_t set_current_limit;
	regulator_get_current_limit_t get_current_limit;
	regulator_set_mode_t set_mode;
	regulator_get_mode_t get_mode;
	regulator_get_error_flags_t get_error_flags;
};

/**
 * @name Regulator flags
 * @anchor REGULATOR_FLAGS
 * @{
 */
/** Indicates regulator must stay always ON */
#define REGULATOR_ALWAYS_ON	BIT(0)
/** Indicates regulator must be initialized ON */
#define REGULATOR_BOOT_ON	BIT(1)
/** Indicates if regulator must be enabled when initialized */
#define REGULATOR_INIT_ENABLED  (REGULATOR_ALWAYS_ON | REGULATOR_BOOT_ON)

/** @} */

/** Indicates initial mode is unknown/not specified */
#define REGULATOR_INITIAL_MODE_UNKNOWN UINT8_MAX

/**
 * @brief Common regulator config.
 *
 * This structure **must** be placed first in the driver's config structure.
 */
struct regulator_common_config {
	/** Minimum allowed voltage, in microvolts. */
	int32_t min_uv;
	/** Maximum allowed voltage, in microvolts. */
	int32_t max_uv;
	/** Initial voltage, in microvolts. */
	int32_t init_uv;
	/** Minimum allowed current, in microamps. */
	int32_t min_ua;
	/** Maximum allowed current, in microamps. */
	int32_t max_ua;
	/** Allowed modes */
	const regulator_mode_t *allowed_modes;
	/** Number of allowed modes */
	uint8_t allowed_modes_cnt;
	/** Regulator initial mode */
	regulator_mode_t initial_mode;
	/** Flags (@reg REGULATOR_FLAGS). */
	uint8_t flags;
};

/**
 * @brief Initialize common driver config from devicetree.
 *
 * @param node_id Node identifier.
 */
#define REGULATOR_DT_COMMON_CONFIG_INIT(node_id)                               \
	{                                                                      \
		.min_uv = DT_PROP_OR(node_id, regulator_min_microvolt,         \
				     INT32_MIN),                               \
		.max_uv = DT_PROP_OR(node_id, regulator_max_microvolt,         \
				     INT32_MAX),                               \
		.init_uv = DT_PROP_OR(node_id, regulator_init_microvolt,       \
				      INT32_MIN),			       \
		.min_ua = DT_PROP_OR(node_id, regulator_min_microamp,          \
				     INT32_MIN),                               \
		.max_ua = DT_PROP_OR(node_id, regulator_max_microamp,          \
				     INT32_MAX),                               \
		.allowed_modes = (const regulator_mode_t [])                   \
			DT_PROP_OR(node_id, regulator_allowed_modes, {}),      \
		.allowed_modes_cnt =                                           \
			DT_PROP_LEN_OR(node_id, regulator_allowed_modes, 0),   \
		.initial_mode = DT_PROP_OR(node_id, regulator_initial_mode,    \
					   REGULATOR_INITIAL_MODE_UNKNOWN),    \
		.flags = ((DT_PROP_OR(node_id, regulator_always_on, 0U) *      \
			   REGULATOR_ALWAYS_ON) |                              \
			  (DT_PROP_OR(node_id, regulator_boot_on, 0U) *        \
			   REGULATOR_BOOT_ON)),                                \
	}

/**
 * @brief Initialize common driver config from devicetree instance.
 *
 * @param inst Instance.
 */
#define REGULATOR_DT_INST_COMMON_CONFIG_INIT(inst)                             \
	REGULATOR_DT_COMMON_CONFIG_INIT(DT_DRV_INST(inst))

/**
 * @brief Common regulator data.
 *
 * This structure **must** be placed first in the driver's data structure.
 */
struct regulator_common_data {
#if defined(CONFIG_REGULATOR_THREAD_SAFE_REFCNT) || defined(__DOXYGEN__)
	/** Lock (only if @kconfig{CONFIG_REGULATOR_THREAD_SAFE_REFCNT}=y) */
	struct k_mutex lock;
#endif
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

/**
 * @brief Common function to initialize the regulator at init time.
 *
 * This function needs to be called after drivers initialize the regulator. It
 * will:
 *
 * - Automatically enable the regulator if it is set to `regulator-boot-on`
 *   or `regulator-always-on` and increase its usage count.
 * - Configure the regulator mode if `regulator-initial-mode` is set.
 * - Ensure regulator voltage is set to a valid range.
 *
 * Regulators that are enabled by default in hardware, must set @p is_enabled to
 * `true`.
 *
 * @param dev Regulator device instance
 * @param is_enabled Indicate if the regulator is enabled by default in
 * hardware.
 *
 * @retval 0 If enabled successfully.
 * @retval -errno Negative errno in case of failure.
 */
int regulator_common_init(const struct device *dev, bool is_enabled);

/**
 * @brief Check if regulator is expected to be enabled at init time.
 *
 * @param dev Regulator device instance
 * @return true If regulator needs to be enabled at init time.
 * @return false If regulator does not need to be enabled at init time.
 */
static inline bool regulator_common_is_init_enabled(const struct device *dev)
{
	const struct regulator_common_config *config =
		(const struct regulator_common_config *)dev->config;

	return (config->flags & REGULATOR_INIT_ENABLED) != 0U;
}

/** @endcond */

/**
 * @brief Regulator Parent Interface
 * @defgroup regulator_parent_interface Regulator Parent Interface
 * @{
 */

/**
 * @brief Set a DVS state.
 *
 * Some PMICs feature DVS (Dynamic Voltage Scaling) by allowing to program the
 * voltage level for multiple states. Such states may be automatically changed
 * by hardware using GPIO pins. Certain MCUs even allow to automatically
 * configure specific output pins when entering low-power modes so that PMIC
 * state is changed without software intervention. This API can be used when
 * state needs to be changed by software.
 *
 * @param dev Parent regulator device instance.
 * @param state DVS state (vendor specific identifier).
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If given state is not supported.
 * @retval -EPERM If state can't be changed by software.
 * @retval -ENOSYS If function is not implemented.
 * @retval -errno In case of any other error.
 */
static inline int regulator_parent_dvs_state_set(const struct device *dev,
						 regulator_dvs_state_t state)
{
	const struct regulator_parent_driver_api *api =
		(const struct regulator_parent_driver_api *)dev->api;

	if (api->dvs_state_set == NULL) {
		return -ENOSYS;
	}

	return api->dvs_state_set(dev, state);
}

/** @} */

/**
 * @brief Enable a regulator.
 *
 * Reference-counted request that a regulator be turned on. A regulator is
 * considered "on" when it has reached a stable/usable state. Regulators that
 * are always on, or configured in devicetree with `regulator-always-on` will
 * always stay enabled, and so this function will always succeed.
 *
 * @param dev Regulator device instance
 *
 * @retval 0 If regulator has been successfully enabled.
 * @retval -errno Negative errno in case of failure.
 */
int regulator_enable(const struct device *dev);

/**
 * @brief Check if a regulator is enabled.
 *
 * @param dev Regulator device instance.
 *
 * @retval true If regulator is enabled.
 * @retval false If regulator is disabled.
 */
bool regulator_is_enabled(const struct device *dev);

/**
 * @brief Disable a regulator.
 *
 * Release a regulator after a previous regulator_enable() completed
 * successfully. Regulators that are always on, or configured in devicetree with
 * `regulator-always-on` will always stay enabled, and so this function will
 * always succeed.
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
 * voltage. The voltage will be applied to the active or selected mode. Output
 * voltage may be limited using `regulator-min-microvolt` and/or
 * `regulator-max-microvolt` in devicetree.
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
int regulator_set_voltage(const struct device *dev, int32_t min_uv,
			  int32_t max_uv);

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
 * configured current limit. Current may be limited using `current-min-microamp`
 * and/or `current-max-microamp` in Devicetree.
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
int regulator_set_current_limit(const struct device *dev, int32_t min_ua,
				int32_t max_ua);

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
 * the regulator. Allowed modes may be limited using `regulator-allowed-modes`
 * devicetree property.
 *
 * @param dev Regulator device instance.
 * @param mode Mode to select for this regulator.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If mode is not supported.
 * @retval -ENOSYS If function is not implemented.
 * @retval -errno In case of any other error.
 */
int regulator_set_mode(const struct device *dev, regulator_mode_t mode);

/**
 * @brief Get mode.
 *
 * @param dev Regulator device instance.
 * @param[out] mode Where mode will be stored.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If function is not implemented.
 * @retval -errno In case of any other error.
 */
static inline int regulator_get_mode(const struct device *dev,
				     regulator_mode_t *mode)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->get_mode == NULL) {
		return -ENOSYS;
	}

	return api->get_mode(dev, mode);
}

/**
 * @brief Get active error flags.
 *
 * @param dev Regulator device instance.
 * @param[out] flags Where error flags will be stored.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If function is not implemented.
 * @retval -errno In case of any other error.
 */
static inline int regulator_get_error_flags(const struct device *dev,
					    regulator_error_flags_t *flags)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	if (api->get_error_flags == NULL) {
		return -ENOSYS;
	}

	return api->get_error_flags(dev, flags);
}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_REGULATOR_H_ */
