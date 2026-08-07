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

#include <string.h>

#include <clic.h>
#include <bflb_soc.h>
#include <glb_reg.h>
#include <hbn_reg.h>

#if defined(CONFIG_BT_BFLB_BL70XL)
extern char _hbn_load_start[];
extern char _hbn_run_start[];
extern char _hbn_run_size[];
#endif

void soc_early_init_hook(void)
{
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

	/* 'seam' — BLE exchange memory (EM) via GLB_EM_SEL.
	 * EM_SEL values: 0x0=0KB, 0x3=8KB, 0xF=16KB
	 */
	tmp = sys_read32(GLB_BASE + GLB_SEAM_MISC_OFFSET);
#if defined(CONFIG_BFLB_BL70XL_BLE_EM_16K)
	tmp = (tmp & GLB_EM_SEL_UMSK) | (0xFU << GLB_EM_SEL_POS);
#elif defined(CONFIG_BT_BFLB_BL70XL)
	tmp = (tmp & GLB_EM_SEL_UMSK) | (0x3U << GLB_EM_SEL_POS);
#else
	tmp = (tmp & GLB_EM_SEL_UMSK) | (0x0U << GLB_EM_SEL_POS);
#endif
	sys_write32(tmp, GLB_BASE + GLB_SEAM_MISC_OFFSET);

	/* Clear all interrupt enable and pending bits */
	volatile uint32_t *p = (volatile uint32_t *)(CLIC_HART0_ADDR + CLIC_INTIE);

	for (uint32_t i = 0U; i < ((uint32_t)IRQn_LAST + 3U) / 4U; i++) {
		p[i] = 0;
	}

	p = (volatile uint32_t *)(CLIC_HART0_ADDR + CLIC_INTIP);

	for (uint32_t i = 0U; i < ((uint32_t)IRQn_LAST + 3U) / 4U; i++) {
		p[i] = 0;
	}

	sys_cache_data_flush_and_invd_all();

#if defined(CONFIG_BT_BFLB_BL70XL)
	/* Copy .hbn_code from flash (LMA) to HBN RAM (VMA) */
	memcpy(_hbn_run_start, _hbn_load_start, (size_t)_hbn_run_size);
#endif
}
