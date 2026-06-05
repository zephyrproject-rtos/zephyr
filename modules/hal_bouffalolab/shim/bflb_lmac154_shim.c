/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdint.h>

#include <zephyr/kernel.h>

#include <bflb_soc.h>
#include <glb_reg.h>

#define SWRST_BITS_PER_REG 32U

/* Pulses one GLB_SWRST_CFGx bit; referenced by the lmac154 blob. */
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

	tmp = sys_read32(reg_addr);
	tmp &= ~(1U << bit);
	sys_write32(tmp, reg_addr);

	tmp = sys_read32(reg_addr);
	tmp |= (1U << bit);
	sys_write32(tmp, reg_addr);

	tmp = sys_read32(reg_addr);
	tmp &= ~(1U << bit);
	sys_write32(tmp, reg_addr);
}

/* Blob memcpy helpers; arch_memcpy4 copies n 32-bit words. */
void *arch_memcpy(void *dst, const void *src, uint32_t n)
{
	return memcpy(dst, src, n);
}

uint32_t *arch_memcpy4(uint32_t *dst, const uint32_t *src, uint32_t n)
{
	const uint32_t *p = src;
	uint32_t *q = dst;

	while (n--) {
		*q++ = *p++;
	}

	return dst;
}
