/*
 * Copyright (c) 2021 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <soc.h>

/* exports */
GTEXT(entry)

SECTION_FUNC(init, entry)
	/* Disable linker relaxation before GP register initialization. */
	.option push
	.option norelax

#ifdef __nds_execit
	/* Initialize EXECIT table */
	la t0, _ITB_BASE_
	csrw uitb, t0
#endif

#ifdef CONFIG_CACHE_ENABLE
	/*
	 * Enable I/D cache with HW prefetcher,
	 * D-cache write-around (threshold: 4 cache lines),
	 * and CM (Coherence Manager).
	 */
	li t0, (0x3 << 13)
	csrc NDS_MCACHE_CTL, t0
	li t0, (1 << 19) | (1 << 13) | (1 << 10) | (1 << 9) | (0x3)
	csrs NDS_MCACHE_CTL, t0

	/* Check if CPU support CM or not. */
	csrr t0, NDS_MCACHE_CTL
	li t1, (1 << 19)
	and t0, t0, t1
	beqz t0, cache_enable_finish

	/* If CPU support CM, check if CM is enabled. */
	li t1, (1 << 20)
check_cm_enabled:
	csrr t0, NDS_MCACHE_CTL
	and t0, t0, t1
	beqz t0, check_cm_enabled

cache_enable_finish:
#endif

	/* Enable misaligned access and non-blocking load */
	li t0, (1 << 8) | (1 << 6)
	csrs NDS_MMISC_CTL, t0

	j __start

	.option pop
