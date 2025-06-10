/*
 * Copyright (c) 2021 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc_v5.h"

#include <zephyr/toolchain.h>

#if defined(CONFIG_FLASH_LOAD_OFFSET)
#define FLASH_LOAD_OFFSET CONFIG_FLASH_LOAD_OFFSET
#else
#define FLASH_LOAD_OFFSET 0
#endif

#if defined(CONFIG_ROM_END_OFFSET)
#define ROM_END_OFFSET CONFIG_ROM_END_OFFSET
#else
#define ROM_END_OFFSET 0
#endif

/* exports */
GTEXT(entry)

SECTION_FUNC(init, entry)
	/* Disable linker relaxation before GP register initialization. */
	.option push
	.option norelax

#ifdef CONFIG_SOC_EGIS_ET171
#if CONFIG_FLASH_LOAD_SIZE < 1
#error Define CONFIG_FLASH_LOAD_SIZE
#endif
	j header_end
	j header_end /* @TODO: use to align. ".align" 2 doesn't work */
	.word 0x53494745 /* "EGIS" */
	.word 0xaaaaaaaa /* version */
	.word CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET /* address */
	.word CONFIG_FLASH_LOAD_SIZE - ROM_END_OFFSET - 64 /* 64 bytes for signature */
	.word 0 /* unused */
	.word 0 /* unused */

header_end:
#endif

#ifdef CONFIG_SOC_ANDES_V5_EXECIT
	/* Initialize EXECIT table */
	la t0, _ITB_BASE_
	csrw NDS_UITB, t0
#endif

#ifdef CONFIG_ICACHE
	/* Enable I cache with HW prefetcher. */
	li t0, (1 << 9) | (1 << 0)
	csrs NDS_MCACHE_CTL, t0
#endif

#ifdef CONFIG_DCACHE
	/*
	 * Enable D cache with HW prefetcher, D-cache write-around
	 * (threshold: 4 cache lines), and CM (Coherence Manager).
	 */
	li t0, (0x3 << 13)
	csrc NDS_MCACHE_CTL, t0
	li t0, (1 << 19) | (1 << 13) | (1 << 10) | (1 << 1)
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
