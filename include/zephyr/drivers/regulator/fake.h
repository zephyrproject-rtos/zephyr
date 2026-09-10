/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fake regulator driver API functions.
 * @ingroup regulator_fake
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_REGULATOR_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_REGULATOR_FAKE_H_

#include <zephyr/drivers/regulator.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fake regulator driver
 * @defgroup regulator_fake Fake regulator
 * @ingroup io_emulators
 * @ingroup regulator_interface
 *
 * @driver_fake{regulator_interface,CONFIG_REGULATOR_FAKE,zephyr\,fake-regulator}
 *
 * A @dtcompatible{zephyr\,fake-regulator} node is instantiated as the regulator
 * parent device and each of its child nodes as a regulator device.
 *
 * `regulator_fake_get_voltage()` is given a default `custom_fake` returning 1 V,
 * re-installed on every reset. Install your own `custom_fake` in the test case
 * to change what it reports.
 *
 * @{
 */

/** @fake_of{regulator_driver_api::enable} */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_enable, const struct device *);
/** @fake_of{regulator_driver_api::disable} */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_disable, const struct device *);
/** @fake_of{regulator_driver_api::count_voltages} */
DECLARE_FAKE_VALUE_FUNC(unsigned int, regulator_fake_count_voltages,
			const struct device *);
/** @fake_of{regulator_driver_api::list_voltage} */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_list_voltage, const struct device *,
			unsigned int, int32_t *);
/** @fake_of{regulator_driver_api::set_voltage} */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_set_voltage, const struct device *,
			int32_t, int32_t);
/** @fake_of{regulator_driver_api::get_voltage} */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_get_voltage, const struct device *,
			int32_t *);
/** @fake_of{regulator_driver_api::count_current_limits} */
DECLARE_FAKE_VALUE_FUNC(unsigned int, regulator_fake_count_current_limits, const struct device *);
/** @fake_of{regulator_driver_api::list_current_limit} */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_list_current_limit, const struct device *, unsigned int,
			int32_t *);
/** @fake_of{regulator_driver_api::set_current_limit} */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_set_current_limit,
			const struct device *, int32_t, int32_t);
/** @fake_of{regulator_driver_api::get_current_limit} */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_get_current_limit,
			const struct device *, int32_t *);
/** @fake_of{regulator_driver_api::set_mode} */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_set_mode, const struct device *,
			regulator_mode_t);
/** @fake_of{regulator_driver_api::get_mode} */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_get_mode, const struct device *,
			regulator_mode_t *);
/** @fake_of{regulator_driver_api::set_active_discharge} */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_set_active_discharge, const struct device *,
			bool);
/** @fake_of{regulator_driver_api::get_active_discharge} */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_get_active_discharge, const struct device *,
			bool *);
/** @fake_of{regulator_driver_api::get_error_flags} */
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_get_error_flags,
			const struct device *, regulator_error_flags_t *);

/** @fake_of{regulator_parent_driver_api::dvs_state_set} */
DECLARE_FAKE_VALUE_FUNC(int, regulator_parent_fake_dvs_state_set,
			const struct device *, regulator_dvs_state_t);
/** @fake_of{regulator_parent_driver_api::ship_mode} */
DECLARE_FAKE_VALUE_FUNC(int, regulator_parent_fake_ship_mode,
			const struct device *);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_REGULATOR_FAKE_H_ */
