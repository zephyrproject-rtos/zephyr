/*
 * Copyright 2023 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_smartbond_regulator

#include <stdint.h>

#include <zephyr/drivers/regulator.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <DA1469xAB.h>

LOG_MODULE_REGISTER(regulator_da1469x, CONFIG_REGULATOR_LOG_LEVEL);

#define DCDC_REQUESTED (DCDC_DCDC_VDD_REG_DCDC_VDD_ENABLE_HV_Msk |\
			DCDC_DCDC_VDD_REG_DCDC_VDD_ENABLE_LV_Msk)

#define DA1469X_LDO_3V0_MODE_VBAT				BIT(8)
#define DA1469X_LDO_3V0_MODE_VBUS				BIT(9)

static const struct linear_range curren_ranges[] = {
	LINEAR_RANGE_INIT(30000, 30000, 0, 31),
};

static const struct linear_range vdd_clamp_ranges[] = {
	LINEAR_RANGE_INIT(706000, 0, 15, 15),
	LINEAR_RANGE_INIT(798000, 0, 14, 14),
	LINEAR_RANGE_INIT(828000, 0, 13, 13),
	LINEAR_RANGE_INIT(861000, 0, 11, 11),
	LINEAR_RANGE_INIT(862000, 0, 12, 12),
	LINEAR_RANGE_INIT(889000, 0, 10, 10),
	LINEAR_RANGE_INIT(918000, 0, 9, 9),
	LINEAR_RANGE_INIT(946000, 0, 3, 3),
	LINEAR_RANGE_INIT(952000, 0, 8, 8),
	LINEAR_RANGE_INIT(978000, 0, 2, 2),
	LINEAR_RANGE_INIT(1005000, 0, 1, 1),
	LINEAR_RANGE_INIT(1030000, 0, 7, 7),
	LINEAR_RANGE_INIT(1037000, 0, 0, 0),
	LINEAR_RANGE_INIT(1058000, 0, 6, 6),
	LINEAR_RANGE_INIT(1089000, 0, 5, 5),
	LINEAR_RANGE_INIT(1120000, 0, 4, 4),
};

static const struct linear_range vdd_ranges[] = {
	LINEAR_RANGE_INIT(900000, 100000, 0, 3),
};

static const struct linear_range vdd_sleep_ranges[] = {
	LINEAR_RANGE_INIT(750000, 50000, 0, 3),
};

static const struct linear_range v14_ranges[] = {
	LINEAR_RANGE_INIT(1200000, 50000, 0, 7),
};

static const struct linear_range v30_ranges[] = {
	LINEAR_RANGE_INIT(3000000, 300000, 0, 1),
};

static const struct linear_range v18_ranges[] = {
	LINEAR_RANGE_INIT(1200000, 600000, 0, 1),
};

static const struct linear_range v18p_ranges[] = {
	LINEAR_RANGE_INIT(1800000, 0, 0, 0),
};

enum da1469x_rail {
	VDD_CLAMP,
	VDD_SLEEP,
	VDD,
	V14,
	V18,
	V18P,
	V30,
};

struct dcdc_regs {
	uint32_t v18;
	uint32_t v18p;
	uint32_t vdd;
	uint32_t v14;
	uint32_t ctrl1;
};

static struct dcdc_regs dcdc_state;

struct regulator_da1469x_desc {
	const struct linear_range *voltage_ranges;
	const struct linear_range *current_ranges;
	uint8_t voltage_range_count;
	/* Bit from POWER_CTRL_REG that can be used for enabling rail */
	uint32_t enable_mask;
	uint32_t voltage_idx_mask;
	volatile uint32_t *dcdc_register;
	uint32_t *dcdc_register_shadow;
};

static const struct regulator_da1469x_desc vdd_desc = {
	.voltage_ranges = vdd_ranges,
	.current_ranges = curren_ranges,
	.voltage_range_count = ARRAY_SIZE(vdd_ranges),
	.enable_mask = CRG_TOP_POWER_CTRL_REG_LDO_CORE_ENABLE_Msk,
	.voltage_idx_mask = CRG_TOP_POWER_CTRL_REG_VDD_LEVEL_Msk,
	.dcdc_register = &DCDC->DCDC_VDD_REG,
	.dcdc_register_shadow = &dcdc_state.vdd,
};

