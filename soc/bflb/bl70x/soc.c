/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
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

void soc_early_init_hook(void)
{
	uint32_t *p;
	uint32_t i = 0;
	uint32_t tmp;

	/* disable hardware_pullup_pull_down (reg_en_hw_pu_pd = 0) */
	tmp = sys_read32(HBN_BASE + HBN_IRQ_MODE_OFFSET);
	/* "BL_CLR_REG_BIT" */
	tmp = tmp & HBN_REG_EN_HW_PU_PD_UMSK;
	sys_write32(tmp, HBN_BASE + HBN_IRQ_MODE_OFFSET);

	/* Configure "seam" (BLE exchange memory): 8KB or 16KB with BLE, 0KB otherwise */
	tmp = sys_read32(GLB_BASE + GLB_SEAM_MISC_OFFSET);
#if defined(CONFIG_BFLB_BL70X_BLE_EM_16K)
	tmp = (tmp & GLB_EM_SEL_UMSK) | (0xFU << GLB_EM_SEL_POS);
#elif defined(CONFIG_BT_BFLB_BL70X)
	tmp = (tmp & GLB_EM_SEL_UMSK) | (3U << GLB_EM_SEL_POS);
#else
	tmp = (tmp & GLB_EM_SEL_UMSK) | (0U << GLB_EM_SEL_POS);
#endif
	sys_write32(tmp, GLB_BASE + GLB_SEAM_MISC_OFFSET);

	/* Reset UART signal swap */
	tmp = sys_read32(GLB_BASE + GLB_PARM_OFFSET);
	tmp = (tmp & GLB_UART_SWAP_SET_UMSK) | (0U << GLB_UART_SWAP_SET_POS);
	sys_write32(tmp, GLB_BASE + GLB_PARM_OFFSET);

	/* Clear all interrupt */
	p = (uint32_t *)(CLIC_HART0_ADDR + CLIC_INTIE);

	for (i = 0; i < (IRQn_LAST + 3) / 4; i++) {
		p[i] = 0;
	}

	p = (uint32_t *)(CLIC_HART0_ADDR + CLIC_INTIP);

	for (i = 0; i < (IRQn_LAST + 3) / 4; i++) {
		p[i] = 0;
	}

#if !DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(psram))
	/* If no PSRAM configured, reuse its GPIO registers (32-37) for external pins that have
	 * registers allocated to SF2 internal flash (23-28).
	 */
	sys_write32(GLB_CFG_GPIO_USE_PSRAM_IO_MSK, GLB_BASE + GLB_GPIO_USE_PSRAM__IO_OFFSET);
#endif
	sys_cache_data_flush_and_invd_all();
}
