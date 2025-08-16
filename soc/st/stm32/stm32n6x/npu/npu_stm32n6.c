/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_npu

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/init.h>
#include <soc.h>

#include <zephyr/drivers/clock_control/stm32_clock_control.h>

/* Read-only driver configuration */
struct npu_stm32_cfg {
	/* Clock configuration. */
	struct stm32_pclken pclken;
	/* Reset configuration */
	const struct reset_dt_spec reset;
};

static void npu_risaf_config(void)
{
	RIMC_MasterConfig_t RIMC_master = {0};

	RIMC_master.MasterCID = RIF_CID_1;
	RIMC_master.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV;
	HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_NPU, &RIMC_master);
	HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_NPU,
					      RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
}

static int npu_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct npu_stm32_cfg *cfg = dev->config;

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

	if (clock_control_on(clk, (clock_control_subsys_t) &cfg->pclken) != 0) {
		return -EIO;
	}

	if (!device_is_ready(cfg->reset.dev)) {
		return -ENODEV;
	}

	/* Reset timer to default state using RCC */
	(void)reset_line_toggle_dt(&cfg->reset);

	npu_risaf_config();

	return 0;
}


static const struct npu_stm32_cfg npu_stm32_cfg = {
	.pclken = {
		.enr = DT_CLOCKS_CELL(DT_NODELABEL(npu), bits),
		.bus = DT_CLOCKS_CELL(DT_NODELABEL(npu), bus),
	},
	.reset = RESET_DT_SPEC_GET(DT_NODELABEL(npu)),
};

DEVICE_DT_DEFINE(DT_NODELABEL(npu), npu_stm32_init, NULL,
		 NULL, &npu_stm32_cfg, POST_KERNEL,
		 CONFIG_APPLICATION_INIT_PRIORITY, NULL);
