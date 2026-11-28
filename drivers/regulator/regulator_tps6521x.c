/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Register facts: TPS65214 datasheet SLVSHK7A, TPS65215-Q1 datasheet SLVSHQ9.
 */

#define DT_DRV_COMPAT ti_tps6521x_regulator

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mfd/tps6521x.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>

enum regulator_tps6521x_id {
	TPS6521X_BUCK1,
	TPS6521X_BUCK2,
	TPS6521X_BUCK3,
	TPS6521X_LDO1,
	TPS6521X_LDO2,
	TPS6521X_NUM_RAILS,
};

#define TPS6521X_VSET_MASK GENMASK(5, 0)

/* BUCKx_VOUT registers, the same on both variants */
#define TPS6521X_REG_BUCK3_VOUT 0x08U
#define TPS6521X_REG_BUCK2_VOUT 0x09U
#define TPS6521X_REG_BUCK1_VOUT 0x0AU

#define TPS65214_REG_LDO1_VOUT 0x05U
#define TPS65214_REG_LDO2_VOUT 0x06U
#define TPS65215_REG_LDO2_VOUT 0x05U
#define TPS65215_REG_LDO1_VOUT 0x07U

#define TPS6521X_EN_BUCK1 0U
#define TPS6521X_EN_BUCK2 1U
#define TPS6521X_EN_BUCK3 2U
#define TPS65214_EN_LDO1  5U
#define TPS65214_EN_LDO2  4U
#define TPS65215_EN_LDO1  3U
#define TPS65215_EN_LDO2  5U

/* LDOx_VOUT bits that make an LDO a load switch or put it in bypass */
#define TPS65214_LDO1_LSW_CONFIG BIT(6)
#define TPS65214_LDO2_LSW_CONFIG BIT(7)
#define TPS65215_LDO1_LSW_CONFIG BIT(7)
#define TPS65215_LDO1_BYP_CONFIG BIT(6)
#define TPS65215_LDO2_LSW_CONFIG BIT(6)

struct regulator_tps6521x_rail {
	uint8_t vout_reg;
	uint8_t enable_bit;
	/* load-switch and bypass bits in vout_reg: set, no voltage control */
	uint8_t fixed_mask;
	const struct linear_range *ranges;
	uint8_t num_ranges;
};

/* 0.6-1.4 V in 25 mV steps, 1.4-3.4 V in 100 mV steps; codes above 0x34 read as 3.4 V */
static const struct linear_range buck_ranges[] = {
	LINEAR_RANGE_INIT(600000, 25000, 0x00, 0x1f),
	LINEAR_RANGE_INIT(1400000, 100000, 0x20, 0x34),
};

/* TPS65214 LDO1/LDO2: codes 0x00-0x02 read 0.6 V, codes above 0x38 read 3.3 V */
static const struct linear_range tps65214_ldo_ranges[] = {
	LINEAR_RANGE_INIT(600000, 50000, 0x02, 0x38),
};

/* TPS65215 LDO1: 0.6-3.4 V */
static const struct linear_range tps65215_ldo1_ranges[] = {
	LINEAR_RANGE_INIT(600000, 50000, 0x00, 0x38),
};

/* TPS65215 LDO2: codes below 0x0c read 1.2 V, codes above 0x36 read 3.3 V */
static const struct linear_range tps65215_ldo2_ranges[] = {
	LINEAR_RANGE_INIT(1200000, 50000, 0x0c, 0x36),
};

#define RAIL(_reg, _en, _lsw, _ranges)                                                             \
	{                                                                                          \
		.vout_reg = _reg, .enable_bit = BIT(_en), .fixed_mask = _lsw, .ranges = _ranges,   \
		.num_ranges = ARRAY_SIZE(_ranges)                                                  \
	}