static const struct regulator_da1469x_desc vdd_sleep_desc = {
	.voltage_ranges = vdd_sleep_ranges,
	.voltage_range_count = ARRAY_SIZE(vdd_sleep_ranges),
	.enable_mask = CRG_TOP_POWER_CTRL_REG_LDO_CORE_RET_ENABLE_SLEEP_Msk,
	.voltage_idx_mask = CRG_TOP_POWER_CTRL_REG_VDD_SLEEP_LEVEL_Msk,
};

static const struct regulator_da1469x_desc vdd_clamp_desc = {
	.voltage_ranges = vdd_clamp_ranges,
	.voltage_range_count = ARRAY_SIZE(vdd_clamp_ranges),
	.enable_mask = 0,
	.voltage_idx_mask = CRG_TOP_POWER_CTRL_REG_VDD_CLAMP_LEVEL_Msk,
};

static const struct regulator_da1469x_desc v14_desc = {
	.voltage_ranges = v14_ranges,
	.current_ranges = curren_ranges,
	.voltage_range_count = ARRAY_SIZE(v14_ranges),
	.enable_mask = CRG_TOP_POWER_CTRL_REG_LDO_RADIO_ENABLE_Msk,
	.voltage_idx_mask = CRG_TOP_POWER_CTRL_REG_V14_LEVEL_Msk,
	.dcdc_register = &DCDC->DCDC_V14_REG,
	.dcdc_register_shadow = &dcdc_state.v14,
};

static const struct regulator_da1469x_desc v18_desc = {
	.voltage_ranges = v18_ranges,
	.current_ranges = curren_ranges,
	.voltage_range_count = ARRAY_SIZE(v18_ranges),
	.enable_mask = CRG_TOP_POWER_CTRL_REG_LDO_1V8_ENABLE_Msk |
		       CRG_TOP_POWER_CTRL_REG_LDO_1V8_RET_ENABLE_SLEEP_Msk,
	.voltage_idx_mask = CRG_TOP_POWER_CTRL_REG_V18_LEVEL_Msk,
	.dcdc_register = &DCDC->DCDC_V18_REG,
	.dcdc_register_shadow = &dcdc_state.v18,
};

static const struct regulator_da1469x_desc v18p_desc = {
	.voltage_ranges = v18p_ranges,
	.current_ranges = curren_ranges,
	.voltage_range_count = ARRAY_SIZE(v18p_ranges),
	.enable_mask = CRG_TOP_POWER_CTRL_REG_LDO_1V8P_ENABLE_Msk |
		       CRG_TOP_POWER_CTRL_REG_LDO_1V8P_RET_ENABLE_SLEEP_Msk,
	.voltage_idx_mask = 0,
	.dcdc_register = &DCDC->DCDC_V18P_REG,
	.dcdc_register_shadow = &dcdc_state.v18p,
};

static const struct regulator_da1469x_desc v30_desc = {
	.voltage_ranges = v30_ranges,
	.voltage_range_count = ARRAY_SIZE(v30_ranges),
	.enable_mask = CRG_TOP_POWER_CTRL_REG_LDO_3V0_RET_ENABLE_SLEEP_Msk |
		       CRG_TOP_POWER_CTRL_REG_LDO_3V0_MODE_Msk,
	.voltage_idx_mask = CRG_TOP_POWER_CTRL_REG_V30_LEVEL_Msk,
};

#define DA1469X_LDO_VDD_CLAMP_RET	0
#define DA1469X_LDO_VDD_SLEEP_RET	0
#define DA1469X_LDO_VDD_RET		CRG_TOP_POWER_CTRL_REG_LDO_CORE_RET_ENABLE_SLEEP_Msk
#define DA1469X_LDO_V14_RET		0
#define DA1469X_LDO_V18_RET		CRG_TOP_POWER_CTRL_REG_LDO_1V8_RET_ENABLE_SLEEP_Msk
#define DA1469X_LDO_V18P_RET		CRG_TOP_POWER_CTRL_REG_LDO_1V8P_RET_ENABLE_SLEEP_Msk
#define DA1469X_LDO_V30_RET		CRG_TOP_POWER_CTRL_REG_LDO_3V0_RET_ENABLE_SLEEP_Msk

