/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bouffalo Lab BL70XL SoC initialization code
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/cache.h>

#include <clic.h>
#include <bflb_soc.h>
#include <glb_reg.h>
#include <hbn_reg.h>

void soc_early_init_hook(void)
{
	volatile uint32_t *p;
	uint32_t i = 0;
	uint32_t tmp;

	/* Clear PDS fastboot flag (HBN_RSV0) immediately.
	 * HBN registers survive normal resets (always-on domain). If a
	 * previous PDS cycle left the flag set, the ROM bootloader would
	 * jump to a stale fastboot address on every subsequent boot,
	 * preventing new firmware from running. PM code re-sets this flag
	 * only right before PDS entry.
	 */
	sys_write32(0, HBN_BASE + HBN_RSV0_OFFSET);

	/* disable hardware_pullup_pull_down (reg_en_hw_pu_pd = 0) */
	tmp = sys_read32(HBN_BASE + HBN_IRQ_MODE_OFFSET);
	tmp = tmp & HBN_REG_EN_HW_PU_PD_UMSK;
	sys_write32(tmp, HBN_BASE + HBN_IRQ_MODE_OFFSET);

	/* 'seam' 0kb, undocumented */
	tmp = sys_read32(GLB_BASE + GLB_SEAM_MISC_OFFSET);
	tmp = (tmp & GLB_EM_SEL_UMSK) | (0U << GLB_EM_SEL_POS);
	sys_write32(tmp, GLB_BASE + GLB_SEAM_MISC_OFFSET);

	/* Clear all interrupts */
	p = (volatile uint32_t *)(CLIC_HART0_ADDR + CLIC_INTIE);

	for (i = 0; i < (IRQn_LAST + 3) / 4; i++) {
		p[i] = 0;
	}

	p = (volatile uint32_t *)(CLIC_HART0_ADDR + CLIC_INTIP);

	for (i = 0; i < (IRQn_LAST + 3) / 4; i++) {
		p[i] = 0;
	}

	sys_cache_data_flush_and_invd_all();
}
