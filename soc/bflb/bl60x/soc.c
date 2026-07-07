/*
 * Copyright (c) 2021-2025 ATL Electronics
 * Copyright (c) 2024-2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bouffalo Lab RISC-V MCU series initialization code
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
#include <pds_reg.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(wifi_ram))
/* WIFI_RAM is a custom memory-region -- Zephyr's arch_data_copy /
 * arch_bss_zero do not touch it.  Zero it here so the WiFi blob's
 * SHAREDRAM / SHAREDRAMIPC objects (which the linker places NOLOAD)
 * start in a known state.
 */
static void bl60x_zero_wifi_ram(void)
{
	volatile uint32_t *r = (volatile uint32_t *)DT_REG_ADDR(DT_NODELABEL(wifi_ram));
	size_t cnt = DT_REG_SIZE(DT_NODELABEL(wifi_ram)) / sizeof(uint32_t);

	while (cnt-- > 0U) {
		*r++ = 0U;
	}
}
#endif

void soc_early_init_hook(void)
{
	uint32_t *p;
	uint32_t i = 0;
	uint32_t tmp = 0;

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(wifi_ram))
	bl60x_zero_wifi_ram();
#endif

	/* disable hardware_pullup_pull_down (reg_en_hw_pu_pd = 0) */
	tmp = sys_read32(HBN_BASE + HBN_IRQ_MODE_OFFSET);
	/* "BL_CLR_REG_BIT" */
	tmp = tmp & HBN_REG_EN_HW_PU_PD_UMSK;
	sys_write32(tmp, HBN_BASE + HBN_IRQ_MODE_OFFSET);

	/* BLE exchange memory (SEAM): 8 KB when BLE enabled, 0 otherwise */
#define BLE_EM_SEL_8K  3U
	tmp = sys_read32(GLB_BASE + GLB_SEAM_MISC_OFFSET);
	tmp &= GLB_EM_SEL_UMSK;
#if defined(CONFIG_BT_BFLB_BL60X)
	tmp |= (BLE_EM_SEL_8K << GLB_EM_SEL_POS);
#endif
	sys_write32(tmp, GLB_BASE + GLB_SEAM_MISC_OFFSET);

	/* Fix 26M xtal clkpll_sdmin */
	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_SDM_OFFSET);

	if ((tmp & PDS_CLKPLL_SDMIN_MSK) == 0x49D39D) {
		tmp = (tmp & PDS_CLKPLL_SDMIN_UMSK) | (uint32_t)(0x49D89E);
		sys_write32(tmp, PDS_BASE + PDS_CLKPLL_SDM_OFFSET);
	}

	tmp = sys_read32(GLB_BASE + GLB_PARM_OFFSET);
	/* GLB_UART_Sig_Swap_Set(UART_SIG_SWAP_NONE);
	 * no swap = 0
	 * see bl602_glb.h for other possible values
	 */
	tmp = (tmp & GLB_UART_SWAP_SET_UMSK) | ((uint32_t)(0) <<
GLB_UART_SWAP_SET_POS);
	/* GLB_JTAG_Sig_Swap_Set(JTAG_SIG_SWAP_NONE); */
	tmp = (tmp & GLB_JTAG_SWAP_SET_UMSK) | ((uint32_t)(0) <<
GLB_JTAG_SWAP_SET_POS);
	sys_write32(tmp, GLB_BASE + GLB_PARM_OFFSET);

	/* CLear all interrupt */
	p = (uint32_t *)(CLIC_HART0_ADDR + CLIC_INTIE);

	for (i = 0; i < (IRQn_LAST + 3) / 4; i++) {
		p[i] = 0;
	}

	p = (uint32_t *)(CLIC_HART0_ADDR + CLIC_INTIP);

	for (i = 0; i < (IRQn_LAST + 3) / 4; i++) {
		p[i] = 0;
	}

	sys_cache_data_flush_and_invd_all();
}
