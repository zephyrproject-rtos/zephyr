/*
 * Copyright (c) 2021 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pca9420

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/regulator/pca9420.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>

/** Register memory map. See datasheet for more details. */
/** General purpose registers */
/** @brief Top level system ctrl 0 */
#define PCA9420_TOP_CNTL0     0x09U
/** @brief Top level system ctrl 2 */
#define PCA9420_TOP_CNTL2     0x0BU
/** @brief Top level system ctrl 3 */
#define PCA9420_TOP_CNTL3     0x0CU

/** Regulator status indication registers */
/** @brief Active Discharge configuration for mode 0_0 */
#define PCA9420_ACT_DISCHARGE_CNTL   0x21U
/** @brief Mode configuration for mode 0_0 */
#define PCA9420_MODECFG_0_0          0x22U
/** @brief Mode configuration for mode 0_1 */
#define PCA9420_MODECFG_0_1          0x23U
/** @brief Mode configuration for mode 0_2 */
#define PCA9420_MODECFG_0_2          0x24U
/** @brief Mode configuration for mode 0_3 */
#define PCA9420_MODECFG_0_3          0x25U

/** @brief VIN input current limit selection */
#define PCA9420_TOP_CNTL0_VIN_ILIM_SEL_POS 5U
#define PCA9420_TOP_CNTL0_VIN_ILIM_SEL_MASK 0xE0U
#define PCA9420_TOP_CNTL0_VIN_ILIM_SEL_DISABLED 0x7U

/** @brief ASYS UVLO threshold selection */
#define PCA9420_TOP_CNTL2_ASYS_UVLO_SEL_POS  6U
#define PCA9420_TOP_CNTL2_ASYS_UVLO_SEL_MASK 0xC0U

/** @brief I2C Mode control mask */
#define PCA9420_TOP_CNTL3_MODE_I2C_POS 3U
#define PCA9420_TOP_CNTL3_MODE_I2C_MASK 0x18U

/*
 * @brief Mode control selection mask. When this bit is set, the external
 * PMIC pins MODESEL0 and MODESEL1 can be used to select the active mode
 */
#define PCA9420_MODECFG_0_X_EN_MODE_SEL_BY_PIN 0x40U

/*
 * @brief Mode configuration upon falling edge applied to ON pin. If set,
 * the device will switch to mode 0 when a valid falling edge is applied.
 * to the ON pin
 */
/** @brief Mode output voltage mask */
#define PCA9420_MODECFG_0_SW1_OUT_MASK       0x3FU
#define PCA9420_MODECFG_0_SW1_OUT_POS        0U
/** @brief SW2_OUT offset and voltage level mask */
#define PCA9420_MODECFG_1_SW2_OUT_MASK       0x3FU
#define PCA9420_MODECFG_1_SW2_OUT_POS        0U
/** @brief LDO1_OUT voltage level mask */
#define PCA9420_MODECFG_2_LDO1_OUT_MASK      0xF0U
#define PCA9420_MODECFG_2_LDO1_OUT_POS       4U
/** @brief SW1 Enable */
#define PCA9420_MODECFG_2_SW1_EN_MASK	     0x08U
#define PCA9420_MODECFG_2_SW1_EN_VAL	     0x08U
/** @brief SW2 Enable */
#define PCA9420_MODECFG_2_SW2_EN_MASK	     0x04U
#define PCA9420_MODECFG_2_SW2_EN_VAL	     0x04U
/** @brief LDO1 Enable */
#define PCA9420_MODECFG_2_LDO1_EN_MASK	     0x02U
#define PCA9420_MODECFG_2_LDO1_EN_VAL	     0x02U
/** @brief LDO2 Enable */
#define PCA9420_MODECFG_2_LDO2_EN_MASK	     0x01U
#define PCA9420_MODECFG_2_LDO2_EN_VAL	     0x01U
/** @brief LDO2_OUT offset and voltage level mask */
#define PCA9420_MODECFG_3_LDO2_OUT_MASK      0x3FU
#define PCA9420_MODECFG_3_LDO2_OUT_POS       0U
/** @brief SW1 active discharge control */
#define PCA9420_ACT_DISCHARGE_CNTL_SW1_MASK  0x08U
#define PCA9420_ACT_DISCHARGE_CNTL_SW1_POS   4U
/** @brief SW2 active discharge control */
#define PCA9420_ACT_DISCHARGE_CNTL_SW2_MASK  0x04U
#define PCA9420_ACT_DISCHARGE_CNTL_SW2_POS   3U
/** @brief LDO1 active discharge control */
#define PCA9420_ACT_DISCHARGE_CNTL_LDO1_MASK 0x02U
#define PCA9420_ACT_DISCHARGE_CNTL_LDO1_POS  2U
/** @brief LDO2 active discharge control */
#define PCA9420_ACT_DISCHARGE_CNTL_LDO2_MASK 0x01U
#define PCA9420_ACT_DISCHARGE_CNTL_LDO2_POS  1U

