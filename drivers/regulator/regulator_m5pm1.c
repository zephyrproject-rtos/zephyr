/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT m5stack_m5pm1_regulator

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mfd/m5pm1.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(regulator_m5pm1, CONFIG_REGULATOR_LOG_LEVEL);

#define M5PM1_REG_PWR_CFG 0x06

#define M5PM1_PWR_CFG_DCDC_3V3_EN BIT(1)
#define M5PM1_PWR_CFG_LDO_3V3_EN  BIT(2)
#define M5PM1_PWR_CFG_BOOST_5V_EN BIT(3)

struct regulator_m5pm1_desc {
	uint8_t enable_mask;
	int32_t voltage_uv;
};

struct regulator_m5pm1_config {
	struct regulator_common_config common;
	const struct regulator_m5pm1_desc *desc;
	const struct device *mfd;
};

struct regulator_m5pm1_data {
	struct regulator_common_data common;
};

static const struct regulator_m5pm1_desc m5pm1_dcdc_3v3_desc = {
	.enable_mask = M5PM1_PWR_CFG_DCDC_3V3_EN,
	.voltage_uv = 3300000,
};

static const struct regulator_m5pm1_desc m5pm1_ldo_3v3_desc = {
	.enable_mask = M5PM1_PWR_CFG_LDO_3V3_EN,
	.voltage_uv = 3300000,
};

static const struct regulator_m5pm1_desc m5pm1_boost_5v_desc = {
	.enable_mask = M5PM1_PWR_CFG_BOOST_5V_EN,
	.voltage_uv = 5000000,
};

static int regulator_m5pm1_enable(const struct device *dev)
{
	const struct regulator_m5pm1_config *config = dev->config;

	return mfd_m5pm1_update_reg(config->mfd, M5PM1_REG_PWR_CFG, config->desc->enable_mask,
				    config->desc->enable_mask);
}

static int regulator_m5pm1_disable(const struct device *dev)
{
	const struct regulator_m5pm1_config *config = dev->config;

	return mfd_m5pm1_update_reg(config->mfd, M5PM1_REG_PWR_CFG, config->desc->enable_mask, 0);
}

static unsigned int regulator_m5pm1_count_voltages(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static int regulator_m5pm1_list_voltage(const struct device *dev, unsigned int idx,
					int32_t *volt_uv)
{
	const struct regulator_m5pm1_config *config = dev->config;

	if (idx != 0) {
		return -EINVAL;
	}

	*volt_uv = config->desc->voltage_uv;

	return 0;
}

static int regulator_m5pm1_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_m5pm1_config *config = dev->config;

	*volt_uv = config->desc->voltage_uv;

	return 0;
}

static DEVICE_API(regulator, regulator_m5pm1_api) = {
	.enable = regulator_m5pm1_enable,
	.disable = regulator_m5pm1_disable,
	.count_voltages = regulator_m5pm1_count_voltages,
	.list_voltage = regulator_m5pm1_list_voltage,
	.get_voltage = regulator_m5pm1_get_voltage,
};

static int regulator_m5pm1_init(const struct device *dev)
{
	const struct regulator_m5pm1_config *config = dev->config;
	bool is_enabled;
	uint8_t pwr_cfg;
	int ret;

	regulator_common_data_init(dev);

	if (!device_is_ready(config->mfd)) {
		LOG_ERR_DEVICE_NOT_READY(config->mfd);
		return -ENODEV;
	}

	ret = mfd_m5pm1_read_reg(config->mfd, M5PM1_REG_PWR_CFG, &pwr_cfg);
	if (ret < 0) {
		return ret;
	}

	is_enabled = (pwr_cfg & config->desc->enable_mask) != 0U;

	return regulator_common_init(dev, is_enabled);
}

#define M5PM1_REGULATOR_DEFINE(node_id, id)                                                        \
	static struct regulator_m5pm1_data data_##id;                                              \
	static const struct regulator_m5pm1_config config_##id = {                                 \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.desc = &m5pm1_##id##_desc,                                                        \
		.mfd = DEVICE_DT_GET(DT_GPARENT(node_id)),                                         \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, regulator_m5pm1_init, NULL, &data_##id, &config_##id,            \
			 POST_KERNEL, CONFIG_REGULATOR_M5PM1_INIT_PRIORITY, &regulator_m5pm1_api);

#define M5PM1_REGULATOR_DEFINE_COND(node, child)                                                   \
	COND_CODE_1(DT_NODE_EXISTS(DT_CHILD(node, child)),                                         \
		    (M5PM1_REGULATOR_DEFINE(DT_CHILD(node, child), child)), ())

#define M5PM1_REGULATOR_DEFINE_ALL(node)                                                           \
	M5PM1_REGULATOR_DEFINE_COND(node, dcdc_3v3)                                                \
	M5PM1_REGULATOR_DEFINE_COND(node, ldo_3v3)                                                 \
	M5PM1_REGULATOR_DEFINE_COND(node, boost_5v)

DT_FOREACH_STATUS_OKAY(m5stack_m5pm1_regulator, M5PM1_REGULATOR_DEFINE_ALL)
