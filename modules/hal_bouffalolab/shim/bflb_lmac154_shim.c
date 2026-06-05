/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdint.h>

#include <bflb_soc.h>
#include <glb_reg.h>

#define SWRST_BITS_PER_REG 32U
#define REG32(addr)        (*(volatile uint32_t *)(uintptr_t)(addr))

void GLB_AHB_MCU_Software_Reset(uint8_t swrst)
{
	uint32_t reg_addr;
	uint32_t bit;
	uint32_t tmp;

	if (swrst < SWRST_BITS_PER_REG) {
		bit = swrst;
		reg_addr = GLB_BASE + GLB_SWRST_CFG0_OFFSET;
	} else if (swrst < (2U * SWRST_BITS_PER_REG)) {
		bit = swrst - SWRST_BITS_PER_REG;
		reg_addr = GLB_BASE + GLB_SWRST_CFG1_OFFSET;
	} else {
		bit = swrst - (2U * SWRST_BITS_PER_REG);
		reg_addr = GLB_BASE + GLB_SWRST_CFG2_OFFSET;
	}

	tmp = REG32(reg_addr);
	tmp &= ~(1U << bit);
	REG32(reg_addr) = tmp;

	tmp = REG32(reg_addr);
	tmp |= (1U << bit);
	REG32(reg_addr) = tmp;

	tmp = REG32(reg_addr);
	tmp &= ~(1U << bit);
	REG32(reg_addr) = tmp;
}

void *arch_memcpy(void *dst, const void *src, uint32_t n)
{
	return memcpy(dst, src, n);
}

void *arch_memcpy4(void *dst, const void *src, uint32_t n)
{
	return memcpy(dst, src, n);
}