/** VIN ILIM resolution, uA/LSB */
#define PCA9420_VIN_ILIM_UA_LSB 170000
/** VIN ILIM minimum value, uA */
#define PCA9420_VIN_ILIM_MIN_UA 85000

/** Number of modes */
#define PCA9420_NUM_MODES 4U
/** Offset applied to MODECFG* registers for a given mode */
#define PCA9420_MODECFG_OFFSET(mode) ((mode) * 4U)

struct regulator_pca9420_desc {
	uint8_t enable_reg;
	uint8_t enable_mask;
	uint8_t enable_val;
	uint8_t vsel_reg;
	uint8_t vsel_mask;
	uint8_t vsel_pos;
	uint8_t ad_mask;
	uint8_t ad_pos;
	int32_t max_ua;
	uint8_t num_ranges;
	const struct linear_range *ranges;
};

struct regulator_pca9420_common_config {
	struct i2c_dt_spec i2c;
	int32_t vin_ilim_ua;
	bool enable_modesel_pins;
	uint8_t asys_uvlo_sel_mv;
};

struct regulator_pca9420_common_data {
	regulator_dvs_state_t dvs_state;
};

struct regulator_pca9420_config {
	struct regulator_common_config common;
	bool enable_inverted;
	int32_t modes_uv[4];
	const struct regulator_pca9420_desc *desc;
	const struct device *parent;
};

struct regulator_pca9420_data {
	struct regulator_common_data data;
};

static const struct linear_range buck1_ranges[] = {
	LINEAR_RANGE_INIT(500000, 25000U, 0x0U, 0x28U),
	LINEAR_RANGE_INIT(1500000, 0U, 0x29U, 0x3E),
	LINEAR_RANGE_INIT(1800000, 0U, 0x3FU, 0x3FU),
};

static const struct linear_range buck2_ranges[] = {
	LINEAR_RANGE_INIT(1500000, 25000U, 0x0U, 0x18U),
	LINEAR_RANGE_INIT(2100000, 0U, 0x19U, 0x1F),
	LINEAR_RANGE_INIT(2700000, 25000U, 0x20U, 0x38U),
	LINEAR_RANGE_INIT(3300000, 0U, 0x39U, 0x3F),
};

static const struct linear_range ldo1_ranges[] = {
	LINEAR_RANGE_INIT(1700000, 25000U, 0x0U, 0x9U),
	LINEAR_RANGE_INIT(1900000, 0U, 0x9U, 0xFU),
};

static const struct linear_range ldo2_ranges[] = {
	LINEAR_RANGE_INIT(1500000, 25000U, 0x0U, 0x18U),
	LINEAR_RANGE_INIT(2100000, 0U, 0x19U, 0x1FU),
	LINEAR_RANGE_INIT(2700000, 25000U, 0x20U, 0x38U),
	LINEAR_RANGE_INIT(3300000, 0U, 0x39U, 0x3FU),
};

