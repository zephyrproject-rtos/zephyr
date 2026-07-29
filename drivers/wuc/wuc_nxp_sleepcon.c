/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_sleepcon_wuc

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_device_registers.h>

LOG_MODULE_REGISTER(nxp_sleepcon_wuc, CONFIG_WUC_LOG_LEVEL);

#define SLEEPCON_WAKEUPEN_COUNT 5U
#define SLEEPCON_WAKEUP_ID_MAX  (SLEEPCON_WAKEUPEN_COUNT * 32U)

struct nxp_sleepcon_wuc_config {
	SLEEPCON0_Type *base;
};

static int nxp_sleepcon_wuc_enable(const struct device *dev, uint32_t id)
{
	const struct nxp_sleepcon_wuc_config *config = dev->config;

	if (id >= SLEEPCON_WAKEUP_ID_MAX) {
		return -EINVAL;
	}

	(&config->base->WAKEUPEN0_SET)[id / 32U] = BIT(id % 32U);

	LOG_DBG("enabled wakeup source %u (WAKEUPEN%u bit %u)", id, id / 32U, id % 32U);

	return 0;
}

static int nxp_sleepcon_wuc_disable(const struct device *dev, uint32_t id)
{
	const struct nxp_sleepcon_wuc_config *config = dev->config;

	if (id >= SLEEPCON_WAKEUP_ID_MAX) {
		return -EINVAL;
	}

	(&config->base->WAKEUPEN0_CLR)[id / 32U] = BIT(id % 32U);

	LOG_DBG("disabled wakeup source %u (WAKEUPEN%u bit %u)", id, id / 32U, id % 32U);

	return 0;
}

static DEVICE_API(wuc, nxp_sleepcon_wuc_api) = {
	.enable = nxp_sleepcon_wuc_enable,
	.disable = nxp_sleepcon_wuc_disable,
};

#define NXP_SLEEPCON_WUC_INIT(inst)                                                                \
	static const struct nxp_sleepcon_wuc_config nxp_sleepcon_wuc_config_##inst = {             \
		.base = (SLEEPCON0_Type *)DT_INST_REG_ADDR(inst),                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &nxp_sleepcon_wuc_config_##inst,             \
			      PRE_KERNEL_1, CONFIG_WUC_INIT_PRIORITY, &nxp_sleepcon_wuc_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_SLEEPCON_WUC_INIT)