struct regulator_da1469x_config {
	struct regulator_common_config common;
	enum da1469x_rail rail;
	const struct regulator_da1469x_desc *desc;
	uint32_t power_bits;
	uint32_t dcdc_bits;
};

struct regulator_da1469x_data {
	struct regulator_common_data common;
};

static int regulator_da1469x_enable(const struct device *dev)
{
	const struct regulator_da1469x_config *config = dev->config;
	uint32_t reg_val;

	if (config->desc->enable_mask & config->power_bits) {
		reg_val = CRG_TOP->POWER_CTRL_REG & ~(config->desc->enable_mask);
		reg_val |= config->power_bits & config->desc->enable_mask;
		CRG_TOP->POWER_CTRL_REG |= reg_val;
	}

	if (config->desc->dcdc_register) {
		reg_val = *config->desc->dcdc_register &
			  ~(DCDC_DCDC_V14_REG_DCDC_V14_ENABLE_HV_Msk |
			    DCDC_DCDC_V14_REG_DCDC_V14_ENABLE_LV_Msk);
		reg_val |= config->dcdc_bits;
		*config->desc->dcdc_register_shadow = reg_val;
		*config->desc->dcdc_register = reg_val;
	}

	/*
	 * Enable DCDC if:
	 * 1. it was not already enabled, and
	 * 2. VBAT is above minimal value
	 * 3. Just turned on rail requested DCDC
	 */
	if (((DCDC->DCDC_CTRL1_REG & DCDC_DCDC_CTRL1_REG_DCDC_ENABLE_Msk) == 0) &&
	    (CRG_TOP->ANA_STATUS_REG & CRG_TOP_ANA_STATUS_REG_COMP_VBAT_HIGH_Msk) &&
	    config->dcdc_bits & DCDC_REQUESTED) {
		DCDC->DCDC_CTRL1_REG |= DCDC_DCDC_CTRL1_REG_DCDC_ENABLE_Msk;
		dcdc_state.ctrl1 = DCDC->DCDC_CTRL1_REG;
	}

	return 0;
}

static int regulator_da1469x_disable(const struct device *dev)
{
	const struct regulator_da1469x_config *config = dev->config;
	uint32_t reg_val;

	if (config->desc->enable_mask & config->power_bits) {
		CRG_TOP->POWER_CTRL_REG &= ~(config->desc->enable_mask &
					     config->power_bits);
	}
	if (config->desc->dcdc_register) {
		reg_val = *config->desc->dcdc_register &
			  ~(DCDC_DCDC_V14_REG_DCDC_V14_ENABLE_HV_Msk |
			    DCDC_DCDC_V14_REG_DCDC_V14_ENABLE_LV_Msk);
		*config->desc->dcdc_register_shadow = reg_val;
		*config->desc->dcdc_register = reg_val;
	}

	/* Turn off DCDC if it's no longer requested by any rail */
	if ((DCDC->DCDC_CTRL1_REG & DCDC_DCDC_CTRL1_REG_DCDC_ENABLE_Msk) &&
	    (DCDC->DCDC_VDD_REG & DCDC_REQUESTED) == 0 &&
	    (DCDC->DCDC_V14_REG & DCDC_REQUESTED) == 0 &&
	    (DCDC->DCDC_V18_REG & DCDC_REQUESTED) == 0 &&
	    (DCDC->DCDC_V18P_REG & DCDC_REQUESTED) == 0) {
		DCDC->DCDC_CTRL1_REG &= ~DCDC_DCDC_CTRL1_REG_DCDC_ENABLE_Msk;
		dcdc_state.ctrl1 = DCDC->DCDC_CTRL1_REG;
	}

	return 0;
}

static unsigned int regulator_da1469x_count_voltages(const struct device *dev)
{
	const struct regulator_da1469x_config *config = dev->config;

	return linear_range_group_values_count(config->desc->voltage_ranges,
					       config->desc->voltage_range_count);
}

static int regulator_da1469x_list_voltage(const struct device *dev,
					  unsigned int idx,
					  int32_t *volt_uv)
{
	const struct regulator_da1469x_config *config = dev->config;

	if (config->desc->voltage_ranges) {
		return linear_range_group_get_value(config->desc->voltage_ranges,
						    config->desc->voltage_range_count,
						    idx, volt_uv);
	}

	return -ENOTSUP;
}