static const struct regulator_tps6521x_rail tps65214_rails[TPS6521X_NUM_RAILS] = {
	[TPS6521X_BUCK1] = RAIL(TPS6521X_REG_BUCK1_VOUT, TPS6521X_EN_BUCK1, 0U, buck_ranges),
	[TPS6521X_BUCK2] = RAIL(TPS6521X_REG_BUCK2_VOUT, TPS6521X_EN_BUCK2, 0U, buck_ranges),
	[TPS6521X_BUCK3] = RAIL(TPS6521X_REG_BUCK3_VOUT, TPS6521X_EN_BUCK3, 0U, buck_ranges),
	[TPS6521X_LDO1] = RAIL(TPS65214_REG_LDO1_VOUT, TPS65214_EN_LDO1, TPS65214_LDO1_LSW_CONFIG,
			       tps65214_ldo_ranges),
	[TPS6521X_LDO2] = RAIL(TPS65214_REG_LDO2_VOUT, TPS65214_EN_LDO2, TPS65214_LDO2_LSW_CONFIG,
			       tps65214_ldo_ranges),
};

static const struct regulator_tps6521x_rail tps65215_rails[TPS6521X_NUM_RAILS] = {
	[TPS6521X_BUCK1] = RAIL(TPS6521X_REG_BUCK1_VOUT, TPS6521X_EN_BUCK1, 0U, buck_ranges),
	[TPS6521X_BUCK2] = RAIL(TPS6521X_REG_BUCK2_VOUT, TPS6521X_EN_BUCK2, 0U, buck_ranges),
	[TPS6521X_BUCK3] = RAIL(TPS6521X_REG_BUCK3_VOUT, TPS6521X_EN_BUCK3, 0U, buck_ranges),
	[TPS6521X_LDO1] = RAIL(TPS65215_REG_LDO1_VOUT, TPS65215_EN_LDO1,
			       TPS65215_LDO1_LSW_CONFIG | TPS65215_LDO1_BYP_CONFIG,
			       tps65215_ldo1_ranges),
	[TPS6521X_LDO2] = RAIL(TPS65215_REG_LDO2_VOUT, TPS65215_EN_LDO2, TPS65215_LDO2_LSW_CONFIG,
			       tps65215_ldo2_ranges),
};

struct regulator_tps6521x_config {
	struct regulator_common_config common;
	const struct device *mfd;
	enum regulator_tps6521x_id id;
};

struct regulator_tps6521x_data {
	struct regulator_common_data data;
	const struct regulator_tps6521x_rail *rail;
	bool fixed;
};

static int regulator_tps6521x_enable(const struct device *dev)
{
	const struct regulator_tps6521x_config *config = dev->config;
	struct regulator_tps6521x_data *data = dev->data;

	return mfd_tps6521x_reg_update(config->mfd, MFD_TPS6521X_REG_ENABLE_CTRL,
				       data->rail->enable_bit, data->rail->enable_bit);
}

static int regulator_tps6521x_disable(const struct device *dev)
{
	const struct regulator_tps6521x_config *config = dev->config;
	struct regulator_tps6521x_data *data = dev->data;

	return mfd_tps6521x_reg_update(config->mfd, MFD_TPS6521X_REG_ENABLE_CTRL,
				       data->rail->enable_bit, 0);
}

static unsigned int regulator_tps6521x_count_voltages(const struct device *dev)
{
	struct regulator_tps6521x_data *data = dev->data;

	if (data->fixed) {
		return 0;
	}

	return linear_range_group_values_count(data->rail->ranges, data->rail->num_ranges);
}

static int regulator_tps6521x_list_voltage(const struct device *dev, unsigned int idx,
					   int32_t *volt_uv)
{
	struct regulator_tps6521x_data *data = dev->data;

	if (data->fixed) {
		return -EINVAL;
	}

	return linear_range_group_get_value(data->rail->ranges, data->rail->num_ranges,
					    data->rail->ranges[0].min_idx + idx, volt_uv);
}

static int regulator_tps6521x_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_tps6521x_config *config = dev->config;
	struct regulator_tps6521x_data *data = dev->data;
	uint16_t idx;
	int ret;

	/* -ENOSYS: regulator_common_init then skips the range snap */
	if (data->fixed) {
		return -ENOSYS;
	}

	/* -ERANGE still gives a valid index inside the window */
	ret = linear_range_group_get_win_index(data->rail->ranges, data->rail->num_ranges, min_uv,
					       max_uv, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	return mfd_tps6521x_reg_update(config->mfd, data->rail->vout_reg, TPS6521X_VSET_MASK,
				       (uint8_t)idx);
}

