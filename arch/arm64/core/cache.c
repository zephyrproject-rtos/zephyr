/* cache.c - d-cache support for AARCH64 CPUs */

/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief d-cache manipulation
 *
 * This module contains functions for manipulation of the d-cache.
 */

#include <cache.h>

#define	CTR_EL0_DMINLINE_SHIFT	16
#define	CTR_EL0_DMINLINE_MASK	GENMASK(19, 16)
#define	CTR_EL0_CWG_SHIFT	24
#define	CTR_EL0_CWG_MASK	GENMASK(27, 24)

/* clidr_el1 */
#define CLIDR_EL1_LOC_SHIFT		24
#define CLIDR_EL1_LOC_MASK		BIT_MASK(3)
#define CLIDR_EL1_CTYPE_SHIFT(level)	((level) * 3)
#define CLIDR_EL1_CTYPE_MASK		BIT_MASK(3)

/* ccsidr_el1 */
#define CCSIDR_EL1_LN_SZ_SHIFT		0
#define CCSIDR_EL1_LN_SZ_MASK		BIT_MASK(3)
#define CCSIDR_EL1_WAYS_SHIFT		3
#define CCSIDR_EL1_WAYS_MASK		BIT_MASK(10)
#define CCSIDR_EL1_SETS_SHIFT		13
#define CCSIDR_EL1_SETS_MASK		BIT_MASK(15)

#define dc_ops(op, val)							\
({									\
	__asm__ volatile ("dc " op ", %0" :: "r" (val) : "memory");	\
})

int arch_dcache_flush(void *addr, size_t size);
int arch_dcache_invd(void *addr, size_t size);

static size_t dcache_line_size;

int arch_dcache_range(void *addr, size_t size, int op)
{
	if (op == K_CACHE_INVD) {
		arch_dcache_invd(addr, size);
	} else if (op == K_CACHE_WB_INVD) {
		arch_dcache_flush(addr, size);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

size_t arch_dcache_line_size_get(void)
{
	uint64_t ctr_el0;
	uint32_t cwg;
	uint32_t dminline;

	if (dcache_line_size)
		return dcache_line_size;

	ctr_el0 = read_sysreg(CTR_EL0);

	cwg = (ctr_el0 & CTR_EL0_CWG_MASK) >> CTR_EL0_CWG_SHIFT;
	dminline = (ctr_el0 & CTR_EL0_DMINLINE_MASK) >>
		CTR_EL0_DMINLINE_SHIFT;

	dcache_line_size = cwg ? 4 << cwg : 4 << dminline;

	return dcache_line_size;
}

/*
 * operation for all data cache
 * ops:  K_CACHE_INVD: invalidate
 *	 K_CACHE_WB: clean
 *	 K_CACHE_WB_INVD: clean and invalidate
 */
int arch_dcache_all(int op)
{
	uint32_t clidr_el1, csselr_el1, ccsidr_el1;
	uint8_t loc, ctype, cache_level, line_size, way_pos;
	uint32_t max_ways, max_sets, dc_val, set, way;

	if (op != K_CACHE_INVD && op != K_CACHE_WB && op != K_CACHE_WB_INVD)
		return -ENOTSUP;

	clidr_el1 = read_clidr_el1();

	loc = (clidr_el1 >> CLIDR_EL1_LOC_SHIFT) & CLIDR_EL1_LOC_MASK;
	if (!loc)
		return 0;

	for (cache_level = 0; cache_level < loc; cache_level++) {
		ctype = (clidr_el1 >> CLIDR_EL1_CTYPE_SHIFT(cache_level))
				& CLIDR_EL1_CTYPE_MASK;
		/* No data cache, continue */
		if (ctype < 2)
			continue;

		/* select cache level */
		csselr_el1 = cache_level << 1;
		write_csselr_el1(csselr_el1);
		isb();

		ccsidr_el1 = read_ccsidr_el1();
		line_size = (ccsidr_el1 >> CCSIDR_EL1_LN_SZ_SHIFT
				& CCSIDR_EL1_LN_SZ_MASK) + 4;
		max_ways = (ccsidr_el1 >> CCSIDR_EL1_WAYS_SHIFT)
				& CCSIDR_EL1_WAYS_MASK;
		max_sets = (ccsidr_el1 >> CCSIDR_EL1_SETS_SHIFT)
				& CCSIDR_EL1_SETS_MASK;
		/* 32-log2(ways), bit position of way in DC operand */
		way_pos = __builtin_clz(max_ways);

		for (set = 0; set <= max_sets; set++) {
			for (way = 0; way <= max_ways; way++) {
				/* way number, aligned to pos in DC operand */
				dc_val = way << way_pos;
				/* cache level, aligned to pos in DC operand */
				dc_val |= csselr_el1;
				/* set number, aligned to pos in DC operand */
				dc_val |= set << line_size;

				if (op == K_CACHE_INVD) {
					dc_ops("isw", dc_val);
				} else if (op == K_CACHE_WB_INVD) {
					dc_ops("cisw", dc_val);
				} else if (op == K_CACHE_WB) {
					dc_ops("csw", dc_val);
				}
			}
		}
	}
	return 0;
}
