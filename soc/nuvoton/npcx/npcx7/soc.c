/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

static int soc_npcx7_init(void)
{
	struct scfg_reg *inst_scfg = (struct scfg_reg *)
			DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), scfg);

	/*
	 * Set bit 7 of DEVCNT again for npcx7 series. Please see Errata
	 * for more information. It will be fixed in next chip.
	 */
	inst_scfg->DEVCNT |= BIT(7);

	return 0;
}

SYS_INIT(soc_npcx7_init, PRE_KERNEL_1, 0);
