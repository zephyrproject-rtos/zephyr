/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT adi_max20362_regulator

#include <zephyr/logging/log.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/regulator/max20362.h>
#include <zephyr/dt-bindings/regulator/max20362.h>

LOG_MODULE_REGISTER(regulator_max20362, CONFIG_REGULATOR_LOG_LEVEL);

#define MAX20362_CHIP_ID_VAL 0x00

/* Register addresses */
#define MAX20362_REG_CHIP_ID        0x00
#define MAX20362_REG_BBST_CFG       0x01
#define MAX20362_REG_BBST_VST       0x02
#define MAX20362_REG_CAP_VST        0x0A
#define MAX20362_REG_IGN_CFG        0x0C
#define MAX20362_REG_STATUS         0x10
#define MAX20362_REG_INT            0x14
#define MAX20362_REG_INGEN_INT      0x16
#define MAX20362_REG_LDO_INT        0x17
#define MAX20362_REG_INT_MASK       0x18
#define MAX20362_REG_INGEN_INT_MASK 0x1A
#define MAX20362_REG_LDO_INT_MASK   0x1B
#define MAX20362_REG_LDO_CFG        0x40
#define MAX20362_REG_LDO_VST        0x41
#define MAX20362_REG_WRTE_LCK       0x50
#define MAX20362_REG_BBST_LCK       0x51
#define MAX20362_REG_DVS_CFG        0x54

/* Register bit masks */
#define MAX20362_BBST_VSET_MASK  GENMASK(6, 0)
#define MAX20362_CAP_VSET_MASK   GENMASK(3, 0)
#define MAX20362_LDO_VSET_MASK   GENMASK(4, 0)
#define MAX20362_CAP_STEP_MASK   GENMASK(5, 4)
#define MAX20362_CAP_CSET_MASK   GENMASK(5, 0)
#define MAX20362_BB_EN_MASK      BIT(5)
#define MAX20362_LDO_EN_MASK     BIT(0)
#define MAX20362_LDO_DSCRGE_MASK BIT(2)
#define MAX20362_BB_DSCRGE_MASK  BIT(1)
#define MAX20362_CAP_DSCRGE_MASK BIT(7)
#define MAX20362_BBLDO_MASK      BIT(5)
#define MAX20362_DVS_MASK        GENMASK(1, 0)
#define MAX20362_BBVDROP_MASK    GENMASK(7, 6)

/* Lock/unlock values */
#define MAX20362_LOCK_BB      0xAA
#define MAX20362_UNLOCK_BB    0x55
#define MAX20362_MASK_WRITE   0x01
#define MAX20362_UNMASK_WRITE 0x00

/* BBLDO field values: whether the LDO waits for the buck-boost to be on before enabling */
#define MAX20362_LDO_WAIT_FOR_BB    0x01
#define MAX20362_LDO_NO_WAIT_FOR_BB 0x00

/* Settle time after switching the DVS interface source (microseconds) */
#define MAX20362_DVS_SETTLE_TIME_US 300

const struct linear_range cap_current_range = LINEAR_RANGE_INIT(5000, 1000, 0x00, 0x2D);

const struct linear_range bbout_range[] = {
	LINEAR_RANGE_INIT(1500000, 50000, 0x00, 0x50),
};

const struct linear_range ldo_range[] = {
	LINEAR_RANGE_INIT(900000, 100000, 0x00, 0x1F),
};

const struct linear_range cap_ranges[] = {
	LINEAR_RANGE_INIT(2500000, 500000, 0x00, 0x0E),
	LINEAR_RANGE_INIT(1600000, 250000, 0x00, 0x0F),
	LINEAR_RANGE_INIT(1650000, 125000, 0x04, 0x0F),
};

enum max20362_pmic_sources {
	MAX20362_PMIC_SOURCE_BBOOST,
	MAX20362_PMIC_SOURCE_CAP,
	MAX20362_PMIC_SOURCE_LDO,
};

