/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/regulator.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/syscon.h>
#include "dlink_sys_reg.h"
#include "dlink_ldo_reg.h"

LOG_MODULE_REGISTER(regulator_rts5817, CONFIG_REGULATOR_LOG_LEVEL);

#define DT_DRV_COMPAT realtek_rts5817_regulator

#define DVDDS_IDX_OFFSET 5

typedef enum {
	SVIO_VOL_1V800 = 0x04,
	SVIO_VOL_1V850 = 0x05,
	SVIO_VOL_2V800 = 0x09,
	SVIO_VOL_2V850 = 0x0a,
	SVIO_VOL_2V900 = 0x0c,
	SVIO_VOL_3V000 = 0x0e,
	SVIO_VOL_3V100 = 0x10,
	SVIO_VOL_3V300 = 0x14,
	SVIO_VOL_INVALID = 0xff,
} svio_voltage_t;

typedef enum {
	SVA_VOL_1V800 = 0x03,
	SVA_VOL_1V850 = 0x05,
	SVA_VOL_2V800 = 0x09,
	SVA_VOL_2V850 = 0x0a,
	SVA_VOL_2V900 = 0x0b,
	SVA_VOL_3V000 = 0x0d,
	SVA_VOL_3V100 = 0x10,
	SVA_VOL_3V300 = 0x14,
	SVA_VOL_INVALID = 0xff,
} sva_voltage_t;

typedef enum {
	LDO_DVDDS = 0,
	LDO_SVIO = 1,
	LDO_SVA = 2,
} ldo_id_t;

static const struct linear_range dvdds_range[] = {
	LINEAR_RANGE_INIT(1000000U, 25000U, 0x0U, 0x8U),
};

static const struct linear_range svio_range[] = {
	LINEAR_RANGE_INIT(1800000U, 50000U, 0x0U, 0x1U),
	LINEAR_RANGE_INIT(2800000U, 50000U, 0x2U, 0x4U),
	LINEAR_RANGE_INIT(3000000U, 100000U, 0x5U, 0x6U),
	LINEAR_RANGE_INIT(3300000U, 0U, 0x7U, 0x7U),
};

static const struct linear_range sva_range[] = {
	LINEAR_RANGE_INIT(1800000U, 50000U, 0x0U, 0x1U),
	LINEAR_RANGE_INIT(2800000U, 50000U, 0x2U, 0x4U),
	LINEAR_RANGE_INIT(3000000U, 100000U, 0x5U, 0x6U),
	LINEAR_RANGE_INIT(3300000U, 0U, 0x7U, 0x7U),
};

struct regulator_rtsfp_desc {
	const uint8_t num_ranges;
	const struct linear_range *ranges;
	ldo_id_t id;
};

struct regulator_rtsfp_data {
	struct regulator_common_data common;
};

struct regulator_rtsfp_config {
	struct regulator_common_config common;
	const struct device *syscon_ldo;
	const struct device *syscon_sys;
	const struct regulator_rtsfp_desc *desc;
	bool ocp_en;
};

static const struct regulator_rtsfp_desc dvdds_desc = {
	.ranges = dvdds_range,
	.num_ranges = ARRAY_SIZE(dvdds_range),
	.id = LDO_DVDDS,
};

static const struct regulator_rtsfp_desc svio_desc = {
	.ranges = svio_range,
	.num_ranges = ARRAY_SIZE(svio_range),
	.id = LDO_SVIO,
};

static const struct regulator_rtsfp_desc sva_desc = {
	.ranges = sva_range,
	.num_ranges = ARRAY_SIZE(sva_range),
	.id = LDO_SVA,
};

