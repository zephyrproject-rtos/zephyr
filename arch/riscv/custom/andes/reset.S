/*
 * Copyright (c) 2025 Andes Technology Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <andes_csr.h>
#include <zephyr/toolchain.h>
#include <zephyr/devicetree.h>
#include <zephyr/arch/cpu.h>

#include "../../core/asm_macros.inc"

/* Zero the memory from \start to \end. */
	.macro nds_zero_region start, end
	j .Lcheck\@
.Lloop\@:
	sr x0, 0(\start)
	addi \start, \start, RV_REGSIZE
.Lcheck\@:
	bltu \start, \end, .Lloop\@
	.endm

/*
 * If the ECC of the local memory is enabled, temporarily disable it and
 * clear the memory from \start to \end to regenerate the ECC codes.
 */
	.macro nds_lm_ecc_init csr, eccen, start, end
	csrr t0, \csr
	andi t0, t0, \eccen
	beqz t0, .Ldone\@
	csrci \csr, \eccen
	nds_zero_region \start, \end
	/* Restore the ECC settings. */
	csrs \csr, t0
.Ldone\@:
	.endm

/* Disable the local memory if \cfg reports that it is present. */
	.macro nds_lm_disable cfg, size_mask, csr, en
	csrr t0, \cfg
	li t1, \size_mask
	and t1, t0, t1
	beqz t1, .Ldone\@
	csrci \csr, \en
.Ldone\@:
	.endm

/* exports */
GTEXT(__reset)

/* imports */
GTEXT(__initialize)

SECTION_FUNC(reset, __reset)

#ifdef CONFIG_RISCV_CUSTOM_CSR_ANDES_NMI
	/*
	 * mnvec is read-only and reads back as reset_vector, so NMIs enter
	 * here too. Reset leaves mcause and mepc at 0; an NMI sets mcause to
	 * exactly 1, interrupt bit clear unlike the encoding the spec advises.
	 */
	csrr t0, mcause
	li   t1, 0x1
	bne  t0, t1, andes_cold_boot
	csrr t1, mepc
	beqz t1, andes_cold_boot
	tail _andes_nmi_entry
andes_cold_boot:
#endif

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
	csrsi NDS_MILMB, NDS_MILMB_IEN
#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_itcm)) && defined(CONFIG_XIP) && \
	!DT_SAME_NODE(DT_CHOSEN(zephyr_itcm), DT_CHOSEN(zephyr_flash))
	/*
	 * When XIP=n, or when the program runs from ILM, the earlier boot
	 * stage or loader is responsible for initializing ILM and its ECC.
	 *
	 * Only the Zephyr ITCM sections, from the start of ITCM to
	 * __itcm_end, are cleared.
	 */
	li t1, DT_REG_ADDR(DT_CHOSEN(zephyr_itcm))
	la t2, __itcm_end
	nds_lm_ecc_init NDS_MILMB, NDS_MILMB_ECCEN, t1, t2
#endif
#else
	/* Disable ILM to avoid relocation accessing overlaid DDR. */
	nds_lm_disable NDS_MICM_CFG, NDS_MICM_CFG_ILMSZ, NDS_MILMB, NDS_MILMB_IEN
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(andestech_dlm)
	/* Enable DLM. */
	csrsi NDS_MDLMB, NDS_MDLMB_DEN
#ifdef CONFIG_XIP
	/*
	 * When XIP=n, the earlier boot stage or loader is responsible for
	 * initializing DLM and its ECC before being loaded.
	 *
	 * The entire DLM region is cleared to support Zephyr DTCM, data, bss,
	 * and stack usage.
	 */
	li t1, DT_REG_ADDR(DT_INST(0, andestech_dlm))
	li t2, DT_REG_SIZE(DT_INST(0, andestech_dlm))
	add t2, t1, t2
	nds_lm_ecc_init NDS_MDLMB, NDS_MDLMB_ECCEN, t1, t2
#endif
#else
	/* Disable DLM to avoid relocation accessing overlaid DDR. */
	nds_lm_disable NDS_MDCM_CFG, NDS_MDCM_CFG_DLMSZ, NDS_MDLMB, NDS_MDLMB_DEN
#endif

	/* Enable misaligned access and non-blocking load */
	li t0, (1 << 8) | (1 << 6)
	csrs NDS_MMISC_CTL, t0

	/* Jump to __initialize */
	call __initialize

#ifdef CONFIG_RISCV_CUSTOM_CSR_ANDES_NMI
WTEXT(_andes_nmi_entry)
SECTION_FUNC(text, _andes_nmi_entry)
	/*
	 * Placeholder so the option links on its own; SoCs enabling NMIs are
	 * expected to provide a real handler. Nothing can be assumed about the
	 * NMI stack here, so it only parks the core.
	 */
1:	wfi
	j 1b
#endif
