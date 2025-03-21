/*
 * Copyright (c) 2024 Arduino SA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pf1550_regulator

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/linear_range.h>

#define PMIC_DEVICE_ID         0x00
#define PMIC_OTP_FLAVOR        0x01
#define PMIC_SILICON_REV       0x02
#define PMIC_INT_CATEGORY      0x06
#define PMIC_SW_INT_STAT0      0x08
#define PMIC_SW_INT_MASK0      0x09
#define PMIC_SW_INT_SENSE0     0x0A
#define PMIC_SW_INT_STAT1      0x0B
#define PMIC_SW_INT_MASK1      0x0C
#define PMIC_SW_INT_SENSE1     0x0D
#define PMIC_SW_INT_STAT2      0x0E
#define PMIC_SW_INT_MASK2      0x0F
#define PMIC_SW_INT_SENSE2     0x10
#define PMIC_LDO_INT_STAT0     0x18
#define PMIC_LDO_INT_MASK0     0x19
#define PMIC_LDO_INT_SENSE0    0x1A
#define PMIC_TEMP_INT_STAT0    0x20
#define PMIC_TEMP_INT_MASK0    0x21
#define PMIC_TEMP_INT_SENSE0   0x22
#define PMIC_ONKEY_INT_STAT0   0x24
#define PMIC_ONKEY_INT_MASK0   0x25
#define PMIC_ONKEY_INT_SENSE0  0x26
#define PMIC_MISC_INT_STAT0    0x28
#define PMIC_MISC_INT_MASK0    0x29
#define PMIC_MISC_INT_SENSE0   0x2A
#define PMIC_COINCELL_CONTROL  0x30
#define PMIC_SW1_VOLT          0x32
#define PMIC_SW1_STBY_VOLT     0x33
#define PMIC_SW1_SLP_VOLT      0x34
#define PMIC_SW1_CTRL          0x35
#define PMIC_SW1_CTRL1         0x36
#define PMIC_SW2_VOLT          0x38
#define PMIC_SW2_STBY_VOLT     0x39
#define PMIC_SW2_SLP_VOLT      0x3A
#define PMIC_SW2_CTRL          0x3B
#define PMIC_SW2_CTRL1         0x3C
#define PMIC_SW3_VOLT          0x3E
#define PMIC_SW3_STBY_VOLT     0x3F
#define PMIC_SW3_SLP_VOLT      0x40
#define PMIC_SW3_CTRL          0x41
#define PMIC_SW3_CTRL1         0x42
#define PMIC_VSNVS_CTRL        0x48
#define PMIC_VREFDDR_CTRL      0x4A
#define PMIC_LDO1_VOLT         0x4C
#define PMIC_LDO1_CTRL         0x4D
#define PMIC_LDO2_VOLT         0x4F
#define PMIC_LDO2_CTRL         0x50
#define PMIC_LDO3_VOLT         0x52
#define PMIC_LDO3_CTRL         0x53
#define PMIC_PWRCTRL0          0x58
#define PMIC_PWRCTRL1          0x59
#define PMIC_PWRCTRL2          0x5A
#define PMIC_PWRCTRL3          0x5B
#define PMIC_SW1_PWRDN_SEQ     0x5F
#define PMIC_SW2_PWRDN_SEQ     0x60
#define PMIC_SW3_PWRDN_SEQ     0x61
#define PMIC_LDO1_PWRDN_SEQ    0x62
#define PMIC_LDO2_PWRDN_SEQ    0x63
#define PMIC_LDO3_PWRDN_SEQ    0x64
#define PMIC_VREFDDR_PWRDN_SEQ 0x65
#define PMIC_STATE_INFO        0x67
#define PMIC_I2C_ADDR          0x68
#define PMIC_RC_16MHZ          0x6B
#define PMIC_KEY1              0x6B

enum pf1550_pmic_sources {
	PF1550_PMIC_SOURCE_BUCK1,
	PF1550_PMIC_SOURCE_BUCK2,
	PF1550_PMIC_SOURCE_BUCK3,
	PF1550_PMIC_SOURCE_LDO1,
	PF1550_PMIC_SOURCE_LDO2,
	PF1550_PMIC_SOURCE_LDO3,
};

struct regulator_pf1550_desc {
	uint8_t vsel_reg;
	uint8_t enable_mask;
	uint8_t enable_val;
	uint8_t cfg_reg;
	const struct linear_range *uv_range;
	uint8_t uv_nranges;
	const struct linear_range *ua_range;
	uint8_t ua_nranges;
};

struct regulator_pf1550_common_config {
	struct i2c_dt_spec bus;
};

struct regulator_pf1550_config {
	struct regulator_common_config common;
	struct i2c_dt_spec bus;
	const struct regulator_pf1550_desc *desc;
	uint8_t source;
};

struct regulator_pf1550_data {
	struct regulator_common_data common;
};

/*
 * Output voltage for BUCK1/2 with DVS disabled (OTP_SWx_DVS_SEL =  1).
 * This is needed to reach the 3V3 maximum range
 */