struct regulator_max20362_desc {
	uint8_t vset_mask;
	uint8_t vsel_reg;
	uint8_t enable_mask;
	uint8_t cfg_reg;
	uint8_t act_dscrge_mask;
	uint8_t discharge_reg;
	uint8_t cset_mask;
	uint8_t csel_reg;
	uint8_t uv_range_size;
	const struct linear_range *uv_range;
	const struct linear_range *ua_range;
};

struct regulator_max20362_common_config {
	struct i2c_dt_spec bus;
	uint8_t bbat_vdrop;
	uint8_t dvs_source;
	uint8_t ldo_source;
};

struct regulator_max20362_config {
	struct regulator_common_config common;
	struct i2c_dt_spec bus;
	const struct regulator_max20362_desc *desc;
	uint8_t source;
};

struct regulator_max20362_data {
	struct regulator_common_data common;
};

static const struct regulator_max20362_desc __maybe_unused bboost_desc = {
	.vset_mask = MAX20362_BBST_VSET_MASK,
	.vsel_reg = MAX20362_REG_BBST_VST,
	.enable_mask = MAX20362_BB_EN_MASK,
	.cfg_reg = MAX20362_REG_BBST_CFG,
	.act_dscrge_mask = MAX20362_BB_DSCRGE_MASK,
	.discharge_reg = MAX20362_REG_BBST_CFG,
	.uv_range = bbout_range,
	.uv_range_size = ARRAY_SIZE(bbout_range),
};

static const struct regulator_max20362_desc __maybe_unused cap_desc = {
	.vset_mask = MAX20362_CAP_VSET_MASK,
	.vsel_reg = MAX20362_REG_CAP_VST,
	.act_dscrge_mask = MAX20362_CAP_DSCRGE_MASK,
	.discharge_reg = MAX20362_REG_CAP_VST,
	.cset_mask = MAX20362_CAP_CSET_MASK,
	.csel_reg = MAX20362_REG_IGN_CFG,
	.uv_range = cap_ranges,
	.ua_range = &cap_current_range,
	.uv_range_size = ARRAY_SIZE(cap_ranges),
};

static const struct regulator_max20362_desc __maybe_unused ldo_desc = {
	.vset_mask = MAX20362_LDO_VSET_MASK,
	.vsel_reg = MAX20362_REG_LDO_VST,
	.enable_mask = MAX20362_LDO_EN_MASK,
	.cfg_reg = MAX20362_REG_LDO_CFG,
	.act_dscrge_mask = MAX20362_LDO_DSCRGE_MASK,
	.discharge_reg = MAX20362_REG_LDO_CFG,
	.uv_range = ldo_range,
	.uv_range_size = ARRAY_SIZE(ldo_range),
};

static inline int regulator_max20362_reg_read(const struct i2c_dt_spec *bus, uint8_t reg,
					      uint8_t *data)
{
	return i2c_reg_read_byte_dt(bus, reg, data);
}

static inline int regulator_max20362_reg_write(const struct i2c_dt_spec *bus, uint8_t reg,
					       uint8_t data)
{
	return i2c_reg_write_byte_dt(bus, reg, data);
}

static inline int regulator_max20362_reg_update(const struct i2c_dt_spec *bus, uint8_t addr,
						uint8_t mask, uint8_t value)
{
	return i2c_reg_update_byte_dt(bus, addr, mask, FIELD_PREP(mask, value));
}

static int regulator_max20362_set_lock(const struct device *dev, bool lock)
{
	const struct regulator_max20362_config *config = dev->config;
	int ret;

	if (config->source != MAX20362_PMIC_SOURCE_BBOOST) {
		LOG_ERR("Regulator source %d does not support set_lock.", config->source);
		return -ENOTSUP;
	}

	ret = regulator_max20362_reg_write(&config->bus, MAX20362_REG_WRTE_LCK,
					   MAX20362_UNMASK_WRITE);
	if (ret < 0) {
		LOG_ERR("Failed to write lock register.");
		return ret;
	}

	ret = regulator_max20362_reg_write(&config->bus, MAX20362_REG_BBST_LCK,
					   lock ? MAX20362_LOCK_BB : MAX20362_UNLOCK_BB);
	if (ret < 0) {
		LOG_ERR("Failed to write buck-boost lock register.");
		return ret;
	}

	if (lock) {
		ret = regulator_max20362_reg_write(&config->bus, MAX20362_REG_WRTE_LCK,
						   MAX20362_MASK_WRITE);
		if (ret < 0) {
			LOG_ERR("Failed to re-mask buck-boost lock register.");
			return ret;
		}
	}

	return 0;
}

