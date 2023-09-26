/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adp5360_regulator

#include <errno.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/dt-bindings/regulator/adp5360.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>

/* ADP5360 regulator related registers */
#define ADP5360_BUCK_CFG       0x29U
#define ADP5360_BUCK_OUTPUT    0x2AU
#define ADP5360_BUCKBST_CFG    0x2BU
#define ADP5360_BUCKBST_OUTPUT 0x2CU

/* Buck/boost configure register. */
#define ADP5360_BUCK_CFG_SS_MSK        GENMASK(7, 6)
#define ADP5360_BUCK_CFG_SS_POS        6U
#define ADP5360_BUCK_CFG_BST_ILIM_MSK  GENMASK(5, 3)
#define ADP5360_BUCK_CFG_BST_ILIM_POS  3U
#define ADP5360_BUCK_CFG_BUCK_ILIM_MSK GENMASK(5, 3)
#define ADP5360_BUCK_CFG_BUCK_ILIM_POS 3U
#define ADP5360_BUCK_CFG_BUCK_MODE_MSK BIT(3)
#define ADP5360_BUCK_CFG_BUCK_MODE_POS 3U
#define ADP5360_BUCK_CFG_STP_MSK       BIT(2)
#define ADP5360_BUCK_CFG_DISCHG_MSK    BIT(1)
#define ADP5360_BUCK_CFG_EN_MSK        BIT(0)

/* Buck/boost output voltage setting register. */
#define ADP5360_BUCK_OUTPUT_VOUT_MSK GENMASK(5, 0)
#define ADP5360_BUCK_OUTPUT_VOUT_POS 0U
#define ADP5360_BUCK_OUTPUT_DLY_MSK  GENMASK(7, 6)
#define ADP5360_BUCK_OUTPUT_DLY_POS  6U

struct regulator_adp5360_desc {
	uint8_t cfg_reg;
	uint8_t out_reg;
	bool has_modes;
	const struct linear_range *ranges;
	uint8_t nranges;
};

static const struct linear_range buck_ranges[] = {
	LINEAR_RANGE_INIT(600000, 50000U, 0x0U, 0x3FU),
};

static const struct regulator_adp5360_desc buck_desc = {
	.cfg_reg = ADP5360_BUCK_CFG,
	.out_reg = ADP5360_BUCK_OUTPUT,
	.has_modes = true,
	.ranges = buck_ranges,
	.nranges = ARRAY_SIZE(buck_ranges),
};

static const struct linear_range buckboost_ranges[] = {
	LINEAR_RANGE_INIT(1800000, 100000U, 0x0U, 0x0BU),
	LINEAR_RANGE_INIT(2950000, 50000U, 0xCU, 0x3FU),
};

static const struct regulator_adp5360_desc buckboost_desc = {
	.cfg_reg = ADP5360_BUCKBST_CFG,
	.out_reg = ADP5360_BUCKBST_OUTPUT,
	.has_modes = false,
	.ranges = buckboost_ranges,
	.nranges = ARRAY_SIZE(buckboost_ranges),
};

struct regulator_adp5360_config {
	struct regulator_common_config common;
	struct i2c_dt_spec i2c;
	const struct regulator_adp5360_desc *desc;
	int8_t dly_idx;
	int8_t ss_idx;
	int8_t ilim_idx;
	bool stp_en;
	bool dis_en;
};

struct regulator_adp5360_data {
	struct regulator_common_data data;
};

static unsigned int regulator_adp5360_count_voltages(const struct device *dev)
{
	const struct regulator_adp5360_config *config = dev->config;

	return linear_range_group_values_count(config->desc->ranges, config->desc->nranges);
}

static int regulator_adp5360_list_voltage(const struct device *dev, unsigned int idx,
					  int32_t *volt_uv)
{
	const struct regulator_adp5360_config *config = dev->config;

	return linear_range_group_get_value(config->desc->ranges, config->desc->nranges, idx,
					    volt_uv);
}

static int regulator_adp5360_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_adp5360_config *config = dev->config;
	uint16_t idx;
	int ret;

	ret = linear_range_group_get_win_index(config->desc->ranges, config->desc->nranges, min_uv,
					       max_uv, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	return i2c_reg_update_byte_dt(&config->i2c, config->desc->out_reg,
				      ADP5360_BUCK_OUTPUT_VOUT_MSK,
				      (uint8_t)idx << ADP5360_BUCK_OUTPUT_VOUT_POS);
}

static int regulator_adp5360_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_adp5360_config *config = dev->config;
	int ret;
	uint8_t raw_reg;

	ret = i2c_reg_read_byte_dt(&config->i2c, config->desc->out_reg, &raw_reg);
	if (ret < 0) {
		return ret;
	}

	raw_reg = (raw_reg & ADP5360_BUCK_OUTPUT_VOUT_MSK) >> ADP5360_BUCK_OUTPUT_VOUT_POS;

	return linear_range_group_get_value(config->desc->ranges, config->desc->nranges, raw_reg,
					    volt_uv);
}

static int regulator_adp5360_set_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_adp5360_config *config = dev->config;

	if (!config->desc->has_modes || (mode > ADP5360_MODE_PWM)) {
		return -ENOTSUP;
	}

	return i2c_reg_update_byte_dt(&config->i2c, config->desc->cfg_reg,
				      ADP5360_BUCK_CFG_BUCK_MODE_MSK,
				      mode << ADP5360_BUCK_CFG_BUCK_MODE_POS);
}