static const struct linear_range buck12_range[] = {
	LINEAR_RANGE_INIT(1100000, 0, 0, 0), LINEAR_RANGE_INIT(1200000, 0, 1, 1),
	LINEAR_RANGE_INIT(1350000, 0, 2, 2), LINEAR_RANGE_INIT(1500000, 0, 3, 3),
	LINEAR_RANGE_INIT(1800000, 0, 4, 4), LINEAR_RANGE_INIT(2500000, 0, 5, 5),
	LINEAR_RANGE_INIT(3000000, 0, 6, 6), LINEAR_RANGE_INIT(3300000, 0, 7, 7),
};
static const struct linear_range buck3_range[] = {
	LINEAR_RANGE_INIT(1800000, 100000, 0, 15),
};
static const struct linear_range buck123_current_limit_range[] = {
	LINEAR_RANGE_INIT(1000000, 0, 0, 0),
	LINEAR_RANGE_INIT(1200000, 0, 0, 0),
	LINEAR_RANGE_INIT(1500000, 0, 0, 0),
	LINEAR_RANGE_INIT(2000000, 0, 0, 0),
};
static const struct linear_range ldo13_range[] = {
	LINEAR_RANGE_INIT(750000, 50000, 0, 15),
	LINEAR_RANGE_INIT(1800000, 100000, 16, 31),
};
static const struct linear_range ldo2_range[] = {
	LINEAR_RANGE_INIT(1800000, 100000, 0, 15),
};

#define PF1550_RAIL_EN        BIT(0)
#define PF1550_RAIL_EN_MASK   GENMASK(1, 0)
#define PF1550_GOTO_SHIP      BIT(0)
#define PF1550_GOTO_SHIP_MASK GENMASK(1, 0)

static const struct regulator_pf1550_desc __maybe_unused buck1_desc = {
	.vsel_reg = PMIC_SW1_VOLT,
	.enable_mask = PF1550_RAIL_EN,
	.enable_val = PF1550_RAIL_EN_MASK,
	.cfg_reg = PMIC_SW1_CTRL,
	.uv_range = buck12_range,
	.uv_nranges = ARRAY_SIZE(buck12_range),
	.ua_range = buck123_current_limit_range,
	.ua_nranges = ARRAY_SIZE(buck123_current_limit_range),
};

static const struct regulator_pf1550_desc __maybe_unused buck2_desc = {
	.vsel_reg = PMIC_SW2_VOLT,
	.enable_mask = PF1550_RAIL_EN,
	.enable_val = PF1550_RAIL_EN_MASK,
	.cfg_reg = PMIC_SW2_CTRL,
	.uv_range = buck12_range,
	.uv_nranges = ARRAY_SIZE(buck12_range),
	.ua_range = buck123_current_limit_range,
	.ua_nranges = ARRAY_SIZE(buck123_current_limit_range),
};

static const struct regulator_pf1550_desc __maybe_unused buck3_desc = {
	.vsel_reg = PMIC_SW3_VOLT,
	.enable_mask = PF1550_RAIL_EN,
	.enable_val = PF1550_RAIL_EN_MASK,
	.cfg_reg = PMIC_SW3_CTRL,
	.uv_range = buck3_range,
	.uv_nranges = ARRAY_SIZE(buck3_range),
	.ua_range = buck123_current_limit_range,
	.ua_nranges = ARRAY_SIZE(buck123_current_limit_range),
};

static const struct regulator_pf1550_desc __maybe_unused ldo1_desc = {
	.vsel_reg = PMIC_LDO1_VOLT,
	.enable_mask = PF1550_RAIL_EN,
	.enable_val = PF1550_RAIL_EN_MASK,
	.cfg_reg = PMIC_LDO1_CTRL,
	.uv_range = ldo13_range,
	.uv_nranges = ARRAY_SIZE(ldo13_range),
};

static const struct regulator_pf1550_desc __maybe_unused ldo2_desc = {
	.vsel_reg = PMIC_LDO2_VOLT,
	.enable_mask = PF1550_RAIL_EN,
	.enable_val = PF1550_RAIL_EN_MASK,
	.cfg_reg = PMIC_LDO2_CTRL,
	.uv_range = ldo2_range,
	.uv_nranges = ARRAY_SIZE(ldo2_range),
};

