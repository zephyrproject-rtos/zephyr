/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com> / Titan Chen
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/pm/policy.h>
#include <zephyr/logging/log.h>
#include "debug_swj.h"
#include <reg/reg_system.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#if defined(CONFIG_RTS5912_ON_ENTER_CPU_IDLE_HOOK)
bool z_arm_on_enter_cpu_idle(void)
{
	/* Returning false prevent device goes to sleep mode */
	return false;
}
#endif

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	SYSTEM_Type *sys_reg = RTS5912_SCCON_REG_BASE;
	int ret;

	/* Apply device related preinit configuration */
	if (IS_ENABLED(CONFIG_RTS5912_DEBUG_SWJ)) {
		ret = swj_connector_init();
		if (ret < 0) {
			LOG_ERR("SWJ init failed %d", ret);
		}
	}

	if (IS_ENABLED(CONFIG_PM)) {
		/* turn off TRNG/ OTP/ LDO3 power */
		sys_write32(0ul, RTS5912_RNG_RUN);
		sys_write32(0ul, RTS5912_CFG_PDSTB);
		sys_reg->LDOCTRL &= ~SYSTEM_LDOCTRL_LDO3EN_Msk;
	}
}