static int regulator_da1469x_set_voltage(const struct device *dev, int32_t min_uv,
					 int32_t max_uv)
{
	int ret;
	const struct regulator_da1469x_config *config = dev->config;
	uint16_t idx;
	uint32_t mask;

	ret = linear_range_group_get_win_index(config->desc->voltage_ranges,
					       config->desc->voltage_range_count,
					       min_uv, max_uv, &idx);

	if (ret == 0) {
		mask = config->desc->voltage_idx_mask;
		/*
		 * Mask is 0 for V18.
		 * Setting value 1.8V is accepted since range is valid and already checked.
		 */
		if (mask) {
			CRG_TOP->POWER_CTRL_REG = (CRG_TOP->POWER_CTRL_REG & ~mask) |
						  FIELD_PREP(mask, idx);
		}
	}

	return ret;
}

static int regulator_da1469x_get_voltage(const struct device *dev,
					 int32_t *volt_uv)
{
	const struct regulator_da1469x_config *config = dev->config;
	uint16_t idx;

	if (config->desc->voltage_idx_mask) {
		idx = FIELD_GET(CRG_TOP->POWER_CTRL_REG, config->desc->voltage_idx_mask);
	} else {
		idx = 0;
	}

	return linear_range_group_get_value(config->desc->voltage_ranges,
					    config->desc->voltage_range_count, idx, volt_uv);
}

static int regulator_da1469x_set_current_limit(const struct device *dev,
					       int32_t min_ua, int32_t max_ua)
{
	const struct regulator_da1469x_config *config = dev->config;
	int ret;
	uint16_t idx;
	uint32_t reg_val;

	if (config->desc->current_ranges == NULL) {
		return -ENOTSUP;
	}

	ret = linear_range_group_get_win_index(config->desc->current_ranges,
					       1,
					       min_ua, max_ua, &idx);
	if (ret) {
		return ret;
	}

	/* All registers have same bits layout */
	reg_val = *config->desc->dcdc_register & ~(DCDC_DCDC_V14_REG_DCDC_V14_CUR_LIM_MAX_HV_Msk |
						   DCDC_DCDC_V14_REG_DCDC_V14_CUR_LIM_MAX_LV_Msk |
						   DCDC_DCDC_V14_REG_DCDC_V14_CUR_LIM_MIN_Msk);
	reg_val |= FIELD_PREP(DCDC_DCDC_V14_REG_DCDC_V14_CUR_LIM_MAX_HV_Msk, idx);
	reg_val |= FIELD_PREP(DCDC_DCDC_V14_REG_DCDC_V14_CUR_LIM_MAX_LV_Msk, idx);
	reg_val |= FIELD_PREP(DCDC_DCDC_V14_REG_DCDC_V14_CUR_LIM_MIN_Msk, idx);
	*config->desc->dcdc_register_shadow = reg_val;
	*config->desc->dcdc_register = reg_val;

	return ret;
}

static int regulator_da1469x_get_current_limit(const struct device *dev,
					       int32_t *curr_ua)
{
	const struct regulator_da1469x_config *config = dev->config;
	int ret;
	uint16_t idx;

	if (config->desc->current_ranges == NULL) {
		return -ENOTSUP;
	}
	idx = FIELD_GET(*config->desc->dcdc_register,
			DCDC_DCDC_V14_REG_DCDC_V14_CUR_LIM_MAX_HV_Msk);
	ret = linear_range_group_get_value(config->desc->current_ranges, 1, idx, curr_ua);

	return ret;
}

static const struct regulator_driver_api regulator_da1469x_api = {
	.enable = regulator_da1469x_enable,
	.disable = regulator_da1469x_disable,
	.count_voltages = regulator_da1469x_count_voltages,
	.list_voltage = regulator_da1469x_list_voltage,
	.set_voltage = regulator_da1469x_set_voltage,
	.get_voltage = regulator_da1469x_get_voltage,
	.set_current_limit = regulator_da1469x_set_current_limit,
	.get_current_limit = regulator_da1469x_get_current_limit,
};

