/*
 * Copyright (c) 2026 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

BUILD_ASSERT((CONFIG_NPCK_EPURST_DEBOUNCE_TIME_SEC > 0) &&
		     (CONFIG_NPCK_EPURST_DEBOUNCE_TIME_SEC < 32),
	     "Debounce time must be between 1 and 31 seconds.");

extern void scfg_init(void);
void soc_early_init_hook(void)
{
	struct glue_reg *inst_glue =
		(struct glue_reg *)DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), glue);

	struct scfg_reg *inst_scfg =
		(struct scfg_reg *)DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), scfg);

	if (IS_ENABLED(CONFIG_SOC_NPCK_EPURST_DISABLE)) {
		/* EXT_PURST# signal is not selected to the pin by default */
		inst_glue->EPURST_CTL &= ~BIT(NPCX_EPURST_CTL_EPUR2_EN);
	} else {
		inst_glue->EPURST_CTL |= BIT(NPCX_EPURST_CTL_EPUR2_EN);
		SET_FIELD(inst_glue->EPURST_DBC, NPCX_EPURST_EPUR_DBC,
			  CONFIG_NPCK_EPURST_DEBOUNCE_TIME_SEC);
	}

	if (IS_ENABLED(CONFIG_SOC_NPCK_DEBUG_DISABLE)) {
		/* Core debugging interface is disabled */
		inst_scfg->DEVCNT |= BIT(1);
	}

	scfg_init();
}
