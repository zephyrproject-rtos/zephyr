/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv5 exception vector installation
 *
 * ARMv5 cores fetch exception vectors from either 0x00000000 or 0xFFFF0000
 * (SCTLR.V), never from a programmable base address: there is no VBAR. Unless
 * the image happens to be linked so that _vector_table already sits at the
 * vector base, the 64-byte position independent vector block therefore has to
 * be copied there at boot.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arm/arm9/lib_helpers.h>

extern void *_vector_table[];

void z_armv5_vector_table_init(void)
{
	uintptr_t base = VECTOR_BASE_LOW;

	__set_SCTLR(__get_SCTLR() & ~SCTLR_V_BIT);
	__ISB();

	if (base != (uintptr_t)_vector_table) {
		/*
		 * Copied a word at a time rather than with memcpy(): the low
		 * vector base is address 0, and the compiler is entitled to
		 * assume memcpy() is never handed a null pointer.
		 */
		volatile uint32_t *dst = (volatile uint32_t *)base;
		const uint32_t *src = (const uint32_t *)_vector_table;

		for (size_t i = 0; i < VECTOR_TABLE_SIZE / sizeof(uint32_t); i++) {
			dst[i] = src[i];
		}

		/*
		 * The copy went out through the data side. Clean it out to
		 * memory and drop the instruction cache so that the very
		 * first exception fetches the new vectors rather than
		 * whatever used to be at the vector base. This is done with
		 * the CP15 operations directly because it has to work
		 * regardless of CONFIG_CACHE_MANAGEMENT.
		 */
		for (uintptr_t addr = base; addr < base + VECTOR_TABLE_SIZE;
		     addr += CONFIG_DCACHE_LINE_SIZE) {
			MCR15(c7, 0, c10, 1, addr);
		}
		__DSB();
		MCR15(c7, 0, c5, 0, 0);
		__ISB();
	}
}