static int regulator_da1469x_init(const struct device *dev)
{
	const struct regulator_da1469x_config *config = dev->config;

	regulator_common_data_init(dev);

	if ((config->rail == V30) &&
	    (config->power_bits & CRG_TOP_POWER_CTRL_REG_LDO_3V0_REF_Msk)) {
		CRG_TOP->POWER_CTRL_REG |= CRG_TOP_POWER_CTRL_REG_LDO_3V0_REF_Msk;
	}

	return regulator_common_init(dev, 0);
}

#if IS_ENABLED(CONFIG_PM_DEVICE)
static int regulator_da1469x_pm_action(const struct device *dev,
				       enum pm_device_action action)
{
	const struct regulator_da1469x_config *config = dev->config;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (config->desc->dcdc_register) {
			*config->desc->dcdc_register = *config->desc->dcdc_register_shadow;
			if ((CRG_TOP->ANA_STATUS_REG & CRG_TOP_ANA_STATUS_REG_COMP_VBAT_HIGH_Msk) &&
			    (*config->desc->dcdc_register_shadow & DCDC_REQUESTED)) {
				DCDC->DCDC_CTRL1_REG = dcdc_state.ctrl1;
			}
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/*
		 * Shadow register is saved on each regulator API call, there is no need
		 * to save it here.
		 */
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif

#define REGULATOR_DA1469X_DEFINE(node, id, rail_id)                            \
	static struct regulator_da1469x_data data_##id;                        \
                                                                               \
	static const struct regulator_da1469x_config config_##id = {           \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node),               \
		.desc = &id ## _desc,                                          \
		.power_bits =                                                  \
			(DT_PROP(node, renesas_regulator_v30_clamp) *          \
			CRG_TOP_POWER_CTRL_REG_CLAMP_3V0_VBAT_ENABLE_Msk) |    \
			(DT_PROP(node, renesas_regulator_v30_vbus) *           \
			DA1469X_LDO_3V0_MODE_VBAT) |                           \
			(DT_PROP(node, renesas_regulator_v30_vbat) *           \
			DA1469X_LDO_3V0_MODE_VBUS) |                           \
			(DT_PROP(node, renesas_regulator_sleep_ldo) *          \
			(DA1469X_LDO_ ## rail_id ##_RET)) |                    \
			(DT_PROP(node, renesas_regulator_v30_ref_bandgap) *    \
			CRG_TOP_POWER_CTRL_REG_LDO_3V0_REF_Msk),               \
		.dcdc_bits =                                                   \
			(DT_PROP(node, renesas_regulator_dcdc_vbat_high) *     \
			DCDC_DCDC_VDD_REG_DCDC_VDD_ENABLE_HV_Msk) |            \
			(DT_PROP(node, renesas_regulator_dcdc_vbat_low) *      \
			DCDC_DCDC_VDD_REG_DCDC_VDD_ENABLE_LV_Msk)              \
	};                                                                     \
	PM_DEVICE_DT_DEFINE(node, regulator_da1469x_pm_action);                \
	DEVICE_DT_DEFINE(node, regulator_da1469x_init,                         \
			 PM_DEVICE_DT_GET(node),                               \
			 &data_##id,                                           \
			 &config_##id, PRE_KERNEL_1,                           \
			 CONFIG_REGULATOR_DA1469X_INIT_PRIORITY,               \
			 &regulator_da1469x_api);

#define REGULATOR_DA1469X_DEFINE_COND(inst, child, source)                     \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                \
		    (REGULATOR_DA1469X_DEFINE(                                 \
			DT_INST_CHILD(inst, child), child, source)),           \
		    ())

#define REGULATOR_DA1469X_DEFINE_ALL(inst)                                     \
	REGULATOR_DA1469X_DEFINE_COND(inst, vdd_clamp, VDD_CLAMP)              \
	REGULATOR_DA1469X_DEFINE_COND(inst, vdd_sleep, VDD_SLEEP)              \
	REGULATOR_DA1469X_DEFINE_COND(inst, vdd, VDD)                          \
	REGULATOR_DA1469X_DEFINE_COND(inst, v14, V14)                          \
	REGULATOR_DA1469X_DEFINE_COND(inst, v18, V18)                          \
	REGULATOR_DA1469X_DEFINE_COND(inst, v18p, V18P)                        \
	REGULATOR_DA1469X_DEFINE_COND(inst, v30, V30)                          \

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_DA1469X_DEFINE_ALL)