static const struct regulator_pf1550_desc __maybe_unused ldo3_desc = {
	.vsel_reg = PMIC_LDO3_VOLT,
	.enable_mask = PF1550_RAIL_EN,
	.enable_val = PF1550_RAIL_EN_MASK,
	.cfg_reg = PMIC_LDO3_CTRL,
	.uv_range = ldo13_range,
	.uv_nranges = ARRAY_SIZE(ldo13_range),
};

static int regulator_pf1550_set_enable(const struct device *dev, bool enable)
{
	const struct regulator_pf1550_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->bus, config->desc->cfg_reg,
				      config->desc->enable_mask,
				      enable ? config->desc->enable_val : 0);
}

static int regulator_pf1550_enable(const struct device *dev)
{
	return regulator_pf1550_set_enable(dev, true);
}

static int regulator_pf1550_disable(const struct device *dev)
{
	return regulator_pf1550_set_enable(dev, false);
}

static unsigned int regulator_pf1550_count_voltages(const struct device *dev)
{
	const struct regulator_pf1550_config *config = dev->config;

	return linear_range_group_values_count(config->desc->uv_range, config->desc->uv_nranges);
}

static int regulator_pf1550_list_voltage(const struct device *dev, unsigned int idx,
					 int32_t *volt_uv)
{
	const struct regulator_pf1550_config *config = dev->config;

	return linear_range_group_get_value(config->desc->uv_range, config->desc->uv_nranges, idx,
					    volt_uv);
}

static int regulator_pf1550_set_buck_ldo_voltage(const struct device *dev, int32_t min_uv,
						 int32_t max_uv, const struct linear_range *range,
						 const uint8_t nranges, uint8_t vout_reg)
{
	const struct regulator_pf1550_config *config = dev->config;
	uint16_t idx;
	int ret;

	ret = linear_range_group_get_win_index(range, nranges, min_uv, max_uv, &idx);
	if (ret < 0) {
		return ret;
	}

	return i2c_reg_write_byte_dt(&config->bus, vout_reg, (uint8_t)idx);
}

static int regulator_pf1550_buck12_ldo123_get_voltage(const struct device *dev,
						      const struct linear_range *range,
						      const uint8_t nranges, uint8_t vout_reg,
						      int32_t *volt_uv)
{
	const struct regulator_pf1550_config *config = dev->config;
	uint8_t idx;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->bus, vout_reg, &idx);
	if (ret < 0) {
		return ret;
	}

	return linear_range_group_get_value(range, nranges, idx, volt_uv);
}

static int regulator_pf1550_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_pf1550_config *config = dev->config;

	return regulator_pf1550_buck12_ldo123_get_voltage(dev, config->desc->uv_range,
							  config->desc->uv_nranges,
							  config->desc->vsel_reg, volt_uv);
}

static int regulator_pf1550_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_pf1550_config *config = dev->config;

	return regulator_pf1550_set_buck_ldo_voltage(dev, min_uv, max_uv, config->desc->uv_range,
						     config->desc->uv_nranges,
						     config->desc->vsel_reg);
}

static unsigned int regulator_pf1550_count_current_limits(const struct device *dev)
{
	const struct regulator_pf1550_config *config = dev->config;

	if (config->source != PF1550_PMIC_SOURCE_BUCK1 &&
	    config->source != PF1550_PMIC_SOURCE_BUCK2 &&
	    config->source != PF1550_PMIC_SOURCE_BUCK3) {
		return -ENOTSUP;
	}

	return linear_range_group_values_count(config->desc->ua_range, config->desc->ua_nranges);
}

static int regulator_pf1550_list_current_limit(const struct device *dev, unsigned int idx,
					       int32_t *current_ua)
{
	const struct regulator_pf1550_config *config = dev->config;

	if (config->source != PF1550_PMIC_SOURCE_BUCK1 &&
	    config->source != PF1550_PMIC_SOURCE_BUCK2 &&
	    config->source != PF1550_PMIC_SOURCE_BUCK3) {
		return -ENOTSUP;
	}

	return linear_range_group_get_value(config->desc->ua_range, config->desc->ua_nranges, idx,
					    current_ua);
}