static inline svio_voltage_t svio_idx_to_voltage(uint16_t idx)
{
	switch (idx) {
	case 0:
		return SVIO_VOL_1V800;
	case 1:
		return SVIO_VOL_1V850;
	case 2:
		return SVIO_VOL_2V800;
	case 3:
		return SVIO_VOL_2V850;
	case 4:
		return SVIO_VOL_2V900;
	case 5:
		return SVIO_VOL_3V000;
	case 6:
		return SVIO_VOL_3V100;
	case 7:
		return SVIO_VOL_3V300;
	default:
		return SVIO_VOL_INVALID;
	}
}

static inline sva_voltage_t sva_idx_to_voltage(uint16_t idx)
{
	switch (idx) {
	case 0:
		return SVA_VOL_1V800;
	case 1:
		return SVA_VOL_1V850;
	case 2:
		return SVA_VOL_2V800;
	case 3:
		return SVA_VOL_2V850;
	case 4:
		return SVA_VOL_2V900;
	case 5:
		return SVA_VOL_3V000;
	case 6:
		return SVA_VOL_3V100;
	case 7:
		return SVA_VOL_3V300;
	default:
		return SVA_VOL_INVALID;
	}
}

static uint16_t svio_voltage_to_idx(svio_voltage_t vol)
{
	switch (vol) {
	case SVIO_VOL_1V800:
		return 0;
	case SVIO_VOL_1V850:
		return 1;
	case SVIO_VOL_2V800:
		return 2;
	case SVIO_VOL_2V850:
		return 3;
	case SVIO_VOL_2V900:
		return 4;
	case SVIO_VOL_3V000:
		return 5;
	case SVIO_VOL_3V100:
		return 6;
	case SVIO_VOL_3V300:
		return 7;
	default:
		return UINT16_MAX;
	}
}

static uint16_t sva_voltage_to_idx(sva_voltage_t vol)
{
	switch (vol) {
	case SVA_VOL_1V800:
		return 0;
	case SVA_VOL_1V850:
		return 1;
	case SVA_VOL_2V800:
		return 2;
	case SVA_VOL_2V850:
		return 3;
	case SVA_VOL_2V900:
		return 4;
	case SVA_VOL_3V000:
		return 5;
	case SVA_VOL_3V100:
		return 6;
	case SVA_VOL_3V300:
		return 7;
	default:
		return UINT16_MAX;
	}
}

static int regulator_rtsfp_ocp_configure(const struct device *dev, bool enable)
{
	const struct regulator_rtsfp_config *config = dev->config;
	uint32_t ocp_reg;
	uint32_t ocp_mask;
	uint32_t al1_mask;
	int ret;

	switch (config->desc->id) {
	case LDO_SVIO:
		ocp_reg = R_LDO_TOP_OC_SVIO;
		ocp_mask = OC_EN_SVIO_MASK | OC_POW_OFF_EN_SVIO_MASK;
		al1_mask = SVIO_SUSPEND_OCP_EN;
		break;
	case LDO_SVA:
		ocp_reg = R_LDO_TOP_OC_SVA;
		ocp_mask = OC_EN_SVA_MASK | OC_POW_OFF_EN_SVA_MASK;
		al1_mask = SVA_SUSPEND_OCP_EN;
		break;
	case LDO_DVDDS:
		/* LDO_DVDDS do not support OCP */
		return 0;
	default:

		return -ENOTSUP;
	}

	if (enable) {
		ret = syscon_update_bits(config->syscon_ldo, ocp_reg, ocp_mask, ocp_mask);
		if (ret < 0) {
			return ret;
		}
		ret = syscon_update_bits(config->syscon_sys, R_AL_DUMMY1, al1_mask, al1_mask);
	} else {
		ret = syscon_update_bits(config->syscon_ldo, ocp_reg, ocp_mask, 0);
		if (ret < 0) {
			return ret;
		}
		ret = syscon_update_bits(config->syscon_sys, R_AL_DUMMY1, al1_mask, 0);
	}
	return ret;
}