static int regulator_max20362_set_enable(const struct device *dev, bool enable)
{
	const struct regulator_max20362_config *config = dev->config;

	if (config->source == MAX20362_PMIC_SOURCE_CAP) {
		LOG_ERR("Regulator source %d does not support set_enable.", config->source);
		return -ENOTSUP;
	}

	return regulator_max20362_reg_update(&config->bus, config->desc->cfg_reg,
					     config->desc->enable_mask, enable);
}

static int regulator_max20362_get_ldo_enable_status(const struct device *dev, bool *enabled)
{
	const struct regulator_max20362_config *config = dev->config;
	uint8_t val;
	int ret;

	if (config->source == MAX20362_PMIC_SOURCE_CAP) {
		LOG_ERR("Regulator source %d does not support get_ldo_enable_status.",
			config->source);
		return -ENOTSUP;
	}

	ret = regulator_max20362_reg_read(&config->bus, config->desc->cfg_reg, &val);
	if (ret < 0) {
		LOG_ERR("Failed to read enable register.");
		return ret;
	}

	*enabled = (FIELD_GET(config->desc->enable_mask, val) != 0);

	return 0;
}

static inline int regulator_max20362_enable(const struct device *dev)
{
	return regulator_max20362_set_enable(dev, true);
}

static inline int regulator_max20362_disable(const struct device *dev)
{
	return regulator_max20362_set_enable(dev, false);
}

static unsigned int regulator_max20362_count_voltages(const struct device *dev)
{
	const struct regulator_max20362_config *config = dev->config;

	return linear_range_group_values_count(config->desc->uv_range, config->desc->uv_range_size);
}

static int regulator_max20362_list_voltage(const struct device *dev, unsigned int idx,
					   int32_t *volt_uv)
{
	const struct regulator_max20362_config *config = dev->config;

	return linear_range_group_get_value(config->desc->uv_range, config->desc->uv_range_size,
					    idx, volt_uv);
}

static int regulator_max20362_set_rail_voltage(const struct device *dev, int32_t min_uv,
					       int32_t max_uv, const struct linear_range *range,
					       uint8_t range_size)
{
	const struct regulator_max20362_config *config = dev->config;
	uint16_t idx;
	int ret = 0;

	for (int i = 0; i < range_size; i++) {
		ret = linear_range_get_win_index(&range[i], min_uv, max_uv, &idx);
		if (ret < 0) {
			continue;
		}

		if (config->source == MAX20362_PMIC_SOURCE_CAP) {
			ret = regulator_max20362_reg_update(&config->bus, MAX20362_REG_CAP_VST,
							    MAX20362_CAP_STEP_MASK, i);
			if (ret < 0) {
				LOG_ERR("Failed to update supported CAP voltage range.");
				return ret;
			}
		}
		break;
	}
	if (ret < 0) {
		LOG_ERR("Invalid voltage range: min_uv=%d, max_uv=%d.", min_uv, max_uv);
		return ret;
	}

	return regulator_max20362_reg_update(&config->bus, config->desc->vsel_reg,
					     config->desc->vset_mask, idx);
}

