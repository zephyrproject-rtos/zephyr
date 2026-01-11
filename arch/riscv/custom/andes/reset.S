/*
 * Copyright (c) 2025 Andes Technology Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <andes_csr.h>
#include <zephyr/toolchain.h>
#include <zephyr/devicetree.h>

/* exports */
GTEXT(__reset)

/* imports */
GTEXT(__initialize)

SECTION_FUNC(reset, __reset)

#ifdef CONFIG_RISCV_CUSTOM_CSR_ANDES_EXECIT
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
	 * Enable D cache with HW prefetcher and D-cache write-around
	 * (threshold: 4 cache lines).
	 */
	li t0, (0x3 << 13)
	csrc NDS_MCACHE_CTL, t0
	li t0, (1 << 13) | (1 << 10) | (1 << 1)
	csrs NDS_MCACHE_CTL, t0

#if DT_HAS_COMPAT_STATUS_OKAY(andestech_l2c)
	/* Enable D-cache CM (Coherence Manager). */
	li t0, (1 << 19)
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
#endif /* DT_HAS_COMPAT_STATUS_OKAY(andestech_l2c) */
#endif

	/* Enable misaligned access and non-blocking load */
	li t0, (1 << 8) | (1 << 6)
	csrs NDS_MMISC_CTL, t0

	/* Jump to __initialize */
	call __initialize