static int regulator_rtsfp_configure(const struct device *dev, bool enable)
{
	const struct regulator_rtsfp_config *config = dev->config;
	uint32_t pd_res_mask;
	uint32_t ref_mask;
	uint32_t pow_on_mask;
	uint32_t al0_mask;
	int ret;

	switch (config->desc->id) {
	case LDO_SVIO:
		pd_res_mask = SVIO_PD_RES_MASK;
		ref_mask = SVIO_VOL_REF_MASK;
		pow_on_mask = POW_SVIO_ON;
		al0_mask = SVIO_SUSPEND_OCP_FLAG;
		break;
	case LDO_SVA:
		pd_res_mask = SVA_PD_RES_MASK;
		ref_mask = SVA_VOL_REF_MASK;
		pow_on_mask = POW_SVA_ON;
		al0_mask = SVA_SUSPEND_OCP_FLAG;
		break;
	case LDO_DVDDS:
		/* LDO_DVDDS is always enabled */
		return 0;
	default:
		return -ENOTSUP;
	}

	if (enable) {
		ret = syscon_update_bits(config->syscon_ldo, R_LDO_TOP_PD, ref_mask, 0);
		if (ret < 0) {
			return ret;
		}
		ret = syscon_update_bits(config->syscon_ldo, R_LDO_TOP_POW, pow_on_mask,
					 pow_on_mask);
		if (ret < 0) {
			return ret;
		}
		k_usleep(300);
		ret = syscon_update_bits(config->syscon_ldo, R_LDO_TOP_PD, ref_mask, ref_mask);
		if (ret < 0) {
			return ret;
		}
		k_usleep(500);

		ret = syscon_update_bits(config->syscon_sys, R_AL_DUMMY0, al0_mask, al0_mask);
		if (ret < 0) {
			return ret;
		}
		k_usleep(1);
		ret = syscon_update_bits(config->syscon_sys, R_AL_DUMMY0, al0_mask, 0);
	} else {
		ret = syscon_update_bits(config->syscon_ldo, R_LDO_TOP_PD, pd_res_mask,
					 pd_res_mask);
		if (ret < 0) {
			return ret;
		}
		ret = syscon_update_bits(config->syscon_ldo, R_LDO_TOP_POW, pow_on_mask, 0);
	}

	return ret;
}

static int regulator_rtsfp_enable(const struct device *dev)
{
	const struct regulator_rtsfp_config *config = dev->config;
	int ret;

	ret = regulator_rtsfp_ocp_configure(dev, config->ocp_en);
	if (ret) {
		return ret;
	}

	return regulator_rtsfp_configure(dev, true);
}

static int regulator_rtsfp_disable(const struct device *dev)
{
	int ret;

	ret = regulator_rtsfp_ocp_configure(dev, false);
	if (ret) {
		return ret;
	}

	return regulator_rtsfp_configure(dev, false);
}

static int regulator_rtsfp_set_ldo_svio(const struct device *dev, uint16_t idx)
{
	const struct regulator_rtsfp_config *config = dev->config;
	svio_voltage_t val;

	val = svio_idx_to_voltage(idx);
	if (val == SVIO_VOL_INVALID) {
		return -EINVAL;
	}

	return syscon_update_bits(config->syscon_ldo, R_LDO_TOP_VO, REG_TUNE_VO_SVIO_MASK,
				  val << REG_TUNE_VO_SVIO_OFFSET);
}

static int regulator_rtsfp_set_ldo_sva(const struct device *dev, uint16_t idx)
{
	const struct regulator_rtsfp_config *config = dev->config;
	sva_voltage_t val;

	val = sva_idx_to_voltage(idx);
	if (val == SVA_VOL_INVALID) {
		return -EINVAL;
	}

	return syscon_update_bits(config->syscon_ldo, R_LDO_TOP_VO, REG_TUNE_VO_SVA_MASK,
				  val << REG_TUNE_VO_SVA_OFFSET);
}