static const struct regulator_pca9420_desc buck1_desc = {
	.enable_reg = PCA9420_MODECFG_0_2,
	.enable_mask = PCA9420_MODECFG_2_SW1_EN_MASK,
	.enable_val = PCA9420_MODECFG_2_SW1_EN_VAL,
	.vsel_mask = PCA9420_MODECFG_0_SW1_OUT_MASK,
	.vsel_pos = PCA9420_MODECFG_0_SW1_OUT_POS,
	.vsel_reg = PCA9420_MODECFG_0_0,
	.ad_mask = PCA9420_ACT_DISCHARGE_CNTL_SW1_MASK,
	.ad_pos = PCA9420_ACT_DISCHARGE_CNTL_SW1_POS,
	.max_ua = 250000,
	.ranges = buck1_ranges,
	.num_ranges = ARRAY_SIZE(buck1_ranges),
};

static const struct regulator_pca9420_desc buck2_desc = {
	.enable_reg = PCA9420_MODECFG_0_2,
	.enable_mask = PCA9420_MODECFG_2_SW2_EN_MASK,
	.enable_val = PCA9420_MODECFG_2_SW2_EN_VAL,
	.vsel_mask = PCA9420_MODECFG_1_SW2_OUT_MASK,
	.vsel_pos = PCA9420_MODECFG_1_SW2_OUT_POS,
	.vsel_reg = PCA9420_MODECFG_0_1,
	.ad_mask = PCA9420_ACT_DISCHARGE_CNTL_SW2_MASK,
	.ad_pos = PCA9420_ACT_DISCHARGE_CNTL_SW2_POS,
	.max_ua = 500000,
	.ranges = buck2_ranges,
	.num_ranges = ARRAY_SIZE(buck2_ranges),
};

static const struct regulator_pca9420_desc ldo1_desc = {
	.enable_reg = PCA9420_MODECFG_0_2,
	.enable_mask = PCA9420_MODECFG_2_LDO1_EN_MASK,
	.enable_val = PCA9420_MODECFG_2_LDO1_EN_VAL,
	.vsel_mask = PCA9420_MODECFG_2_LDO1_OUT_MASK,
	.vsel_pos = PCA9420_MODECFG_2_LDO1_OUT_POS,
	.vsel_reg = PCA9420_MODECFG_0_2,
	.ad_mask = PCA9420_ACT_DISCHARGE_CNTL_LDO1_MASK,
	.ad_pos = PCA9420_ACT_DISCHARGE_CNTL_LDO1_POS,
	.max_ua = 1000,
	.ranges = ldo1_ranges,
	.num_ranges = ARRAY_SIZE(ldo1_ranges),
};

static const struct regulator_pca9420_desc ldo2_desc = {
	.enable_reg = PCA9420_MODECFG_0_2,
	.enable_mask = PCA9420_MODECFG_2_LDO2_EN_MASK,
	.enable_val = PCA9420_MODECFG_2_LDO2_EN_VAL,
	.vsel_reg = PCA9420_MODECFG_0_3,
	.vsel_mask = PCA9420_MODECFG_3_LDO2_OUT_MASK,
	.vsel_pos = PCA9420_MODECFG_3_LDO2_OUT_POS,
	.ad_mask = PCA9420_ACT_DISCHARGE_CNTL_LDO2_MASK,
	.ad_pos = PCA9420_ACT_DISCHARGE_CNTL_LDO2_POS,
	.max_ua = 250000,
	.ranges = ldo2_ranges,
	.num_ranges = ARRAY_SIZE(ldo2_ranges),
};

static unsigned int regulator_pca9420_count_voltages(const struct device *dev)
{
	const struct regulator_pca9420_config *config = dev->config;

	return linear_range_group_values_count(config->desc->ranges,
					       config->desc->num_ranges);
}

static int regulator_pca9420_list_voltage(const struct device *dev,
					  unsigned int idx, int32_t *volt_uv)
{
	const struct regulator_pca9420_config *config = dev->config;

	return linear_range_group_get_value(config->desc->ranges,
					    config->desc->num_ranges, idx,
					    volt_uv);
}