static int regulator_adp5360_get_mode(const struct device *dev, regulator_mode_t *mode)
{
	const struct regulator_adp5360_config *config = dev->config;
	uint8_t val;
	int ret;

	if (!config->desc->has_modes) {
		return -ENOTSUP;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, config->desc->cfg_reg, &val);
	if (ret < 0) {
		return ret;
	}

	*mode = (val & ADP5360_BUCK_CFG_BUCK_MODE_MSK) >> ADP5360_BUCK_CFG_BUCK_MODE_POS;

	return 0;
}

static int regulator_adp5360_enable(const struct device *dev)
{
	const struct regulator_adp5360_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, config->desc->cfg_reg, ADP5360_BUCK_CFG_EN_MSK,
				      1U);
}

static int regulator_adp5360_disable(const struct device *dev)
{
	const struct regulator_adp5360_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, config->desc->cfg_reg, ADP5360_BUCK_CFG_EN_MSK,
				      0U);
}

static int regulator_adp5360_init(const struct device *dev)
{
	const struct regulator_adp5360_config *config = dev->config;
	int ret;
	uint8_t val, nval, msk;

	regulator_common_data_init(dev);

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	/* apply optional delay */
	msk = 0U;
	nval = 0U;

	ret = i2c_reg_read_byte_dt(&config->i2c, config->desc->out_reg, &val);
	if (ret < 0) {
		return ret;
	}

	if (config->dly_idx >= 0) {
		msk |= ADP5360_BUCK_OUTPUT_DLY_MSK;
		nval |= ((uint8_t)config->dly_idx << ADP5360_BUCK_OUTPUT_DLY_POS) &
			ADP5360_BUCK_OUTPUT_DLY_MSK;
	}

	if (msk != 0U) {
		ret = i2c_reg_write_byte_dt(&config->i2c, config->desc->out_reg,
					    (val & ~msk) | nval);
		if (ret < 0) {
			return ret;
		}
	}

	/* apply optional initial configuration */
	msk = 0U;
	nval = 0U;

	ret = i2c_reg_read_byte_dt(&config->i2c, config->desc->cfg_reg, &val);
	if (ret < 0) {
		return ret;
	}

	if (config->ss_idx >= 0) {
		msk |= ADP5360_BUCK_CFG_SS_MSK;
		nval |= ((uint8_t)config->ss_idx << ADP5360_BUCK_CFG_SS_POS) &
			ADP5360_BUCK_CFG_SS_MSK;
	}

	if (config->ilim_idx >= 0) {
		if (config->desc->has_modes) {
			msk |= ADP5360_BUCK_CFG_BUCK_ILIM_MSK;
			nval |= ((uint8_t)config->ilim_idx << ADP5360_BUCK_CFG_BUCK_ILIM_POS) &
				ADP5360_BUCK_CFG_BUCK_ILIM_MSK;
		} else {
			msk |= ADP5360_BUCK_CFG_BST_ILIM_MSK;
			nval |= ((uint8_t)config->ilim_idx << ADP5360_BUCK_CFG_BST_ILIM_POS) &
				ADP5360_BUCK_CFG_BST_ILIM_MSK;
		}
	}

	if (config->stp_en) {
		msk |= ADP5360_BUCK_CFG_STP_MSK;
		nval |= ADP5360_BUCK_CFG_STP_MSK;
	}

	if (config->dis_en) {
		msk |= ADP5360_BUCK_CFG_DISCHG_MSK;
		nval |= ADP5360_BUCK_CFG_DISCHG_MSK;
	}

	if (msk != 0U) {
		ret = i2c_reg_write_byte_dt(&config->i2c, config->desc->cfg_reg,
					    (val & ~msk) | nval);
		if (ret < 0) {
			return ret;
		}
	}

	return regulator_common_init(dev, (val & ADP5360_BUCK_CFG_EN_MSK) != 0U);
}

static const struct regulator_driver_api api = {
	.enable = regulator_adp5360_enable,
	.disable = regulator_adp5360_disable,
	.count_voltages = regulator_adp5360_count_voltages,
	.list_voltage = regulator_adp5360_list_voltage,
	.set_voltage = regulator_adp5360_set_voltage,
	.get_voltage = regulator_adp5360_get_voltage,
	.set_mode = regulator_adp5360_set_mode,
	.get_mode = regulator_adp5360_get_mode,
};

#define REGULATOR_ADP5360_DEFINE(node_id, id, name)                                                \
	static struct regulator_adp5360_data data_##id;                                            \
                                                                                                   \
	static const struct regulator_adp5360_config config_##id = {                               \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.i2c = I2C_DT_SPEC_GET(DT_GPARENT(node_id)),                                       \
		.desc = &name##_desc,                                                              \
		.dly_idx = DT_ENUM_IDX_OR(node_id, adi_switch_delay_us, -1),                       \
		.ss_idx = DT_ENUM_IDX_OR(node_id, adi_soft_start_ms, -1),                          \
		.ilim_idx = DT_ENUM_IDX_OR(node_id, adi_ilim_milliamp, -1),                        \
		.stp_en = DT_PROP(node_id, adi_enable_stop_pulse),                                 \
		.dis_en = DT_PROP(node_id, adi_enable_output_discharge),                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, regulator_adp5360_init, NULL, &data_##id, &config_##id,          \
			 POST_KERNEL, CONFIG_REGULATOR_ADP5360_INIT_PRIORITY, &api);

#define REGULATOR_ADP5360_DEFINE_COND(inst, child)                                                 \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                                    \
		    (REGULATOR_ADP5360_DEFINE(DT_INST_CHILD(inst, child), child##inst, child)),    \
		    ())

#define REGULATOR_ADP5360_DEFINE_ALL(inst)                                                         \
	REGULATOR_ADP5360_DEFINE_COND(inst, buck)                                                  \
	REGULATOR_ADP5360_DEFINE_COND(inst, buckboost)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_ADP5360_DEFINE_ALL)