static int regulator_max20362_get_rail_voltage(const struct device *dev,
					       const struct linear_range *range, uint8_t range_size,
					       int32_t *volt_uv)
{
	const struct regulator_max20362_config *config = dev->config;
	uint8_t vsel_reg_value = 0;
	uint8_t cap_sel = 0;
	uint8_t idx;
	int ret;

	ret = regulator_max20362_reg_read(&config->bus, config->desc->vsel_reg, &vsel_reg_value);
	if (ret < 0) {
		LOG_ERR("Failed to read voltage register.");
		return ret;
	}

	idx = FIELD_GET(config->desc->vset_mask, vsel_reg_value);

	if (config->source == MAX20362_PMIC_SOURCE_CAP) {
		cap_sel = FIELD_GET(MAX20362_CAP_STEP_MASK, vsel_reg_value);

		if (cap_sel >= config->desc->uv_range_size) {
			LOG_ERR("Invalid/reserved CAP step selection: %u", cap_sel);
			return -EINVAL;
		}

		return linear_range_get_value(&range[cap_sel], idx, volt_uv);
	}

	return linear_range_group_get_value(range, range_size, idx, volt_uv);
}

static int regulator_max20362_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_max20362_config *config = dev->config;

	return regulator_max20362_get_rail_voltage(dev, config->desc->uv_range,
						   config->desc->uv_range_size, volt_uv);
}

static int regulator_max20362_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_max20362_config *config = dev->config;
	bool to_enable = false;
	int ret;

	switch (config->source) {
	case MAX20362_PMIC_SOURCE_BBOOST:
		ret = regulator_max20362_set_lock(dev, false);
		if (ret < 0) {
			LOG_ERR("Failed to unlock write mask.");
			return ret;
		}
		break;
	case MAX20362_PMIC_SOURCE_LDO:
		ret = regulator_max20362_get_ldo_enable_status(dev, &to_enable);
		if (ret < 0) {
			LOG_ERR("Failed to read LDO enable state.");
			return ret;
		}
		if (to_enable) {
			ret = regulator_max20362_set_enable(dev, false);
			if (ret < 0) {
				LOG_ERR("Failed to disable LDO regulator.");
				return ret;
			}
		}
		break;
	default:
		break;
	}

	ret = regulator_max20362_set_rail_voltage(dev, min_uv, max_uv, config->desc->uv_range,
						  config->desc->uv_range_size);
	if (ret < 0) {
		LOG_ERR("Failed to set regulator rail voltage.");
		return ret;
	}

	switch (config->source) {
	case MAX20362_PMIC_SOURCE_BBOOST:
		ret = regulator_max20362_set_lock(dev, true);
		if (ret < 0) {
			LOG_ERR("Failed to re-lock write mask.");
			return ret;
		}
		break;
	case MAX20362_PMIC_SOURCE_LDO:
		if (to_enable) {
			ret = regulator_max20362_set_enable(dev, true);
			if (ret < 0) {
				LOG_ERR("Failed to enable LDO regulator.");
				return ret;
			}
		}
		break;
	default:
		break;
	}

	return 0;
}

static unsigned int regulator_max20362_count_current_limits(const struct device *dev)
{
	const struct regulator_max20362_config *config = dev->config;

	if (config->source != MAX20362_PMIC_SOURCE_CAP) {
		LOG_ERR("Regulator source %d does not support count_current_limits.",
			config->source);
		return -ENOTSUP;
	}

	return linear_range_values_count(config->desc->ua_range);
}

static int regulator_max20362_list_current_limit(const struct device *dev, unsigned int idx,
						 int32_t *current_ua)
{
	const struct regulator_max20362_config *config = dev->config;

	if (config->source != MAX20362_PMIC_SOURCE_CAP) {
		LOG_ERR("Regulator source %d does not support list_current_limit.", config->source);
		return -ENOTSUP;
	}

	return linear_range_get_value(config->desc->ua_range, idx, current_ua);
}

