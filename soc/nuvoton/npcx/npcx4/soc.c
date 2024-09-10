/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#define NPCX_FIU_INST_INIT(node_id) DT_REG_ADDR(node_id),

static uintptr_t fiu_insts[] = {
	DT_FOREACH_STATUS_OKAY(nuvoton_npcx_fiu_qspi, NPCX_FIU_INST_INIT)
};

extern void scfg_init(void);
void soc_early_init_hook(void)
{
	/*
	 * Make sure UMA_ADDR_SIZE field of UMA_ECTS register is zero in npcx4
	 * series. There should be no address field in UMA mode by default.
	 */
	for (int i = 0; i < ARRAY_SIZE(fiu_insts); i++) {
		struct fiu_reg *const inst = (struct fiu_reg *)(fiu_insts[i]);

		SET_FIELD(inst->UMA_ECTS, NPCX_UMA_ECTS_UMA_ADDR_SIZE, 0);
	}
	scfg_init();
}
