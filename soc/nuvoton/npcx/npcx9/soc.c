/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

static int soc_npcx9_init(void)
{
	if (IS_ENABLED(CONFIG_NPCX_VCC1_RST_HANG_WORKAROUND)) {
		uintptr_t scfg_base = DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), scfg);

		SET_FIELD(NPCX_JEN_CTL1(scfg_base), NPCX_JEN_CTL1_JEN_HEN,
			  NPCX_JEN_CTL1_JEN_DISABLE);
	}

	return 0;
}

SYS_INIT(soc_npcx9_init, PRE_KERNEL_1, 0);
