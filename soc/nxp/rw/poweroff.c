/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>

#include <fsl_power.h>

void z_sys_poweroff(void)
{
	const power_sleep_config_t config = {
		/* PM4 discards execution context, including the upper AON SRAM. */
		.memPdCfg = PMU_MEM_CFG_AON_MEM_RET_MASK,
	};

	POWER_PowerOffWlan();
	POWER_PowerOffBle();
	/* RC32K and XTAL32K do not need the CAU reference while powered off. */
	if ((PMU->CLK_AON & PMU_CLK_AON_CLK_32K_AON_MASK) <= PMU_CLK_AON_CLK_32K_AON(1U)) {
		POWER_ConfigCauInSleep(true);
	}

	/* Leave the caller's PMU wake sources armed while masking NVIC IRQs. */
	for (size_t i = 0U; i < DIV_ROUND_UP(CONFIG_NUM_IRQS, 32U); i++) {
		NVIC->ICER[i] = UINT32_MAX;
		NVIC->ICPR[i] = UINT32_MAX;
	}

	(void)POWER_EnterPowerMode(kPOWER_Pm4, &config);

	/* A successful PM4 wake starts at the reset vector. */
	k_panic();
	CODE_UNREACHABLE;
}
