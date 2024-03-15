/*
 * Copyright (c) 2023 Grinn
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max20335_regulator

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/dt-bindings/regulator/max20335.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/linear_range.h>

#define MAX20335_BUCK1_CFG        0x0DU
#define MAX20335_BUCK1_VSET       0x0EU
#define MAX20335_BUCK2_CFG        0x0FU
#define MAX20335_BUCK2_VSET       0x10U
#define MAX20335_BUCK12_CSET      0x11U
#define MAX20335_PWR_CMD	  0x1FU
#define MAX20335_BUCK1_CSET_MASK  0xF0U
#define MAX20335_BUCK2_CSET_MASK  0x0FU
#define MAX20335_BUCK2_CSET_SHIFT 4
#define MAX20335_BUCK_EN          BIT(3)
#define MAX20335_BUCK_EN_MASK     GENMASK(4, 3)

#define MAX20335_LDO1_CFG         0x12U
#define MAX20335_LDO1_VSET        0x13U
#define MAX20335_LDO2_CFG         0x14U
#define MAX20335_LDO2_VSET        0x15U
#define MAX20335_LDO3_CFG         0x16U
#define MAX20335_LDO3_VSET        0x17U
#define MAX20335_LDO_MODE_MASK    BIT(0)
#define MAX20335_LDO_EN           BIT(1)
#define MAX20335_LDO_EN_MASK      GENMASK(2, 1)

#define MAX20335_OFF_MODE	  0xB2U

enum max20335_pmic_sources {
	MAX20335_PMIC_SOURCE_BUCK1,
	MAX20335_PMIC_SOURCE_BUCK2,
	MAX20335_PMIC_SOURCE_LDO1,
	MAX20335_PMIC_SOURCE_LDO2,
	MAX20335_PMIC_SOURCE_LDO3,
};

struct regulator_max20335_desc {
	uint8_t vsel_reg;
	uint8_t enable_mask;
	uint8_t enable_val;
	uint8_t cfg_reg;
	const struct linear_range *uv_range;
	const struct linear_range *ua_range;
};

struct regulator_max20335_common_config {
	struct i2c_dt_spec bus;
};

struct regulator_max20335_config {
	struct regulator_common_config common;
	struct i2c_dt_spec bus;
	const struct regulator_max20335_desc *desc;
	uint8_t source;
};

struct regulator_max20335_data {
	struct regulator_common_data common;
};

static const struct linear_range buck1_range = LINEAR_RANGE_INIT(700000, 25000U, 0x0U, 0x3FU);
static const struct linear_range buck2_range = LINEAR_RANGE_INIT(700000, 50000U, 0x0U, 0x3FU);
static const struct linear_range buck12_current_limit_range =
	LINEAR_RANGE_INIT(50000, 25000U, 0x02U, 0x0FU);
static const struct linear_range ldo1_range = LINEAR_RANGE_INIT(800000, 100000U, 0x0U, 0x1CU);
static const struct linear_range ldo23_range = LINEAR_RANGE_INIT(900000, 100000U, 0x0U, 0x1FU);

static const struct regulator_max20335_desc __maybe_unused buck1_desc = {
	.vsel_reg = MAX20335_BUCK1_VSET,
	.enable_mask = MAX20335_BUCK_EN_MASK,
	.enable_val = MAX20335_BUCK_EN,
	.cfg_reg = MAX20335_BUCK1_CFG,
	.uv_range = &buck1_range,
	.ua_range = &buck12_current_limit_range,
};

static const struct regulator_max20335_desc __maybe_unused buck2_desc = {
	.vsel_reg = MAX20335_BUCK2_VSET,
	.enable_mask = MAX20335_BUCK_EN_MASK,
	.enable_val = MAX20335_BUCK_EN,
	.cfg_reg = MAX20335_BUCK2_CFG,
	.uv_range = &buck2_range,
	.ua_range = &buck12_current_limit_range,
};

static const struct regulator_max20335_desc __maybe_unused ldo1_desc = {
	.vsel_reg = MAX20335_LDO1_VSET,
	.enable_mask = MAX20335_LDO_EN_MASK,
	.enable_val = MAX20335_LDO_EN,
	.cfg_reg = MAX20335_LDO1_CFG,
	.uv_range = &ldo1_range,
};

static const struct regulator_max20335_desc __maybe_unused ldo2_desc = {
	.vsel_reg = MAX20335_LDO2_VSET,
	.enable_mask = MAX20335_LDO_EN_MASK,
	.enable_val = MAX20335_LDO_EN,
	.cfg_reg = MAX20335_LDO2_CFG,
	.uv_range = &ldo23_range,
};

static const struct regulator_max20335_desc __maybe_unused ldo3_desc = {
	.vsel_reg = MAX20335_LDO3_VSET,
	.enable_mask = MAX20335_LDO_EN_MASK,
	.enable_val = MAX20335_LDO_EN,
	.cfg_reg = MAX20335_LDO3_CFG,
	.uv_range = &ldo23_range,
};

static int regulator_max20335_set_enable(const struct device *dev, bool enable)
{
	const struct regulator_max20335_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->bus,
				      config->desc->cfg_reg,
				      config->desc->enable_mask,
				      enable ? config->desc->enable_val : 0);
}

static int regulator_max20335_enable(const struct device *dev)
{
	return regulator_max20335_set_enable(dev, true);
}

static int regulator_max20335_disable(const struct device *dev)
{
	return regulator_max20335_set_enable(dev, false);
}

static int regulator_max20335_set_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_max20335_config *config = dev->config;

	if (mode > MAX20335_LOAD_SWITCH_MODE) {
		return -ENOTSUP;
	}

	switch (config->source) {
	case MAX20335_PMIC_SOURCE_LDO1:
		__fallthrough;
	case MAX20335_PMIC_SOURCE_LDO2:
		__fallthrough;
	case MAX20335_PMIC_SOURCE_LDO3:
		return i2c_reg_update_byte_dt(&config->bus,
					      config->desc->cfg_reg,
					      MAX20335_LDO_MODE_MASK,
					      mode);
	default:
		return -ENOTSUP;
	}
}

static unsigned int regulator_max20335_count_voltages(const struct device *dev)
{
	const struct regulator_max20335_config *config = dev->config;

	return linear_range_values_count(config->desc->uv_range);
}

static int regulator_max20335_list_voltage(const struct device *dev, unsigned int idx,
					   int32_t *volt_uv)
{
	const struct regulator_max20335_config *config = dev->config;

	return linear_range_get_value(config->desc->uv_range, idx, volt_uv);
}

static int regulator_max20335_set_buck_ldo_voltage(const struct device *dev, int32_t min_uv,
						   int32_t max_uv, const struct linear_range *range,
						   uint8_t vout_reg)
{
	const struct regulator_max20335_config *config = dev->config;
	uint16_t idx;
	int ret;

	ret = linear_range_get_win_index(range, min_uv, max_uv, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	return i2c_reg_write_byte_dt(&config->bus, vout_reg, (uint8_t)idx);
}

static int regulator_max20335_buck12_ldo123_get_voltage(const struct device *dev,
							const struct linear_range *range,
							uint8_t vout_reg, int32_t *volt_uv)
{
	const struct regulator_max20335_config *config = dev->config;
	uint8_t idx;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->bus, vout_reg, &idx);
	if (ret < 0) {
		return ret;
	}

	return linear_range_get_value(range, idx, volt_uv);
}

static int regulator_max20335_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_max20335_config *config = dev->config;

	return regulator_max20335_buck12_ldo123_get_voltage(dev,
							    config->desc->uv_range,
							    config->desc->vsel_reg,
							    volt_uv);
}

static int regulator_max20335_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_max20335_config *config = dev->config;

	return regulator_max20335_set_buck_ldo_voltage(dev,
						       min_uv,
						       max_uv,
						       config->desc->uv_range,
						       config->desc->vsel_reg);
}

static unsigned int regulator_max20335_count_current_limits(const struct device *dev)
{
	const struct regulator_max20335_config *config = dev->config;

	if (config->source != MAX20335_PMIC_SOURCE_BUCK1 &&
	    config->source != MAX20335_PMIC_SOURCE_BUCK2) {
		return -ENOTSUP;
	}

	return linear_range_values_count(config->desc->ua_range);
}

static int regulator_max20335_list_current_limit(const struct device *dev, unsigned int idx,
						 int32_t *current_ua)
{
	const struct regulator_max20335_config *config = dev->config;

	if (config->source != MAX20335_PMIC_SOURCE_BUCK1 &&
	    config->source != MAX20335_PMIC_SOURCE_BUCK2) {
		return -ENOTSUP;
	}

	return linear_range_get_value(config->desc->ua_range, idx, current_ua);
}

static int regulator_max20335_set_current_limit(const struct device *dev,
						int32_t min_ua,
						int32_t max_ua)
{
	const struct regulator_max20335_config *config = dev->config;
	uint8_t val;
	uint16_t idx;
	int ret;

	if (config->source != MAX20335_PMIC_SOURCE_BUCK1 &&
	    config->source != MAX20335_PMIC_SOURCE_BUCK2) {
		return -ENOTSUP;
	}

	ret = i2c_reg_read_byte_dt(&config->bus, MAX20335_BUCK12_CSET, &val);
	if (ret < 0) {
		return ret;
	}

	ret = linear_range_get_win_index(config->desc->ua_range, min_ua, max_ua, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	switch (config->source) {
	case MAX20335_PMIC_SOURCE_BUCK1:
		val = idx | (val & MAX20335_BUCK1_CSET_MASK);
		break;
	case MAX20335_PMIC_SOURCE_BUCK2:
		val = (idx << MAX20335_BUCK2_CSET_SHIFT) | (val & MAX20335_BUCK2_CSET_MASK);
		break;
	default:
		return -ENOTSUP;
	}

	return i2c_reg_write_byte_dt(&config->bus, MAX20335_BUCK12_CSET, val);
}

static int regulator_max20335_power_off(const struct device *dev)
{
	const struct regulator_max20335_common_config *common_config = dev->config;

	return i2c_reg_write_byte_dt(&common_config->bus, MAX20335_PWR_CMD, MAX20335_OFF_MODE);
}

static int regulator_max20335_init(const struct device *dev)
{
	const struct regulator_max20335_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->bus)) {
		return -ENODEV;
	}

	regulator_common_data_init(dev);

	return regulator_common_init(dev, false);
}

static int regulator_max20335_common_init(const struct device *dev)
{
	const struct regulator_max20335_common_config *common_config = dev->config;

	if (!i2c_is_ready_dt(&common_config->bus)) {
		return -ENODEV;
	}

	return 0;
}

static const struct regulator_parent_driver_api parent_api = {
	.ship_mode = regulator_max20335_power_off,
};

static const struct regulator_driver_api api = {
	.enable = regulator_max20335_enable,
	.disable = regulator_max20335_disable,
	.set_mode = regulator_max20335_set_mode,
	.count_voltages = regulator_max20335_count_voltages,
	.list_voltage = regulator_max20335_list_voltage,
	.set_voltage = regulator_max20335_set_voltage,
	.get_voltage = regulator_max20335_get_voltage,
	.count_current_limits = regulator_max20335_count_current_limits,
	.list_current_limit = regulator_max20335_list_current_limit,
	.set_current_limit = regulator_max20335_set_current_limit,
};

#define REGULATOR_MAX20335_DEFINE(node_id, id, child_name, _source)				\
	static const struct regulator_max20335_config regulator_max20335_config_##id = {	\
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),				\
		.bus = I2C_DT_SPEC_GET(DT_GPARENT(node_id)),					\
		.desc = &child_name##_desc,							\
		.source = _source,								\
	};											\
												\
	static struct regulator_max20335_data regulator_max20335_data_##id;			\
	DEVICE_DT_DEFINE(node_id, regulator_max20335_init, NULL,				\
			 &regulator_max20335_data_##id,						\
			 &regulator_max20335_config_##id,					\
			 POST_KERNEL,								\
			 CONFIG_REGULATOR_MAXIM_MAX20335_INIT_PRIORITY,				\
			 &api);

#define REGULATOR_MAX20335_DEFINE_COND(inst, child, source)					\
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),					\
		    (REGULATOR_MAX20335_DEFINE(DT_INST_CHILD(inst, child),			\
					       child##inst, child, source)),			\
		    ())

#define REGULATOR_MAX20335_DEFINE_ALL(inst)							\
	static const struct regulator_max20335_common_config common_config_##inst = {		\
		.bus = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),					\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, regulator_max20335_common_init,				\
			      NULL, NULL, &common_config_##inst, POST_KERNEL,			\
			      CONFIG_REGULATOR_MAXIM_MAX20335_COMMON_INIT_PRIORITY,		\
			      &parent_api);							\
												\
	REGULATOR_MAX20335_DEFINE_COND(inst, buck1, MAX20335_PMIC_SOURCE_BUCK1)			\
	REGULATOR_MAX20335_DEFINE_COND(inst, buck2, MAX20335_PMIC_SOURCE_BUCK2)			\
	REGULATOR_MAX20335_DEFINE_COND(inst, ldo1, MAX20335_PMIC_SOURCE_LDO1)			\
	REGULATOR_MAX20335_DEFINE_COND(inst, ldo2, MAX20335_PMIC_SOURCE_LDO2)			\
	REGULATOR_MAX20335_DEFINE_COND(inst, ldo3, MAX20335_PMIC_SOURCE_LDO3)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_MAX20335_DEFINE_ALL)
