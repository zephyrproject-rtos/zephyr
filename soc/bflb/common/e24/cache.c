/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <bflb_soc.h>
#include <l1c_reg.h>

void __ramfunc soc_bflb_cache_invalidate(void)
{
	uint32_t tmp;

	tmp = *((uint32_t *)(L1C_BASE + L1C_CONFIG_OFFSET));
	tmp |= 1 << L1C_BYPASS_POS;
	tmp &= ~(1 << L1C_CACHEABLE_POS);
	tmp &= ~L1C_WAY_DIS_MSK;
	tmp &= ~(1 << L1C_CNT_EN_POS);
	*((uint32_t *)(L1C_BASE + L1C_CONFIG_OFFSET)) = tmp;

	tmp &= ~(1 << L1C_INVALID_EN_POS);
	*((uint32_t *)(L1C_BASE + L1C_CONFIG_OFFSET)) = tmp;
	__asm__ volatile (".rept 4 ; nop ; .endr");

	tmp |= (1 << L1C_INVALID_EN_POS);
	*((uint32_t *)(L1C_BASE + L1C_CONFIG_OFFSET)) = tmp;
	__asm__ volatile (".rept 4 ; nop ; .endr");

	while ((*((uint32_t *)(L1C_BASE + L1C_CONFIG_OFFSET)) & L1C_INVALID_DONE_MSK) == 0) {
		__asm__ volatile (".rept 50 ; nop ; .endr");
	}

	tmp = *((uint32_t *)(L1C_BASE + L1C_CONFIG_OFFSET));
	tmp |= 1 << L1C_BYPASS_POS;
	*((uint32_t *)(L1C_BASE + L1C_CONFIG_OFFSET)) = tmp;

	tmp &= ~(1 << L1C_BYPASS_POS);
	tmp |= 1 << L1C_CNT_EN_POS;
	*((uint32_t *)(L1C_BASE + L1C_CONFIG_OFFSET)) = tmp;

	tmp &= ~L1C_WAY_DIS_MSK;
	*((uint32_t *)(L1C_BASE + L1C_CONFIG_OFFSET)) = tmp;

	tmp |= 1 << L1C_CACHEABLE_POS;
	*((uint32_t *)(L1C_BASE + L1C_CONFIG_OFFSET)) = tmp;

	__asm__ volatile (".rept 100 ; nop ; .endr");
}
