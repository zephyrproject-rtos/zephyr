/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fake regulator driver API functions.
 * @ingroup regulator_fake
 */

#ifndef ZEPHYR_DRIVERS_REGULATOR_FAKE_H_
#define ZEPHYR_DRIVERS_REGULATOR_FAKE_H_

#include <zephyr/drivers/regulator.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fake regulator driver API functions.
 * @defgroup regulator_fake Fake regulator
 * @ingroup io_emulators
 * @ingroup regulator_interface
 * @{
 */

/**
 * @brief Enable the fake regulator.
 *
 * @see regulator_enable
 */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_enable, const struct device *);

/**
 * @brief Disable the fake regulator.
 *
 * @see regulator_disable
 */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_disable, const struct device *);

/**
 * @brief Obtain the number of supported voltage levels of the fake regulator.
 *
 * @see regulator_count_voltages
 */
DECLARE_FAKE_VALUE_FUNC(unsigned int, regulator_fake_count_voltages,
			const struct device *);

/**
 * @brief Obtain the value of a fake regulator voltage given an index.
 *
 * @see regulator_list_voltage
 */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_list_voltage, const struct device *,
			unsigned int, int32_t *);

/**
 * @brief Set the output voltage of the fake regulator.
 *
 * @see regulator_set_voltage
 */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_set_voltage, const struct device *,
			int32_t, int32_t);

/**
 * @brief Obtain the output voltage of the fake regulator.
 *
 * @see regulator_get_voltage
 */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_get_voltage, const struct device *,
			int32_t *);

/**
 * @brief Obtain the number of supported current limit levels of the fake regulator.
 *
 * @see regulator_count_current_limits
 */
DECLARE_FAKE_VALUE_FUNC(unsigned int, regulator_fake_count_current_limits, const struct device *);

/**
 * @brief Obtain the value of a fake regulator current limit given an index.
 *
 * @see regulator_list_current_limit
 */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_list_current_limit, const struct device *, unsigned int,
			int32_t *);

/**
 * @brief Set the output current limit of the fake regulator.
 *
 * @see regulator_set_current_limit
 */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_set_current_limit,
			const struct device *, int32_t, int32_t);

/**
 * @brief Get the output current limit of the fake regulator.
 *
 * @see regulator_get_current_limit
 */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_get_current_limit,
			const struct device *, int32_t *);

/**
 * @brief Set the mode of the fake regulator.
 *
 * @see regulator_set_mode
 */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_set_mode, const struct device *,
			regulator_mode_t);

/**
 * @brief Get the mode of the fake regulator.
 *
 * @see regulator_get_mode
 */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_get_mode, const struct device *,
			regulator_mode_t *);

/**
 * @brief Set the active discharge setting of the fake regulator.
 *
 * @see regulator_set_active_discharge
 */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_set_active_discharge, const struct device *,
			bool);

/**
 * @brief Get the active discharge setting of the fake regulator.
 *
 * @see regulator_get_active_discharge
 */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_get_active_discharge, const struct device *,
			bool *);

/**
 * @brief Get the active error flags of the fake regulator.
 *
 * @see regulator_get_error_flags
 */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_get_error_flags,
			const struct device *, regulator_error_flags_t *);

/**
 * @brief Set a DVS state of the fake regulator parent.
 *
 * @see regulator_parent_dvs_state_set
 */
DECLARE_FAKE_VALUE_FUNC(int, regulator_parent_fake_dvs_state_set,
			const struct device *, regulator_dvs_state_t);

/**
 * @brief Enter ship mode on the fake regulator parent.
 *
 * @see regulator_parent_ship_mode
 */
DECLARE_FAKE_VALUE_FUNC(int, regulator_parent_fake_ship_mode,
			const struct device *);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTS_DRIVERS_CAN_SHELL_FAKE_CAN_H_ */
