/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_REGULATOR_FAKE_H_
#define ZEPHYR_DRIVERS_REGULATOR_FAKE_H_

#include <zephyr/drivers/regulator.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTS_DRIVERS_CAN_SHELL_FAKE_CAN_H_ */
