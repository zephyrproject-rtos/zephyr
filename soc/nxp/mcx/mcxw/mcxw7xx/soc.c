/*
 * Copyright 2023-2024, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>

#include <soc.h>

#include <fsl_ccm32k.h>
#include <fsl_common.h>
#include <fsl_clock.h>

extern void nxp_nbu_init(void);

static void vbat_init(void)
{
	VBAT_Type *base = (VBAT_Type *)DT_REG_ADDR(DT_NODELABEL(vbat));

	/* Write 1 to Clear POR detect status bit.
	 *
	 * Clearing this bit is acknowledement
	 * that software has recognized a power on reset.
	 *
	 * This avoids also niche issues with NVIC read/write
	 * when searching for available interrupt lines.
	 */
	base->STATUSA |= VBAT_STATUSA_POR_DET_MASK;
};

void soc_early_init_hook(void)
{
	unsigned int oldLevel; /* old interrupt lock level */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* Smart power switch initialization */
	vbat_init();

	if (IS_ENABLED(CONFIG_PM)) {
		nxp_mcxw7x_power_init();
	}

	/* restore interrupt state */
	irq_unlock(oldLevel);

#if defined(CONFIG_NXP_NBU)
	nxp_nbu_init();
#endif
}
