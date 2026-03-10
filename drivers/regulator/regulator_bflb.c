/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(regulator_bflb, CONFIG_REGULATOR_LOG_LEVEL);

#include <bflb_soc.h>
#include <hbn_reg.h>

static const struct linear_range regulator_ranges[] = {
	LINEAR_RANGE_INIT(600000u, 50000u, 0u, 15u),
};

#ifdef CONFIG_SOC_SERIES_BL61X
#define REGULATOR_BFLB_MIN_V_ID 2
#else
#define REGULATOR_BFLB_MIN_V_ID 0
#endif

#define REGULATOR_BFLB_MAX_CANON_VOLTAGE_ID 10

static const size_t num_regulator_ranges = ARRAY_SIZE(regulator_ranges);

enum regulator_bflb_type {
	REGULATOR_BFLB_TYPE_SOC,
	REGULATOR_BFLB_TYPE_RT,
	REGULATOR_BFLB_TYPE_AON,
};

struct regulator_bflb_config {
	struct regulator_common_config common;
	const uintptr_t reg;
	const enum regulator_bflb_type type;
	const uint8_t sleep_v_id;
};

struct regulator_bflb_data {
	struct regulator_common_data data;
};

/*
 * APIs
 */

static unsigned int regulator_bflb_count_voltages(const struct device *dev)
{
	return linear_range_group_values_count(regulator_ranges, num_regulator_ranges);
}

static int regulator_bflb_list_voltage(const struct device *dev, unsigned int idx, int32_t *volt_uv)
{
	return linear_range_group_get_value(regulator_ranges, num_regulator_ranges, idx, volt_uv);
}

static int regulator_bflb_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_bflb_config *config = dev->config;
	uint16_t idx;
	int ret;
	uint32_t tmp;

	ret = linear_range_group_get_win_index(regulator_ranges,
					       num_regulator_ranges, min_uv, max_uv, &idx);
	if (ret < 0) {
		return ret;
	}

	if (idx < REGULATOR_BFLB_MIN_V_ID) {
		return -EINVAL;
	}

	if (idx > REGULATOR_BFLB_MAX_CANON_VOLTAGE_ID) {
		LOG_WRN("Demanded voltage is over default, this may result in damage to the chip");
	}

	tmp = sys_read32(config->reg + HBN_GLB_OFFSET);
	switch (config->type) {
	case REGULATOR_BFLB_TYPE_SOC:
		tmp &= HBN_SW_LDO11SOC_VOUT_SEL_AON_UMSK;
		tmp |= idx << HBN_SW_LDO11SOC_VOUT_SEL_AON_POS;
		break;
	case REGULATOR_BFLB_TYPE_RT:
		tmp &= HBN_SW_LDO11_RT_VOUT_SEL_UMSK;
		tmp |= idx << HBN_SW_LDO11_RT_VOUT_SEL_POS;
		break;
	case REGULATOR_BFLB_TYPE_AON:
		tmp &= HBN_SW_LDO11_AON_VOUT_SEL_UMSK;
		tmp |= idx << HBN_SW_LDO11_AON_VOUT_SEL_POS;
		break;
	default:
		return -EINVAL;
	}
	sys_write32(tmp, config->reg + HBN_GLB_OFFSET);

	return 0;
}

static int regulator_bflb_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_bflb_config *config = dev->config;
	uint32_t idx;

	idx = sys_read32(config->reg + HBN_GLB_OFFSET);
	switch (config->type) {
	case REGULATOR_BFLB_TYPE_SOC:
		idx = (idx & HBN_SW_LDO11SOC_VOUT_SEL_AON_MSK) >> HBN_SW_LDO11SOC_VOUT_SEL_AON_POS;
		break;
	case REGULATOR_BFLB_TYPE_RT:
		idx = (idx & HBN_SW_LDO11_RT_VOUT_SEL_MSK) >> HBN_SW_LDO11_RT_VOUT_SEL_POS;
		break;
	case REGULATOR_BFLB_TYPE_AON:
		idx = (idx & HBN_SW_LDO11_AON_VOUT_SEL_MSK) >> HBN_SW_LDO11_AON_VOUT_SEL_POS;
		break;
	default:
		return -EINVAL;
	}

	return linear_range_group_get_value(regulator_ranges, num_regulator_ranges, idx, volt_uv);
}

static int regulator_bflb_init(const struct device *dev)
{
	const struct regulator_bflb_config *config = dev->config;
	uint32_t tmp;

	regulator_common_data_init(dev);

	if (config->type != REGULATOR_BFLB_TYPE_SOC) {
		tmp = sys_read32(config->reg + HBN_CTL_OFFSET);
		switch (config->type) {
		case REGULATOR_BFLB_TYPE_RT:
			tmp &= HBN_LDO11_RT_VOUT_SEL_UMSK;
			tmp |= config->sleep_v_id << HBN_LDO11_RT_VOUT_SEL_POS;
			break;
		case REGULATOR_BFLB_TYPE_AON:
			tmp &= HBN_LDO11_AON_VOUT_SEL_UMSK;
			tmp |= config->sleep_v_id << HBN_LDO11_AON_VOUT_SEL_POS;
			break;
		default:
			return -EINVAL;
		}
		sys_write32(tmp, config->reg + HBN_CTL_OFFSET);
	}

	return regulator_common_init(dev, true);
}

static DEVICE_API(regulator, api) = {
	.count_voltages = regulator_bflb_count_voltages,
	.list_voltage = regulator_bflb_list_voltage,
	.set_voltage = regulator_bflb_set_voltage,
	.get_voltage = regulator_bflb_get_voltage,
};

#define REGULATOR_BFLB_CHECKS(n)						\
	BUILD_ASSERT(DT_ENUM_IDX(n, sleep_microvolt) > REGULATOR_BFLB_MIN_V_ID,	\
		     "Voltage must be at least 0.70v");

#define REGULATOR_BFLB_DEFINE(n, n_type)							\
	REGULATOR_BFLB_CHECKS(n)								\
	static struct regulator_bflb_data data_##n;						\
												\
	static const struct regulator_bflb_config config_##n = {				\
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(n),					\
		.reg = DT_REG_ADDR(n),								\
		.type = n_type,									\
		.sleep_v_id = DT_ENUM_IDX(n, sleep_microvolt),					\
	};											\
												\
	DEVICE_DT_DEFINE(n, regulator_bflb_init, NULL, &data_##n, &config_##n,			\
			      PRE_KERNEL_1, CONFIG_REGULATOR_BFLB_INIT_PRIORITY, &api);

DT_FOREACH_STATUS_OKAY_VARGS(bflb_aon_regulator, REGULATOR_BFLB_DEFINE, REGULATOR_BFLB_TYPE_AON);
DT_FOREACH_STATUS_OKAY_VARGS(bflb_rt_regulator, REGULATOR_BFLB_DEFINE, REGULATOR_BFLB_TYPE_RT);
DT_FOREACH_STATUS_OKAY_VARGS(bflb_soc_regulator, REGULATOR_BFLB_DEFINE, REGULATOR_BFLB_TYPE_SOC);
