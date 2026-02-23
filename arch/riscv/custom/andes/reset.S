/*
 * Copyright (c) 2025 Andes Technology Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <andes_csr.h>
#include <zephyr/toolchain.h>
#include <zephyr/devicetree.h>
#include <zephyr/arch/cpu.h>

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

#if DT_HAS_COMPAT_STATUS_OKAY(andestech_ilm)
	/* Enable ILM */
	csrrs t0, NDS_MILMB, 0x1
#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_itcm), okay) && defined(CONFIG_XIP)
	/*
	 * If ILM ECC is enabled by default, temporarily disable ECC and clear
	 * the XIP relocation region to regenerate ECC codes.
	 *
	 * When XIP=n, the earlier boot stage or loader is responsible for
	 * initializing ILM and its ECC before being loaded.
	 */
	andi t0, t0, 0x6
	beqz t0, 1f
	csrci NDS_MILMB, 0x6
	/*
	 * Only the Zephyr ITCM region is cleared. If program executing from
	 * ILM, ILM/ECC are assumed initialized by earlier boot stages.
	 */
	la t1, __itcm_start
	la t2, __itcm_size
	add t2, t1, t2
2:
#ifdef CONFIG_64BIT
	sd x0, 0(t1)
#else
	sw x0, 0(t1)
#endif
	addi t1, t1, RV_REGSIZE
	blt t1, t2, 2b
	/* Restore the ECC settings. */
	csrs NDS_MILMB, t0
1:
#endif
#else
	/* Disable ILM to avoid relocation accessing overlaid DDR. */
	csrr t0, NDS_MICM_CFG
	li t1, 0xf8000
	and t1, t0, t1
	beqz t1, 1f
	csrc NDS_MILMB, 0x1
1:
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(andestech_dlm)
	/* Enable DLM. */
	csrrs t0, NDS_MDLMB, 0x1
#ifdef CONFIG_XIP
	/*
	 * If DLM ECC is enabled by default, temporarily disable ECC and clear
	 * the XIP relocation region to regenerate ECC codes.
	 *
	 * When XIP=n, the earlier boot stage or loader is responsible for
	 * initializing DLM and its ECC before being loaded.
	 */
	andi t0, t0, 0x6
	beqz t0, 1f
	csrci NDS_MDLMB, 0x6
	/*
	 * The entire DLM region is cleared to support Zephyr DTCM, data, bss,
	 * and stack usage.
	 */
	li t1, DT_REG_ADDR(DT_INST(0, andestech_dlm))
	li t2, DT_REG_SIZE(DT_INST(0, andestech_dlm))
	add t2, t1, t2
2:
#ifdef CONFIG_64BIT
	sd x0, 0(t1)
#else
	sw x0, 0(t1)
#endif
	addi t1, t1, RV_REGSIZE
	blt t1, t2, 2b
	/* Restore the ECC settings. */
	csrs NDS_MDLMB, t0
1:
#endif
#else
	/* Disable DLM to avoid relocation accessing overlaid DDR. */
	csrr t0, NDS_MDCM_CFG
	li t1, 0xf8000
	and t1, t0, t1
	beqz t1, 1f
	csrc NDS_MDLMB, 0x1
1:
#endif

	/* Enable misaligned access and non-blocking load */
	li t0, (1 << 8) | (1 << 6)
	csrs NDS_MMISC_CTL, t0

	/* Jump to __initialize */
	call __initialize
