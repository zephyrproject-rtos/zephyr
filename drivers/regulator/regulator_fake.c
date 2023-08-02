/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_fake_regulator

#include <zephyr/devicetree.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/regulator/fake.h>
#include <zephyr/fff.h>
#include <zephyr/toolchain.h>

/* regulator */

struct regulator_fake_config {
	struct regulator_common_config common;
};

struct regulator_fake_data {
	struct regulator_common_data data;
};

DEFINE_FAKE_VALUE_FUNC(int, regulator_fake_enable, const struct device *);
DEFINE_FAKE_VALUE_FUNC(int, regulator_fake_disable, const struct device *);
DEFINE_FAKE_VALUE_FUNC(unsigned int, regulator_fake_count_voltages,
		       const struct device *);
DEFINE_FAKE_VALUE_FUNC(int, regulator_fake_list_voltage, const struct device *,
		       unsigned int, int32_t *);
DEFINE_FAKE_VALUE_FUNC(int, regulator_fake_set_voltage, const struct device *,
		       int32_t, int32_t);
DEFINE_FAKE_VALUE_FUNC(int, regulator_fake_get_voltage, const struct device *,
		       int32_t *);
DEFINE_FAKE_VALUE_FUNC(int, regulator_fake_set_current_limit,
		       const struct device *, int32_t, int32_t);
DEFINE_FAKE_VALUE_FUNC(int, regulator_fake_get_current_limit,
		       const struct device *, int32_t *);
DEFINE_FAKE_VALUE_FUNC(int, regulator_fake_set_mode, const struct device *,
		       regulator_mode_t);
DEFINE_FAKE_VALUE_FUNC(int, regulator_fake_get_mode, const struct device *,
		       regulator_mode_t *);
DEFINE_FAKE_VALUE_FUNC(int, regulator_fake_set_active_discharge, const struct device *,
		       bool);
DEFINE_FAKE_VALUE_FUNC(int, regulator_fake_get_active_discharge, const struct device *,
		       bool *);
DEFINE_FAKE_VALUE_FUNC(int, regulator_fake_get_error_flags,
		       const struct device *, regulator_error_flags_t *);

static struct regulator_driver_api api = {
	.enable = regulator_fake_enable,
	.disable = regulator_fake_disable,
	.count_voltages = regulator_fake_count_voltages,
	.list_voltage = regulator_fake_list_voltage,
	.set_voltage = regulator_fake_set_voltage,
	.get_voltage = regulator_fake_get_voltage,
	.set_current_limit = regulator_fake_set_current_limit,
	.get_current_limit = regulator_fake_get_current_limit,
	.set_mode = regulator_fake_set_mode,
	.get_mode = regulator_fake_get_mode,
	.set_active_discharge = regulator_fake_set_active_discharge,
	.get_active_discharge = regulator_fake_get_active_discharge,
	.get_error_flags = regulator_fake_get_error_flags,
};

static int regulator_fake_init(const struct device *dev)
{
	regulator_common_data_init(dev);

	return regulator_common_init(dev, false);
}

/* parent regulator */

DEFINE_FAKE_VALUE_FUNC(int, regulator_parent_fake_dvs_state_set,
		       const struct device *, regulator_dvs_state_t);

DEFINE_FAKE_VALUE_FUNC(int, regulator_parent_fake_ship_mode,
		       const struct device *);

static struct regulator_parent_driver_api parent_api = {
	.dvs_state_set = regulator_parent_fake_dvs_state_set,
	.ship_mode = regulator_parent_fake_ship_mode,
};

#define FAKE_DATA_NAME(node_id) _CONCAT(data_, DT_DEP_ORD(node_id))
#define FAKE_CONF_NAME(node_id) _CONCAT(config_, DT_DEP_ORD(node_id))

#define REGULATOR_FAKE_DEFINE(node_id)                                         \
	static struct regulator_fake_data FAKE_DATA_NAME(node_id);             \
                                                                               \
	static const struct regulator_fake_config FAKE_CONF_NAME(node_id) = {  \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),            \
	};                                                                     \
                                                                               \
	DEVICE_DT_DEFINE(node_id, regulator_fake_init, NULL,                   \
			 &FAKE_DATA_NAME(node_id), &FAKE_CONF_NAME(node_id),   \
			 POST_KERNEL, CONFIG_REGULATOR_FAKE_INIT_PRIORITY,     \
			 &api);

#define REGULATOR_FAKE_DEFINE_ALL(inst)                                        \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, NULL, POST_KERNEL,       \
			      CONFIG_REGULATOR_FAKE_COMMON_INIT_PRIORITY,      \
			      &parent_api);                                    \
                                                                               \
	DT_INST_FOREACH_CHILD(inst, REGULATOR_FAKE_DEFINE)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_FAKE_DEFINE_ALL)
