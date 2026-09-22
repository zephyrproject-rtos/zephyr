/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_sdram

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(memc_renesas_ra_sdram, CONFIG_MEMC_LOG_LEVEL);

struct memc_renesas_ra_sdram_config {
	const struct pinctrl_dev_config *pincfg;
};

static int renesas_ra_sdram_init(const struct device *dev)
{
	const struct memc_renesas_ra_sdram_config *config = dev->config;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		LOG_ERR("pin function initial failed");
		return err;
	}

	R_BSP_SdramInit(true);

	return 0;
}

PINCTRL_DT_INST_DEFINE(0);
static const struct memc_renesas_ra_sdram_config config = {
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

DEVICE_DT_INST_DEFINE(0, renesas_ra_sdram_init, NULL, NULL, &config, POST_KERNEL,
		      CONFIG_MEMC_INIT_PRIORITY, NULL);