static int regulator_pf1550_set_current_limit(const struct device *dev, int32_t min_ua,
					      int32_t max_ua)
{
	const struct regulator_pf1550_config *config = dev->config;
	uint8_t val;
	uint16_t idx;
	int ret;

	if (config->source != PF1550_PMIC_SOURCE_BUCK1 &&
	    config->source != PF1550_PMIC_SOURCE_BUCK2 &&
	    config->source != PF1550_PMIC_SOURCE_BUCK3) {
		return -ENOTSUP;
	}

	/* Current is stored in SW*_CTRL1 register */
	ret = i2c_reg_read_byte_dt(&config->bus, config->desc->cfg_reg + 1, &val);
	if (ret < 0) {
		return ret;
	}

	ret = linear_range_group_get_win_index(config->desc->ua_range, config->desc->ua_nranges,
					       min_ua, max_ua, &idx);
	if (ret < 0) {
		return ret;
	}

	val |= idx;
	return i2c_reg_write_byte_dt(&config->bus, config->desc->cfg_reg + 1, val);
}

static int regulator_pf1550_power_off(const struct device *dev)
{
	const struct regulator_pf1550_common_config *common_config = dev->config;

	return i2c_reg_update_byte_dt(&common_config->bus, PMIC_PWRCTRL3, PF1550_GOTO_SHIP_MASK,
				      PF1550_GOTO_SHIP);
}

static int regulator_pf1550_init(const struct device *dev)
{
	const struct regulator_pf1550_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->bus)) {
		return -ENODEV;
	}

	regulator_common_data_init(dev);

	return regulator_common_init(dev, false);
}

static int regulator_pf1550_common_init(const struct device *dev)
{
	const struct regulator_pf1550_common_config *common_config = dev->config;

	if (!i2c_is_ready_dt(&common_config->bus)) {
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(regulator_parent, parent_api) = {
	.ship_mode = regulator_pf1550_power_off,
};

static DEVICE_API(regulator, api) = {
	.enable = regulator_pf1550_enable,
	.disable = regulator_pf1550_disable,
	.count_voltages = regulator_pf1550_count_voltages,
	.list_voltage = regulator_pf1550_list_voltage,
	.set_voltage = regulator_pf1550_set_voltage,
	.get_voltage = regulator_pf1550_get_voltage,
	.count_current_limits = regulator_pf1550_count_current_limits,
	.list_current_limit = regulator_pf1550_list_current_limit,
	.set_current_limit = regulator_pf1550_set_current_limit,
};

#define REGULATOR_PF1550_DEFINE(node_id, id, child_name, _source)                                  \
	static const struct regulator_pf1550_config regulator_pf1550_config_##id = {               \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.bus = I2C_DT_SPEC_GET(DT_GPARENT(node_id)),                                       \
		.desc = &child_name##_desc,                                                        \
		.source = _source,                                                                 \
	};                                                                                         \
												   \
	static struct regulator_pf1550_data regulator_pf1550_data_##id;                            \
	DEVICE_DT_DEFINE(node_id, regulator_pf1550_init, NULL, &regulator_pf1550_data_##id,        \
			 &regulator_pf1550_config_##id, POST_KERNEL,                               \
			 CONFIG_MFD_INIT_PRIORITY, &api);

#define REGULATOR_PF1550_DEFINE_COND(inst, child, source)                                          \
	COND_CODE_1(                                                                               \
		DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                                        \
		(REGULATOR_PF1550_DEFINE(DT_INST_CHILD(inst, child), child##inst, child, source)), \
		())

#define REGULATOR_PF1550_DEFINE_ALL(inst)                                                          \
	static const struct regulator_pf1550_common_config common_config_##inst = {                \
		.bus = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                      \
	};                                                                                         \
												   \
	DEVICE_DT_INST_DEFINE(inst, regulator_pf1550_common_init, NULL, NULL,                      \
			      &common_config_##inst, POST_KERNEL,                                  \
			      CONFIG_MFD_INIT_PRIORITY, &parent_api);                              \
												   \
	REGULATOR_PF1550_DEFINE_COND(inst, buck1, PF1550_PMIC_SOURCE_BUCK1)                        \
	REGULATOR_PF1550_DEFINE_COND(inst, buck2, PF1550_PMIC_SOURCE_BUCK2)                        \
	REGULATOR_PF1550_DEFINE_COND(inst, buck3, PF1550_PMIC_SOURCE_BUCK3)                        \
	REGULATOR_PF1550_DEFINE_COND(inst, ldo1, PF1550_PMIC_SOURCE_LDO1)                          \
	REGULATOR_PF1550_DEFINE_COND(inst, ldo2, PF1550_PMIC_SOURCE_LDO2)                          \
	REGULATOR_PF1550_DEFINE_COND(inst, ldo3, PF1550_PMIC_SOURCE_LDO3)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_PF1550_DEFINE_ALL)