static int regulator_rtsfp_set_ldo_dvdds(const struct device *dev, uint16_t idx)
{
	const struct regulator_rtsfp_config *config = dev->config;
	uint32_t val;

	val = idx + DVDDS_IDX_OFFSET;

	return syscon_update_bits(config->syscon_ldo, R_LDO_TOP_VO, REG_TUNE_VO_CORE_MASK,
				  val << REG_TUNE_VO_CORE_OFFSET);
}

static int regulator_rtsfp_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_rtsfp_config *config = dev->config;
	uint16_t idx;
	int ret;

	ret = linear_range_group_get_win_index(config->desc->ranges, config->desc->num_ranges,
					       min_uv, max_uv, &idx);
	if (ret != 0) {
		return ret;
	}

	switch (config->desc->id) {
	case LDO_SVIO:
		return regulator_rtsfp_set_ldo_svio(dev, idx);
	case LDO_SVA:
		return regulator_rtsfp_set_ldo_sva(dev, idx);
	case LDO_DVDDS:
		return regulator_rtsfp_set_ldo_dvdds(dev, idx);
	default:
		LOG_ERR("%s: set voltage is not supported", dev->name);
		return -ENOTSUP;
	}
}

static int regulator_rtsfp_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_rtsfp_config *config = dev->config;
	uint32_t val;
	uint16_t idx;
	int ret;

	switch (config->desc->id) {
	case LDO_SVIO:
		ret = syscon_read_reg(config->syscon_ldo, R_LDO_TOP_VO, &val);
		if (ret < 0) {
			return ret;
		}
		val = (val & REG_TUNE_VO_SVIO_MASK) >> REG_TUNE_VO_SVIO_OFFSET;
		idx = svio_voltage_to_idx(val);
		break;

	case LDO_SVA:
		ret = syscon_read_reg(config->syscon_ldo, R_LDO_TOP_VO, &val);
		if (ret < 0) {
			return ret;
		}
		val = (val & REG_TUNE_VO_SVA_MASK) >> REG_TUNE_VO_SVA_OFFSET;
		idx = sva_voltage_to_idx(val);
		break;

	case LDO_DVDDS:
		ret = syscon_read_reg(config->syscon_ldo, R_LDO_TOP_VO, &val);
		if (ret < 0) {
			return ret;
		}
		val = (val & REG_TUNE_VO_CORE_MASK) >> REG_TUNE_VO_CORE_OFFSET;
		idx = val - DVDDS_IDX_OFFSET;
		break;

	default:
		LOG_ERR("%s: get voltage is not supported", dev->name);
		return -ENOTSUP;
	}

	return linear_range_group_get_value(config->desc->ranges, config->desc->num_ranges, idx,
					    volt_uv);
}

static int regulator_rtsfp_list_voltage(const struct device *dev, unsigned int idx,
					int32_t *volt_uv)
{
	const struct regulator_rtsfp_config *config = dev->config;

	return linear_range_group_get_value(config->desc->ranges, config->desc->num_ranges, idx,
					    volt_uv);
}

static unsigned int regulator_rtsfp_count_voltages(const struct device *dev)
{
	const struct regulator_rtsfp_config *config = dev->config;

	return linear_range_group_values_count(config->desc->ranges, config->desc->num_ranges);
}

#ifdef CONFIG_PM_DEVICE
static int rts5817_regulator_resume(const struct device *dev)
{
	const struct regulator_rtsfp_config *config = dev->config;
	int ret;

	switch (config->desc->id) {
	case LDO_SVIO:
		ret = syscon_update_bits(config->syscon_ldo, R_LDO_TOP_POW, SVIO_SAVE_POWER, 0);
		if (ret < 0) {
			return ret;
		}
		ret = syscon_update_bits(config->syscon_ldo, R_LDO_TOP_TUNE_OCP,
					 REG_TUNE_OCP_LVL_SVIO_MASK, 0x2);
		return ret;
	case LDO_SVA:
		return syscon_update_bits(config->syscon_ldo, R_LDO_TOP_POW, SVA_SAVE_POWER, 0);
	case LDO_DVDDS:
		/* Do nothing for LDO_DVDDS */
		return 0;
	default:
		return -EINVAL;
	}
}

