/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_regulator

#include <zephyr/device.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/logging/log.h>
#include <hal/ldo_ll.h>

LOG_MODULE_REGISTER(regulator_esp32, CONFIG_REGULATOR_LOG_LEVEL);

struct regulator_esp32_config {
	struct regulator_common_config common;
	int ldo_unit;
};

struct regulator_esp32_data {
	struct regulator_common_data common;
};

static int regulator_esp32_enable(const struct device *dev)
{
	const struct regulator_esp32_config *config = dev->config;

	ldo_ll_set_owner(config->ldo_unit, LDO_LL_UNIT_OWNER_SW);
	ldo_ll_enable_ripple_suppression(config->ldo_unit, true);
	ldo_ll_enable(config->ldo_unit, true);

	return 0;
}

static int regulator_esp32_disable(const struct device *dev)
{
	const struct regulator_esp32_config *config = dev->config;

	ldo_ll_enable(config->ldo_unit, false);

	return 0;
}

static int regulator_esp32_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_esp32_config *config = dev->config;
	int voltage_mv = min_uv / 1000;
	uint8_t dref = 0;
	uint8_t mul = 0;
	bool use_rail = false;

	if (voltage_mv < LDO_LL_RECOMMEND_MIN_VOLTAGE_MV || voltage_mv > LDO_LL_RAIL_VOLTAGE_MV ||
	    min_uv > max_uv) {
		return -EINVAL;
	}

	ldo_ll_voltage_to_dref_mul(config->ldo_unit, voltage_mv, &dref, &mul, &use_rail);
	ldo_ll_adjust_voltage(config->ldo_unit, dref, mul, use_rail);

	return 0;
}

static int regulator_esp32_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(volt_uv);

	return -ENOSYS;
}

static DEVICE_API(regulator, regulator_esp32_api) = {
	.enable = regulator_esp32_enable,
	.disable = regulator_esp32_disable,
	.set_voltage = regulator_esp32_set_voltage,
	.get_voltage = regulator_esp32_get_voltage,
};

static int regulator_esp32_init(const struct device *dev)
{
	regulator_common_data_init(dev);

	return regulator_common_init(dev, true);
}

#define REGULATOR_ESP32_DEFINE(node)                                                               \
	static struct regulator_esp32_data data_##node;                                            \
                                                                                                   \
	static const struct regulator_esp32_config config_##node = {                               \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node),                                   \
		.ldo_unit = LDO_ID2UNIT(DT_PROP(node, espressif_ldo_channel)),                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node, regulator_esp32_init, NULL, &data_##node, &config_##node,           \
			 PRE_KERNEL_1, CONFIG_REGULATOR_ESP32_INIT_PRIORITY,                       \
			 &regulator_esp32_api);

#define REGULATOR_ESP32_DEFINE_ALL(inst)                                                           \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, REGULATOR_ESP32_DEFINE)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_ESP32_DEFINE_ALL)