static int regulator_tps6521x_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_tps6521x_config *config = dev->config;
	struct regulator_tps6521x_data *data = dev->data;
	const struct regulator_tps6521x_rail *rail = data->rail;
	uint8_t val;
	uint16_t idx;
	int ret;

	/* -ENOSYS: regulator_common_init then skips the range snap */
	if (data->fixed) {
		return -ENOSYS;
	}

	ret = mfd_tps6521x_reg_read(config->mfd, rail->vout_reg, &val);
	if (ret < 0) {
		return ret;
	}

	/* codes outside the ranges repeat the end voltages */
	idx = CLAMP(FIELD_GET(TPS6521X_VSET_MASK, val), rail->ranges[0].min_idx,
		    rail->ranges[rail->num_ranges - 1].max_idx);

	return linear_range_group_get_value(rail->ranges, rail->num_ranges, idx, volt_uv);
}

static DEVICE_API(regulator, api) = {
	.enable = regulator_tps6521x_enable,
	.disable = regulator_tps6521x_disable,
	.count_voltages = regulator_tps6521x_count_voltages,
	.list_voltage = regulator_tps6521x_list_voltage,
	.set_voltage = regulator_tps6521x_set_voltage,
	.get_voltage = regulator_tps6521x_get_voltage,
};

static int regulator_tps6521x_init(const struct device *dev)
{
	const struct regulator_tps6521x_config *config = dev->config;
	struct regulator_tps6521x_data *data = dev->data;
	uint8_t enable_ctrl;
	uint8_t vout;
	int ret;

	regulator_common_data_init(dev);

	if (!device_is_ready(config->mfd)) {
		return -ENODEV;
	}

	data->rail = mfd_tps6521x_get_variant(config->mfd) == MFD_TPS6521X_VARIANT_TPS65215
			     ? &tps65215_rails[config->id]
			     : &tps65214_rails[config->id];

	if (data->rail->fixed_mask != 0U) {
		ret = mfd_tps6521x_reg_read(config->mfd, data->rail->vout_reg, &vout);
		if (ret < 0) {
			return ret;
		}
		data->fixed = (vout & data->rail->fixed_mask) != 0U;
	}

	ret = mfd_tps6521x_reg_read(config->mfd, MFD_TPS6521X_REG_ENABLE_CTRL, &enable_ctrl);
	if (ret < 0) {
		return ret;
	}

	return regulator_common_init(dev, (enable_ctrl & data->rail->enable_bit) != 0U);
}

#define REGULATOR_TPS6521X_DEFINE(node_id, name, id_)                                              \
	static struct regulator_tps6521x_data data_##name;                                         \
	static const struct regulator_tps6521x_config config_##name = {                            \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.mfd = DEVICE_DT_GET(DT_GPARENT(node_id)),                                         \
		.id = id_,                                                                         \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, regulator_tps6521x_init, NULL, &data_##name, &config_##name,     \
			 POST_KERNEL, CONFIG_REGULATOR_TPS6521X_INIT_PRIORITY, &api);

#define REGULATOR_TPS6521X_DEFINE_COND(inst, child, id)                                            \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                                    \
		    (REGULATOR_TPS6521X_DEFINE(DT_INST_CHILD(inst, child), child##inst, id)), ())

#define REGULATOR_TPS6521X_DEFINE_ALL(inst)                                                        \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_INST_PARENT(inst), ti_tps65214) ||                      \
			     DT_NODE_HAS_COMPAT(DT_INST_PARENT(inst), ti_tps65215),                \
		     "TPS6521x regulator node must be a child of a TPS65214 or TPS65215 node");    \
	REGULATOR_TPS6521X_DEFINE_COND(inst, buck1, TPS6521X_BUCK1)                                \
	REGULATOR_TPS6521X_DEFINE_COND(inst, buck2, TPS6521X_BUCK2)                                \
	REGULATOR_TPS6521X_DEFINE_COND(inst, buck3, TPS6521X_BUCK3)                                \
	REGULATOR_TPS6521X_DEFINE_COND(inst, ldo1, TPS6521X_LDO1)                                  \
	REGULATOR_TPS6521X_DEFINE_COND(inst, ldo2, TPS6521X_LDO2)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_TPS6521X_DEFINE_ALL)