static int regulator_max20362_set_current_limit(const struct device *dev, int32_t min_ua,
						int32_t max_ua)
{
	const struct regulator_max20362_config *config = dev->config;
	uint16_t idx;
	int ret;

	if (config->source != MAX20362_PMIC_SOURCE_CAP) {
		LOG_ERR("Regulator source %d does not support set_current_limit.", config->source);
		return -ENOTSUP;
	}

	ret = linear_range_get_win_index(config->desc->ua_range, min_ua, max_ua, &idx);
	if (ret < 0) {
		LOG_ERR("Invalid current range: min_ua=%d, max_ua=%d.", min_ua, max_ua);
		return ret;
	}

	return regulator_max20362_reg_update(&config->bus, config->desc->csel_reg,
					     config->desc->cset_mask, idx);
}

static int regulator_max20362_set_active_discharge(const struct device *dev, bool active_discharge)
{
	const struct regulator_max20362_config *config = dev->config;

	return regulator_max20362_reg_update(&config->bus, config->desc->discharge_reg,
					     config->desc->act_dscrge_mask, active_discharge);
}

static int regulator_max20362_get_active_discharge(const struct device *dev, bool *active_discharge)
{
	const struct regulator_max20362_config *config = dev->config;
	uint8_t val;
	int ret;

	ret = regulator_max20362_reg_read(&config->bus, config->desc->discharge_reg, &val);
	if (ret < 0) {
		LOG_ERR("Failed to read active discharge register.");
		return ret;
	}

	*active_discharge = FIELD_GET(config->desc->act_dscrge_mask, val);

	return 0;
}

/* Interrupt handling functions */

int regulator_max20362_set_int_mask(const struct device *dev, uint8_t mask)
{
	const struct regulator_max20362_common_config *config = dev->config;

	return regulator_max20362_reg_write(&config->bus, MAX20362_REG_INT_MASK, mask);
}

int regulator_max20362_set_ingen_int_mask(const struct device *dev, uint8_t mask)
{
	const struct regulator_max20362_common_config *config = dev->config;

	return regulator_max20362_reg_write(&config->bus, MAX20362_REG_INGEN_INT_MASK, mask);
}

int regulator_max20362_set_ldo_int_mask(const struct device *dev, uint8_t mask)
{
	const struct regulator_max20362_common_config *config = dev->config;

	return regulator_max20362_reg_write(&config->bus, MAX20362_REG_LDO_INT_MASK, mask);
}

static int regulator_max20362_set_bat_bbin_vdrop(const struct device *dev, uint8_t vdrop)
{
	const struct regulator_max20362_common_config *config = dev->config;

	return regulator_max20362_reg_update(&config->bus, MAX20362_REG_IGN_CFG,
					     MAX20362_BBVDROP_MASK, vdrop);
}

static int regulator_max20362_set_dvs_interface_source(const struct device *dev, uint8_t source)
{
	const struct regulator_max20362_common_config *config = dev->config;
	int ret;

	ret = regulator_max20362_reg_update(&config->bus, MAX20362_REG_DVS_CFG, MAX20362_DVS_MASK,
					    source);
	if (ret < 0) {
		return ret;
	}
	k_usleep(MAX20362_DVS_SETTLE_TIME_US);

	return 0;
}

static int regulator_max20362_set_ldo_input_source(const struct device *dev, uint8_t source)
{
	const struct regulator_max20362_common_config *config = dev->config;

	if (source == MAX20362_LDO_SRC_BBOUT) {
		return regulator_max20362_reg_update(&config->bus, MAX20362_REG_LDO_CFG,
						     MAX20362_BBLDO_MASK, MAX20362_LDO_WAIT_FOR_BB);
	} else {
		return regulator_max20362_reg_update(&config->bus, MAX20362_REG_LDO_CFG,
						     MAX20362_BBLDO_MASK,
						     MAX20362_LDO_NO_WAIT_FOR_BB);
	}
}

static int regulator_max20362_init(const struct device *dev)
{
	const struct regulator_max20362_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR_DEVICE_NOT_READY(config->bus.bus);
		return -ENODEV;
	}

	regulator_common_data_init(dev);

	return regulator_common_init(dev, false);
}

