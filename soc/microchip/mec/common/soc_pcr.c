/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>

#include "soc_pcr.h"

/* Reset a peripheral block */
void soc_xec_pcr_reset_en(uint16_t enc_pcr_scr)
{
	mem_addr_t raddr = XEC_PCR_RST_EN_BASE + (MCHP_XEC_PCR_SCR_GET_IDX(enc_pcr_scr) * 4u);
	unsigned int irq_lock_val = irq_lock();

	sys_write32(XEC_PCR_RST_EN_UNLOCK_VAL, (mem_addr_t)XEC_PCR_RST_EN_LOCK_BASE);
	sys_set_bit(raddr, MCHP_XEC_PCR_SCR_GET_BITPOS(enc_pcr_scr));
	sys_write32(XEC_PCR_RST_EN_LOCK_VAL, XEC_PCR_RST_EN_LOCK_BASE);
	irq_unlock(irq_lock_val);
}

int soc_xec_pcr_cpu_clk_div_set(uint8_t cpu_clk_div)
{
	mm_reg_t pcr_base = XEC_PCR_BASE;
	uint32_t rval = XEC_CC_PCLK_CR_DIV_SET((uint32_t)cpu_clk_div);

	soc_mmcr_mask_set(pcr_base + XEC_CC_PCLK_CR_OFS, rval, XEC_CC_PCLK_CR_DIV_MSK);

	return 0;
}

int soc_xec_pcr_cpu_clk_div_get(uint8_t *cpu_clk_div)
{
	mm_reg_t pcr_base = XEC_PCR_BASE;

	if (cpu_clk_div == NULL) {
		return -EINVAL;
	}

	*cpu_clk_div = (uint8_t)XEC_CC_PCLK_CR_DIV_GET(sys_read32(pcr_base + XEC_CC_PCLK_CR_OFS));

	return 0;
}