static int regulator_pca9420_set_voltage(const struct device *dev,
					 int32_t min_uv, int32_t max_uv)
{
	const struct regulator_pca9420_config *config = dev->config;
	const struct regulator_pca9420_common_config *cconfig = config->parent->config;
	struct regulator_pca9420_common_data *cdata = config->parent->data;
	uint16_t idx;
	int ret;

	ret = linear_range_group_get_win_index(config->desc->ranges,
					       config->desc->num_ranges, min_uv,
					       max_uv, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	idx <<= config->desc->vsel_pos;

	return i2c_reg_update_byte_dt(&cconfig->i2c, config->desc->vsel_reg +
				      PCA9420_MODECFG_OFFSET(cdata->dvs_state),
				      config->desc->vsel_mask, (uint8_t)idx);
}

static int regulator_pca9420_get_voltage(const struct device *dev,
					 int32_t *volt_uv)
{
	const struct regulator_pca9420_config *config = dev->config;
	const struct regulator_pca9420_common_config *cconfig = config->parent->config;
	struct regulator_pca9420_common_data *cdata = config->parent->data;
	int ret;
	uint8_t raw_reg;

	ret = i2c_reg_read_byte_dt(&cconfig->i2c, config->desc->vsel_reg +
				   PCA9420_MODECFG_OFFSET(cdata->dvs_state),
				   &raw_reg);
	if (ret < 0) {
		return ret;
	}

	raw_reg = (raw_reg & config->desc->vsel_mask) >> config->desc->vsel_pos;

	return linear_range_group_get_value(config->desc->ranges,
					    config->desc->num_ranges, raw_reg,
					    volt_uv);
}

static int regulator_pca9420_get_current_limit(const struct device *dev,
					       int32_t *curr_ua)
{
	const struct regulator_pca9420_config *config = dev->config;
	const struct regulator_pca9420_common_config *cconfig = config->parent->config;

	if (cconfig->vin_ilim_ua == 0U) {
		*curr_ua = config->desc->max_ua;
	} else {
		*curr_ua = MIN(config->desc->max_ua, cconfig->vin_ilim_ua);
	}

	return 0;
}

static int regulator_pca9420_set_active_discharge(const struct device *dev,
							bool active_discharge)
{
	const struct regulator_pca9420_config *config = dev->config;
	const struct regulator_pca9420_common_config *cconfig = config->parent->config;
	uint8_t dis_val;

	dis_val = (!active_discharge) << config->desc->ad_pos;
	return i2c_reg_update_byte_dt(&cconfig->i2c, PCA9420_ACT_DISCHARGE_CNTL,
				      config->desc->ad_mask, dis_val);
}

static int regulator_pca9420_get_active_discharge(const struct device *dev,
							bool *active_discharge)
{
	const struct regulator_pca9420_config *config = dev->config;
	const struct regulator_pca9420_common_config *cconfig = config->parent->config;
	uint8_t raw_reg;
	int ret;

	ret = i2c_reg_read_byte_dt(&cconfig->i2c, PCA9420_ACT_DISCHARGE_CNTL, &raw_reg);
	if (ret < 0) {
		return ret;
	}

	*active_discharge = !((raw_reg & config->desc->ad_mask) >> config->desc->ad_pos);

	return 0;
}

static int regulator_pca9420_enable(const struct device *dev)
{
	const struct regulator_pca9420_config *config = dev->config;
	const struct regulator_pca9420_common_config *cconfig = config->parent->config;
	struct regulator_pca9420_common_data *cdata = config->parent->data;
	uint8_t en_val;

	en_val = config->enable_inverted ? 0 : config->desc->enable_val;
	return i2c_reg_update_byte_dt(&cconfig->i2c, config->desc->enable_reg
				      + PCA9420_MODECFG_OFFSET(cdata->dvs_state),
				      config->desc->enable_mask, en_val);
}

static int regulator_pca9420_disable(const struct device *dev)
{
	const struct regulator_pca9420_config *config = dev->config;
	const struct regulator_pca9420_common_config *cconfig = config->parent->config;
	struct regulator_pca9420_common_data *cdata = config->parent->data;
	uint8_t dis_val;

	dis_val = config->enable_inverted ? config->desc->enable_val : 0;
	return i2c_reg_update_byte_dt(&cconfig->i2c, config->desc->enable_reg
				      + PCA9420_MODECFG_OFFSET(cdata->dvs_state),
				      config->desc->enable_mask, dis_val);
}

static DEVICE_API(regulator, api) = {
	.enable = regulator_pca9420_enable,
	.disable = regulator_pca9420_disable,
	.count_voltages = regulator_pca9420_count_voltages,
	.list_voltage = regulator_pca9420_list_voltage,
	.set_voltage = regulator_pca9420_set_voltage,
	.get_voltage = regulator_pca9420_get_voltage,
	.get_current_limit = regulator_pca9420_get_current_limit,
	.set_active_discharge = regulator_pca9420_set_active_discharge,
	.get_active_discharge = regulator_pca9420_get_active_discharge,
};

static int regulator_pca9420_init(const struct device *dev)
{
	const struct regulator_pca9420_config *config = dev->config;
	const struct regulator_pca9420_common_config *cconfig = config->parent->config;

	regulator_common_data_init(dev);

	if (!device_is_ready(config->parent)) {
		return -ENODEV;
	}

	/* configure mode voltages */
	for (uint8_t i = 0U; i < ARRAY_SIZE(config->modes_uv); i++) {
		int ret;

		if (config->modes_uv[i] == 0) {
			/* disable mode if voltage is 0 */
			ret = i2c_reg_update_byte_dt(
				&cconfig->i2c,
				config->desc->enable_reg + PCA9420_MODECFG_OFFSET(i),
				config->desc->enable_mask, 0U);
			if (ret < 0) {
				return ret;
			}
		} else if (config->modes_uv[i] > 0) {
			uint16_t idx;

			/* program mode voltage */
			ret = linear_range_group_get_win_index(
				config->desc->ranges, config->desc->num_ranges,
				config->modes_uv[i], config->modes_uv[i], &idx);
			if (ret == -EINVAL) {
				return ret;
			}

			idx <<= config->desc->vsel_pos;

			ret = i2c_reg_update_byte_dt(
				&cconfig->i2c,
				config->desc->vsel_reg + PCA9420_MODECFG_OFFSET(i),
				config->desc->vsel_mask, (uint8_t)idx);
			if (ret < 0) {
				return ret;
			}
		}
	}

	return regulator_common_init(dev, false);
}

int regulator_pca9420_dvs_state_set(const struct device *dev,
				    regulator_dvs_state_t state)
{
	const struct regulator_pca9420_common_config *config = dev->config;
	struct regulator_pca9420_common_data *data = dev->data;
	int ret;

	if (state >= PCA9420_NUM_MODES) {
		return -ENOTSUP;
	}

	if (config->enable_modesel_pins) {
		/*
		 * The user cannot set DVS state via this API,
		 * but they may want to query/set voltages for another mode.
		 * Return -EPERM to indicate change failed, but change the
		 * dvs_state variable so the user can access the alternative
		 * dvs mode settings.
		 */
		data->dvs_state = state;
		return -EPERM;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, PCA9420_TOP_CNTL3,
				      PCA9420_TOP_CNTL3_MODE_I2C_MASK,
				      state << PCA9420_TOP_CNTL3_MODE_I2C_POS);
	if (ret < 0) {
		return ret;
	}
	/* Record new DVS state */
	data->dvs_state = state;
	return 0;
}

static DEVICE_API(regulator_parent, parent_api) = {
	.dvs_state_set = regulator_pca9420_dvs_state_set,
};

static int regulator_pca9420_common_init(const struct device *dev)
{
	const struct regulator_pca9420_common_config *config = dev->config;
	uint8_t reg_val = PCA9420_TOP_CNTL0_VIN_ILIM_SEL_DISABLED;
	int ret;

	if (!device_is_ready(config->i2c.bus)) {
		return -ENODEV;
	}

	if (config->enable_modesel_pins) {
		/* enable MODESEL0/1 pins for each mode */
		for (uint8_t i = 0U; i < PCA9420_NUM_MODES; i++) {
			ret = i2c_reg_update_byte_dt(
				&config->i2c,
				PCA9420_MODECFG_0_0 +
				PCA9420_MODECFG_OFFSET(i),
				PCA9420_MODECFG_0_X_EN_MODE_SEL_BY_PIN,
				PCA9420_MODECFG_0_X_EN_MODE_SEL_BY_PIN);
			if (ret < 0) {
				return ret;
			}
		}
	}

	/* configure VIN current limit */
	if (config->vin_ilim_ua != 0U) {
		reg_val = (config->vin_ilim_ua - PCA9420_VIN_ILIM_MIN_UA) /
			  PCA9420_VIN_ILIM_UA_LSB;
	}

	ret = i2c_reg_update_byte_dt(
		&config->i2c, PCA9420_TOP_CNTL0,
		PCA9420_TOP_CNTL0_VIN_ILIM_SEL_MASK,
		reg_val << PCA9420_TOP_CNTL0_VIN_ILIM_SEL_POS);

	if (ret != 0) {
		return ret;
	}

	/* configure ASYS UVLO threshold */
	return i2c_reg_update_byte_dt(&config->i2c, PCA9420_TOP_CNTL2,
		PCA9420_TOP_CNTL2_ASYS_UVLO_SEL_MASK,
		config->asys_uvlo_sel_mv <<
		PCA9420_TOP_CNTL2_ASYS_UVLO_SEL_POS);
}

#define REGULATOR_PCA9420_DEFINE(node_id, id, name, _parent)                   \
	static struct regulator_pca9420_data data_##id;                        \
                                                                               \
	static const struct regulator_pca9420_config config_##id = {           \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),            \
		.enable_inverted = DT_PROP(node_id, enable_inverted),          \
		.modes_uv = {                                                  \
			DT_PROP_OR(node_id, nxp_mode0_microvolt, -1),          \
			DT_PROP_OR(node_id, nxp_mode1_microvolt, -1),          \
			DT_PROP_OR(node_id, nxp_mode2_microvolt, -1),          \
			DT_PROP_OR(node_id, nxp_mode3_microvolt, -1),          \
		},                                                             \
		.desc = &name ## _desc,                                        \
		.parent = _parent,                                             \
	};                                                                     \
                                                                               \
	DEVICE_DT_DEFINE(node_id, regulator_pca9420_init, NULL, &data_##id,    \
			 &config_##id, POST_KERNEL,                            \
			 CONFIG_REGULATOR_PCA9420_INIT_PRIORITY, &api);