static int regulator_max20362_common_init(const struct device *dev)
{
	const struct regulator_max20362_common_config *common_config = dev->config;
	uint8_t val;
	int ret;

	if (!i2c_is_ready_dt(&common_config->bus)) {
		LOG_ERR_DEVICE_NOT_READY(common_config->bus.bus);
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&common_config->bus, MAX20362_REG_CHIP_ID, &val);
	if (ret < 0) {
		LOG_ERR("Failed to read CHIP ID register.");
		return ret;
	}

	if (val != MAX20362_CHIP_ID_VAL) {
		LOG_ERR("Mismatched CHIP ID register value returned.");
		return -ENODEV;
	}

	ret = regulator_max20362_set_bat_bbin_vdrop(dev, common_config->bbat_vdrop);
	if (ret < 0) {
		LOG_ERR("Failed to set voltage drop from BAT to BBIN");
		return ret;
	}

	ret = regulator_max20362_set_dvs_interface_source(dev, common_config->dvs_source);
	if (ret < 0) {
		LOG_ERR("Failed to set DVS source");
		return ret;
	}

	ret = regulator_max20362_set_ldo_input_source(dev, common_config->ldo_source);
	if (ret < 0) {
		LOG_ERR("Failed to set LDO input source");
		return ret;
	}

	return 0;
}

static DEVICE_API(regulator, api) = {
	.enable = regulator_max20362_enable,
	.disable = regulator_max20362_disable,
	.count_voltages = regulator_max20362_count_voltages,
	.list_voltage = regulator_max20362_list_voltage,
	.set_voltage = regulator_max20362_set_voltage,
	.get_voltage = regulator_max20362_get_voltage,
	.count_current_limits = regulator_max20362_count_current_limits,
	.list_current_limit = regulator_max20362_list_current_limit,
	.set_current_limit = regulator_max20362_set_current_limit,
	.set_active_discharge = regulator_max20362_set_active_discharge,
	.get_active_discharge = regulator_max20362_get_active_discharge,
};

#define REGULATOR_MAX20362_DEFINE(node_id, id, child_name, _source)                                \
	static const struct regulator_max20362_config regulator_max20362_config_##id = {           \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.bus = I2C_DT_SPEC_GET(DT_PARENT(node_id)),                                        \
		.desc = &child_name##_desc,                                                        \
		.source = _source,                                                                 \
	};                                                                                         \
                                                                                                   \
	static struct regulator_max20362_data regulator_max20362_data_##id;                        \
	DEVICE_DT_DEFINE(node_id, regulator_max20362_init, NULL, &regulator_max20362_data_##id,    \
			 &regulator_max20362_config_##id, POST_KERNEL,                             \
			 CONFIG_REGULATOR_ADI_MAX20362_INIT_PRIORITY, &api);

#define REGULATOR_MAX20362_DEFINE_COND(inst, child, source)                                        \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                                    \
		    (REGULATOR_MAX20362_DEFINE(DT_INST_CHILD(inst, child),                         \
					       child##inst, child, source)),                       \
		    ())

#define REGULATOR_MAX20362_DEFINE_ALL(inst)                                                        \
	static const struct regulator_max20362_common_config common_config_##inst = {              \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.bbat_vdrop = DT_INST_PROP(inst, bat_bbin_vdrop),                                  \
		.dvs_source = DT_INST_PROP(inst, dvs_src),                                         \
		.ldo_source = DT_INST_PROP(inst, ldo_src),                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, regulator_max20362_common_init, NULL, NULL,                    \
			      &common_config_##inst, POST_KERNEL,                                  \
			      CONFIG_REGULATOR_ADI_MAX20362_COMMON_INIT_PRIORITY, NULL);           \
                                                                                                   \
	REGULATOR_MAX20362_DEFINE_COND(inst, bboost, MAX20362_PMIC_SOURCE_BBOOST)                  \
	REGULATOR_MAX20362_DEFINE_COND(inst, cap, MAX20362_PMIC_SOURCE_CAP)                        \
	REGULATOR_MAX20362_DEFINE_COND(inst, ldo, MAX20362_PMIC_SOURCE_LDO)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_MAX20362_DEFINE_ALL)
