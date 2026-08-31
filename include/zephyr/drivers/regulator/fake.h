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
 * The fake regulator driver implements every regulator and regulator parent
 * API callback as a Fake Function Framework (FFF) fake. It is enabled by
 * @kconfig{CONFIG_REGULATOR_FAKE}. A @dtcompatible{zephyr,fake-regulator}
 * devicetree node is instantiated as the regulator parent device, and each of
 * its child nodes as a regulator device.
 *
 * Each fake is named after the API function it backs, with `regulator_` or
 * `regulator_parent_` followed by `fake_` (`regulator_fake_enable()` for
 * `regulator_enable()`, and so on), and is paired with an FFF control structure
 * carrying an additional `_fake` suffix (`regulator_fake_enable_fake`). Test
 * suites include this header to set return values, install custom fakes, or
 * inspect call counts and captured arguments. See @rstref{mocking-fff}.
 *
 * Unlike the other fake drivers, this driver installs no ztest rule, so the
 * fakes are not reset between test cases. `regulator_fake_get_voltage()` is
 * given a default `custom_fake` returning 1 V, installed at device
 * initialization time only, so resetting that fake discards it for the rest of
 * the test binary. FFF gives `custom_fake` precedence over `return_val`.
 *
 * @code{.c}
 * const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(reg0));
 * int32_t volt_uv;
 *
 * // Nothing resets these fakes automatically. Note that resetting
 * // also drops the default get_voltage() delegate.
 * RESET_FAKE(regulator_fake_get_voltage);
 * regulator_fake_get_voltage_fake.return_val = -EIO;
 *
 * zassert_equal(-EIO, regulator_get_voltage(dev, &volt_uv));
 * zassert_equal(1, regulator_fake_get_voltage_fake.call_count);
 * @endcode
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */

DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_enable, const struct device *);
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_disable, const struct device *);
DECLARE_FAKE_VALUE_FUNC(unsigned int, regulator_fake_count_voltages,
			const struct device *);
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_list_voltage, const struct device *,
			unsigned int, int32_t *);
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_set_voltage, const struct device *,
			int32_t, int32_t);
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_get_voltage, const struct device *,
			int32_t *);
DECLARE_FAKE_VALUE_FUNC(unsigned int, regulator_fake_count_current_limits, const struct device *);
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_list_current_limit, const struct device *, unsigned int,
			int32_t *);
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_set_current_limit,
			const struct device *, int32_t, int32_t);
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_get_current_limit,
			const struct device *, int32_t *);
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_set_mode, const struct device *,
			regulator_mode_t);
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_get_mode, const struct device *,
			regulator_mode_t *);
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_set_active_discharge, const struct device *,
			bool);
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_get_active_discharge, const struct device *,
			bool *);
DECLARE_FAKE_VALUE_FUNC(int, regulator_fake_get_error_flags,
			const struct device *, regulator_error_flags_t *);

DECLARE_FAKE_VALUE_FUNC(int, regulator_parent_fake_dvs_state_set,
			const struct device *, regulator_dvs_state_t);
DECLARE_FAKE_VALUE_FUNC(int, regulator_parent_fake_ship_mode,
			const struct device *);

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_REGULATOR_FAKE_H_ */