#define REGULATOR_PCA9420_DEFINE_COND(inst, child, parent)                     \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                \
		    (REGULATOR_PCA9420_DEFINE(DT_INST_CHILD(inst, child),      \
					      child ## inst, child, parent)),  \
		    ())

#define REGULATOR_PCA9420_DEFINE_ALL(inst)                                     \
	static const struct regulator_pca9420_common_config config_##inst = {  \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                             \
		.vin_ilim_ua = DT_INST_PROP(inst, nxp_vin_ilim_microamp),      \
		.enable_modesel_pins =                                         \
			DT_INST_PROP(inst, nxp_enable_modesel_pins),           \
		.asys_uvlo_sel_mv =                                            \
			DT_INST_ENUM_IDX(inst, nxp_asys_uvlo_sel_millivolt),   \
	};                                                                     \
                                                                               \
	static struct regulator_pca9420_common_data data_##inst;               \
                                                                               \
	DEVICE_DT_INST_DEFINE(inst, regulator_pca9420_common_init, NULL,       \
			      &data_##inst,                                    \
			      &config_##inst, POST_KERNEL,                     \
			      CONFIG_REGULATOR_PCA9420_COMMON_INIT_PRIORITY,   \
			      &parent_api);                                    \
                                                                               \
	REGULATOR_PCA9420_DEFINE_COND(inst, buck1, DEVICE_DT_INST_GET(inst))   \
	REGULATOR_PCA9420_DEFINE_COND(inst, buck2, DEVICE_DT_INST_GET(inst))   \
	REGULATOR_PCA9420_DEFINE_COND(inst, ldo1, DEVICE_DT_INST_GET(inst))    \
	REGULATOR_PCA9420_DEFINE_COND(inst, ldo2, DEVICE_DT_INST_GET(inst))

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_PCA9420_DEFINE_ALL)
