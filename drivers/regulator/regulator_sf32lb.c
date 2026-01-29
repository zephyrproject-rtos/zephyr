/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include <register.h>

LOG_MODULE_REGISTER(regulator_sf32lb, CONFIG_REGULATOR_LOG_LEVEL);

#define DT_DRV_COMPAT sifli_sf32lb52x_ldo

/* LDO descriptor with enable and power-down masks */
struct sf32lb_ldo_desc {
	uint32_t enable_mask;
	uint32_t pd_mask;
};

struct regulator_sf32lb_config {
	struct regulator_common_config common;
	uintptr_t reg;
	const struct sf32lb_ldo_desc *desc;
};

struct regulator_sf32lb_data {
	struct regulator_common_data common;
};

static int regulator_sf32lb_enable(const struct device *dev)
{
	const struct regulator_sf32lb_config *config = dev->config;
	const struct sf32lb_ldo_desc *desc = config->desc;
	uint32_t val;

	val = sys_read32(config->reg);
	/* Clear power down bit and set enable bit */
	val &= ~desc->pd_mask;
	val |= desc->enable_mask;
	sys_write32(val, config->reg);

	return 0;
}

static int regulator_sf32lb_disable(const struct device *dev)
{
	const struct regulator_sf32lb_config *config = dev->config;
	const struct sf32lb_ldo_desc *desc = config->desc;
	uint32_t val;

	val = sys_read32(config->reg);
	/* Clear enable bit and set power down bit */
	val &= ~desc->enable_mask;
	val |= desc->pd_mask;
	sys_write32(val, config->reg);

	return 0;
}

static unsigned int regulator_sf32lb_count_voltages(const struct device *dev)
{
	int32_t min_uv;

	return (regulator_common_get_min_voltage(dev, &min_uv) < 0) ? 0U : 1U;
}

static int regulator_sf32lb_list_voltage(const struct device *dev, unsigned int idx,
					 int32_t *volt_uv)
{
	if (idx != 0) {
		return -EINVAL;
	}

	if (regulator_common_get_min_voltage(dev, volt_uv) < 0) {
		return -EINVAL;
	}

	return 0;
}

static int regulator_sf32lb_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	return regulator_sf32lb_list_voltage(dev, 0, volt_uv);
}

static DEVICE_API(regulator, regulator_sf32lb_api) = {
	.enable = regulator_sf32lb_enable,
	.disable = regulator_sf32lb_disable,
	.count_voltages = regulator_sf32lb_count_voltages,
	.list_voltage = regulator_sf32lb_list_voltage,
	.get_voltage = regulator_sf32lb_get_voltage,
};

static int regulator_sf32lb_init(const struct device *dev)
{
	regulator_common_data_init(dev);

	return regulator_common_init(dev, false);
}

/* LDO descriptors, indexed by the devicetree child "reg" property. */
static const struct sf32lb_ldo_desc sf32lb_ldo_descs[] = {
	{
		.enable_mask = PMUC_PERI_LDO_EN_LDO18_Msk,
		.pd_mask = PMUC_PERI_LDO_LDO18_PD_Msk,
	},
	{
		.enable_mask = PMUC_PERI_LDO_EN_VDD33_LDO2_Msk,
		.pd_mask = PMUC_PERI_LDO_VDD33_LDO2_PD_Msk,
	},
	{
		.enable_mask = PMUC_PERI_LDO_EN_VDD33_LDO3_Msk,
		.pd_mask = PMUC_PERI_LDO_VDD33_LDO3_PD_Msk,
	},
};

/* PERI_LDO register offset within PMUC */
#define SF32LB_PERI_LDO_OFFSET 0x5c

#define SF32LB_LDO_DATA_NAME(node_id) _CONCAT(data_, DT_DEP_ORD(node_id))
#define SF32LB_LDO_CONF_NAME(node_id) _CONCAT(config_, DT_DEP_ORD(node_id))

#define SF32LB_LDO_DESC(node_id) (&sf32lb_ldo_descs[DT_PROP(node_id, reg)])

#define REGULATOR_SF32LB_DEFINE(node_id)                                                           \
	static struct regulator_sf32lb_data SF32LB_LDO_DATA_NAME(node_id);                         \
                                                                                                   \
	static const struct regulator_sf32lb_config SF32LB_LDO_CONF_NAME(node_id) = {              \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.reg = DT_REG_ADDR(DT_PARENT(DT_PARENT(node_id))) + SF32LB_PERI_LDO_OFFSET,        \
		.desc = SF32LB_LDO_DESC(node_id),                                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, regulator_sf32lb_init, NULL, &SF32LB_LDO_DATA_NAME(node_id),     \
			 &SF32LB_LDO_CONF_NAME(node_id), PRE_KERNEL_1,                             \
			 CONFIG_REGULATOR_SF32LB_LDO_INIT_PRIORITY, &regulator_sf32lb_api);

#define REGULATOR_SF32LB_DEFINE_ALL(inst)                                                          \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, REGULATOR_SF32LB_DEFINE)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_SF32LB_DEFINE_ALL)
