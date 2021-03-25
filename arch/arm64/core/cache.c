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
