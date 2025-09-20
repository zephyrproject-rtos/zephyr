/*
 * Copyright (c) 2021 Andes Technology Corporation
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc_v5.h"

#include <zephyr/toolchain.h>

/* exports */
GTEXT(entry)

SECTION_FUNC(init, entry)
	/* Disable linker relaxation before GP register initialization. */
	.option push
	.option norelax

	/* The magic number 'EGIS' is located 4 bytes from the beginning of the binary header.
	 * Use the machine code directly to prevent the compiler from optimizing 'jump'
	 * instruction to different sizes */
	.word 0x01c0006f /* machine code: $pc + 28 bytes [j header_end] */
	.word 0x53494745 /* "EGIS" */
	.word CONFIG_SOC_EGIS_ET171_FIRMWARE_VERSION /* version */
	.word CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET /* flash start address */
	.word __egis_binary_size /* It's not included 64 bytes signature */
	.word 0 /* unused */
	.word 0 /* unused */

header_end:

#ifdef CONFIG_ICACHE
	/* Enable I cache with HW prefetcher. */
	li t0, (1 << 9) | (1 << 0)
	csrs NDS_MCACHE_CTL, t0
#endif

#ifdef CONFIG_DCACHE
	/* Enable D cache with HW prefetcher */
	li t0, (1 << 10) | (1 << 1)
	csrs NDS_MCACHE_CTL, t0
#endif

	/* Enable misaligned access and non-blocking load */
	li t0, (1 << 8) | (1 << 6)
	csrs NDS_MMISC_CTL, t0

	j __start

	.option pop
