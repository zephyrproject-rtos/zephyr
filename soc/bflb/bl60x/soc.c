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

#include <clic.h>
#include <bflb_soc.h>
#include <glb_reg.h>
#include <hbn_reg.h>
#include <pds_reg.h>

/* Set Embedded Flash Pullup */
static void system_bor_init(void)
{
	uint32_t tmp = 0;

	tmp = sys_read32(HBN_BASE + HBN_BOR_CFG_OFFSET);
	/* borThreshold = 1 */
	tmp = (tmp & HBN_BOR_VTH_UMSK) | ((uint32_t)(1) << HBN_BOR_VTH_POS);
	/* enablePorInBor true*/
	tmp = (tmp & HBN_BOR_SEL_UMSK) | ((uint32_t)(1) << HBN_BOR_SEL_POS);
	/* enableBor true*/
	tmp = (tmp & HBN_PU_BOR_UMSK) | ((uint32_t)(1) << HBN_PU_BOR_POS);
	sys_write32(tmp, HBN_BASE + HBN_BOR_CFG_OFFSET);


	/* enableBorInt false */
	tmp = sys_read32(HBN_BASE + HBN_IRQ_MODE_OFFSET);
	tmp = tmp & HBN_IRQ_BOR_EN_UMSK;
	sys_write32(tmp, HBN_BASE + HBN_IRQ_MODE_OFFSET);
}

void soc_early_init_hook(void)
{
	uint32_t *p;
	uint32_t i = 0;
	uint32_t tmp = 0;

	/* disable hardware_pullup_pull_down (reg_en_hw_pu_pd = 0) */
	tmp = sys_read32(HBN_BASE + HBN_IRQ_MODE_OFFSET);
	/* "BL_CLR_REG_BIT" */
	tmp = tmp & HBN_REG_EN_HW_PU_PD_UMSK;
	sys_write32(tmp, HBN_BASE + HBN_IRQ_MODE_OFFSET);

	/* 'seam' 0kb, undocumented */
	tmp = sys_read32(GLB_BASE + GLB_SEAM_MISC_OFFSET);
	tmp = (tmp & GLB_EM_SEL_UMSK) | ((uint32_t)(0) << GLB_EM_SEL_POS);
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

	/* init bor for all platform */
	system_bor_init();
}
