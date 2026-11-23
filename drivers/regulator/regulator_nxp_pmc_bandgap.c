/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pmc_bandgap_buffer

#include <zephyr/device.h>
#include <zephyr/drivers/regulator.h>
#include <fsl_device_registers.h>

struct regulator_nxp_pmc_bandgap_config {
	struct regulator_common_config common;
	PMC_Type *base;
};

struct regulator_nxp_pmc_bandgap_data {
	struct regulator_common_data common;
};

static void regulator_nxp_pmc_bandgap_set(const struct device *dev, bool enable)
{
	const struct regulator_nxp_pmc_bandgap_config *config = dev->config;
	/* ACKISO is write-1-to-clear, never write it back. */
	uint8_t value =
		config->base->REGSC & (uint8_t)~(PMC_REGSC_ACKISO_MASK | PMC_REGSC_BGBE_MASK);

	if (enable) {
		value |= PMC_REGSC_BGBE_MASK;
	}

	config->base->REGSC = value;
}

static int regulator_nxp_pmc_bandgap_enable(const struct device *dev)
{
	regulator_nxp_pmc_bandgap_set(dev, true);

	return 0;
}

static int regulator_nxp_pmc_bandgap_disable(const struct device *dev)
{
	regulator_nxp_pmc_bandgap_set(dev, false);

	return 0;
}

static int regulator_nxp_pmc_bandgap_init(const struct device *dev)
{
	const struct regulator_nxp_pmc_bandgap_config *config = dev->config;
	const bool is_enabled = (config->base->REGSC & PMC_REGSC_BGBE_MASK) != 0U;

	regulator_common_data_init(dev);

	return regulator_common_init(dev, is_enabled);
}

static DEVICE_API(regulator, regulator_nxp_pmc_bandgap_api) = {
	.enable = regulator_nxp_pmc_bandgap_enable,
	.disable = regulator_nxp_pmc_bandgap_disable,
};

#define REGULATOR_NXP_PMC_BANDGAP_DEFINE(inst)                                                     \
	static struct regulator_nxp_pmc_bandgap_data data_##inst;                                  \
                                                                                                   \
	static const struct regulator_nxp_pmc_bandgap_config config_##inst = {                     \
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(inst),                              \
		.base = (PMC_Type *)DT_REG_ADDR(DT_INST_PARENT(inst)),                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, regulator_nxp_pmc_bandgap_init, NULL, &data_##inst,            \
			      &config_##inst, POST_KERNEL,                                         \
			      CONFIG_REGULATOR_NXP_PMC_BANDGAP_INIT_PRIORITY,                      \
			      &regulator_nxp_pmc_bandgap_api);

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_NXP_PMC_BANDGAP_DEFINE)