static int rts5817_regulator_suspend(const struct device *dev)
{
	const struct regulator_rtsfp_config *config = dev->config;
	int ret;

	switch (config->desc->id) {
	case LDO_SVIO:
		ret = syscon_update_bits(config->syscon_ldo, R_LDO_TOP_POW, SVIO_SAVE_POWER,
					 SVIO_SAVE_POWER);
		if (ret < 0) {
			return ret;
		}
		ret = syscon_update_bits(config->syscon_ldo, R_LDO_TOP_TUNE_OCP,
					 REG_TUNE_OCP_LVL_SVIO_MASK, 0x4);
		return ret;
	case LDO_SVA:
		return syscon_update_bits(config->syscon_ldo, R_LDO_TOP_POW, SVA_SAVE_POWER,
					  SVA_SAVE_POWER);
	case LDO_DVDDS:
		/* Do nothing for LDO_DVDDS */
		return 0;
	default:
		return -EINVAL;
	}
}

static int rts5817_regulator_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return rts5817_regulator_resume(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		return rts5817_regulator_suspend(dev);
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

static int regulator_rtsfp_init(const struct device *dev)
{
	const struct regulator_rtsfp_config *config = dev->config;

	if (config->desc->id == LDO_DVDDS) {
		syscon_update_bits(config->syscon_sys, R_AL_DUMMY0,
				   CFG_USB_LDO_EN | CFG_USB_LDO_VALUE, CFG_USB_LDO_EN);
	}

	regulator_common_data_init(dev);

	return regulator_common_init(dev, false);
}

static DEVICE_API(regulator, regulator_rtsfp_api) = {
	.enable = regulator_rtsfp_enable,
	.disable = regulator_rtsfp_disable,
	.set_voltage = regulator_rtsfp_set_voltage,
	.get_voltage = regulator_rtsfp_get_voltage,
	.list_voltage = regulator_rtsfp_list_voltage,
	.count_voltages = regulator_rtsfp_count_voltages,
};

#define REGULATOR_RTSFP_DEFINE(node, id)                                                           \
	static struct regulator_rtsfp_data data_##id;                                              \
                                                                                                   \
	static const struct regulator_rtsfp_config config_##id = {                                 \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node),                                   \
		.syscon_ldo = DEVICE_DT_GET(DT_PHANDLE(DT_PARENT(node), syscon_ldo)),              \
		.syscon_sys = DEVICE_DT_GET(DT_PHANDLE(DT_PARENT(node), syscon_sys)),              \
		.desc = &id##_desc,                                                                \
		.ocp_en = DT_PROP_OR(node, ocp_enable, false)};                                    \
	PM_DEVICE_DT_DEFINE(node, rts5817_regulator_pm_action);                                    \
	DEVICE_DT_DEFINE(node, regulator_rtsfp_init, PM_DEVICE_DT_GET(node), &data_##id,           \
			 &config_##id, POST_KERNEL, CONFIG_REGULATOR_RTS5817_INIT_PRIORITY,        \
			 &regulator_rtsfp_api);

#define REGULATOR_RTSFP_DEFINE_COND(inst, child)                                                   \
	IF_ENABLED(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                                    \
		    (REGULATOR_RTSFP_DEFINE(DT_INST_CHILD(inst, child), child)))

#define REGULATOR_RTSFP_DEFINE_ALL(inst)                                                           \
	REGULATOR_RTSFP_DEFINE_COND(inst, dvdds)                                                   \
	REGULATOR_RTSFP_DEFINE_COND(inst, svio)                                                    \
	REGULATOR_RTSFP_DEFINE_COND(inst, sva)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_RTSFP_DEFINE_ALL)
