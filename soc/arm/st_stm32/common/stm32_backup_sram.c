/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_backup_sram

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <stm32_ll_pwr.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stm32_backup_sram, CONFIG_SOC_LOG_LEVEL);

struct stm32_backup_sram_config {
	struct stm32_pclken pclken;
};

static int stm32_backup_sram_init(const struct device *dev)
{
	const struct stm32_backup_sram_config *config = dev->config;

	int ret;

	/* enable clock for subsystem */
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	ret = clock_control_on(clk, (clock_control_subsys_t *)&config->pclken);
	if (ret < 0) {
		LOG_ERR("Could not initialize backup SRAM clock (%d)", ret);
		return ret;
	}

	/* enable write access to backup domain */
	LL_PWR_EnableBkUpAccess();
	while (!LL_PWR_IsEnabledBkUpAccess()) {
	}

	/* enable backup sram regulator (required to retain backup SRAM content
	 * while in standby or VBAT modes).
	 */
	LL_PWR_EnableBkUpRegulator();
	while (!LL_PWR_IsEnabledBkUpRegulator()) {
	}

	return 0;
}

static const struct stm32_backup_sram_config config = {
	.pclken = { .bus = DT_INST_CLOCKS_CELL(0, bus),
		    .enr = DT_INST_CLOCKS_CELL(0, bits) },
};

DEVICE_DT_INST_DEFINE(0, stm32_backup_sram_init, NULL, NULL, &config,
		      POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, NULL);
