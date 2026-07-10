/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/offsets.h>

#include <xtensa/config/tie.h>
#include <xtensa/config/tie-asm.h>

#if defined(CONFIG_XTENSA_EAGER_HIFI_SHARING)
/*
 * Load/save HiFi registers from/to the hifi buffer in the BSA.
 * Round the buffer address up to XCHAL_CP1_SA_ALIGN bytes.
 *
 * Upon entry:
 *   A0 - return address (do not modify)
 *   A1 - address of BSA (do not modify)
 *   A2 - available for use
 *   A3 - available for use
 *
 * HiFi3 (cavs2.5/TGL) uses explicit instruction sequences instead of the
 * xchal_cp1_load/store GAS macros because ae_lalign64.i and ae_salign64.i
 * are FLIX bundles that the LLVM integrated assembler mis-encodes for the
 * intel_tgl_adsp core (they are absent from tgl_enc.inc).  The correct 8-byte
 * bundle encodings are emitted as .byte sequences, verified against the
 * xtensa-intel_tgl_adsp_zephyr-elf GNU assembler.
 *
 * Save-area layout for HiFi3 (XCHAL_CP1_SA_SIZE=184, XCHAL_CP1_SA_ALIGN=8):
 *   +0:   AE_OVF_SAR, AE_BITHEAD, AE_TS_FTS_BU_BP, AE_CW_SD_NO,
 *         AE_CBEGIN0, AE_CEND0    (6 x 4 = 24 bytes)
 *   +24:  aed0-aed4               (5 x 8 = 40 bytes)
 *   +64:  aed5-aed12              (8 x 8 = 64 bytes)
 *   +128: aed13-aed15             (3 x 8 = 24 bytes)
 *   +152: u0-u3                   (4 x 8 = 32 bytes)
 */

#if defined(CONFIG_XTENSA_CPU_HAS_HIFI3)

.global _xtensa_hifi_load
.align 4
_xtensa_hifi_load:
	addi a2, a1, (___xtensa_irq_bsa_t_hifi_OFFSET + XCHAL_CP1_SA_ALIGN - 1)
	movi a3, ~(XCHAL_CP1_SA_ALIGN - 1)
	and a2, a2, a3
	/* Scalar AE state registers */
	l32i    a3, a2, 0
	wur.ae_ovf_sar a3
	l32i    a3, a2, 4
	wur.ae_bithead a3
	l32i    a3, a2, 8
	wur.ae_ts_fts_bu_bp a3
	l32i    a3, a2, 12
	wur.ae_cw_sd_no a3
	l32i    a3, a2, 16
	wur.ae_cbegin0 a3
	l32i    a3, a2, 20
	wur.ae_cend0 a3
	/* aed0..aed4 */
	ae_l64.i aed0, a2, 24
	ae_l64.i aed1, a2, 32
	ae_l64.i aed2, a2, 40
	ae_l64.i aed3, a2, 48
	ae_l64.i aed4, a2, 56
	addi    a2, a2, 64
	/* aed5..aed12 */
	ae_l64.i aed5, a2, 0
	ae_l64.i aed6, a2, 8
	ae_l64.i aed7, a2, 16
	ae_l64.i aed8, a2, 24
	ae_l64.i aed9, a2, 32
	ae_l64.i aed10, a2, 40
	ae_l64.i aed11, a2, 48
	ae_l64.i aed12, a2, 56
	addi    a2, a2, 64
	/* aed13..aed15 */
	ae_l64.i aed13, a2, 0
	ae_l64.i aed14, a2, 8
	ae_l64.i aed15, a2, 16
	addi    a2, a2, 24
	/* u0..u3 (ae_lalign64.i) skipped: these alignment scratch registers are
	 * only used by unaligned HiFi access (ae_l64_x etc.).  LLVM-compiled
	 * SOF code uses only aligned ae_l64.i/ae_s64.i, so u0-u3 never carry
	 * live state across a context switch. */
	ret

.global _xtensa_hifi_save
.align 4
_xtensa_hifi_save:
	addi a2, a1, (___xtensa_irq_bsa_t_hifi_OFFSET + XCHAL_CP1_SA_ALIGN - 1)
	movi a3, ~(XCHAL_CP1_SA_ALIGN - 1)
	and a2, a2, a3
	/* Scalar AE state registers */
	rur.ae_ovf_sar a3
	s32i.n  a3, a2, 0
	rur.ae_bithead a3
	s32i.n  a3, a2, 4
	rur.ae_ts_fts_bu_bp a3
	s32i.n  a3, a2, 8
	rur.ae_cw_sd_no a3
	s32i.n  a3, a2, 12
	rur.ae_cbegin0 a3
	s32i.n  a3, a2, 16
	rur.ae_cend0 a3
	s32i.n  a3, a2, 20
	/* aed0..aed4 */
	ae_s64.i aed0, a2, 24
	ae_s64.i aed1, a2, 32
	ae_s64.i aed2, a2, 40
	ae_s64.i aed3, a2, 48
	ae_s64.i aed4, a2, 56
	addi    a2, a2, 64
	/* aed5..aed12 */
	ae_s64.i aed5, a2, 0
	ae_s64.i aed6, a2, 8
	ae_s64.i aed7, a2, 16
	ae_s64.i aed8, a2, 24
	ae_s64.i aed9, a2, 32
	ae_s64.i aed10, a2, 40
	ae_s64.i aed11, a2, 48
	ae_s64.i aed12, a2, 56
	addi    a2, a2, 64
	/* aed13..aed15 */
	ae_s64.i aed13, a2, 0
	ae_s64.i aed14, a2, 8
	ae_s64.i aed15, a2, 16
	addi    a2, a2, 24
	/* u0..u3 (ae_salign64.i) skipped — see _xtensa_hifi_load comment. */
	ret

#else /* !CONFIG_XTENSA_CPU_HAS_HIFI3 — use generic macro path (e.g. HiFi4) */

.global _xtensa_hifi_load
.align 4
_xtensa_hifi_load:
	addi a2, a1, (___xtensa_irq_bsa_t_hifi_OFFSET + XCHAL_CP1_SA_ALIGN - 1)
	movi a3, ~(XCHAL_CP1_SA_ALIGN - 1)
	and a2, a2, a3
	xchal_cp1_load a2 a3 a3 a3 a3
	ret

.global _xtensa_hifi_save
.align 4
_xtensa_hifi_save:
	addi a2, a1, (___xtensa_irq_bsa_t_hifi_OFFSET + XCHAL_CP1_SA_ALIGN - 1)
	movi a3, ~(XCHAL_CP1_SA_ALIGN - 1)
	and a2, a2, a3
	xchal_cp1_store a2 a3 a3 a3 a3
	ret

#endif /* CONFIG_XTENSA_CPU_HAS_HIFI3 */
#elif defined(CONFIG_XTENSA_LAZY_HIFI_SHARING)
/*
 * Load the HiFi registers from the HiFi buffer in the k_thread structure.
 */
.global xtensa_lazy_hifi_load
.align 4
xtensa_lazy_hifi_load:
	entry a1, 32
	/* Spill registers onto stack */
	call8 xthal_window_spill

	/* A2 should be address of hifi storage; A3 is scratch */
	xchal_cp1_load a2 a3 a3 a3 a3
	retw

/*
 * Save the HiFi registers to the HiFi buffer in the k_thread structure.
 */
.global xtensa_lazy_hifi_save
.align 4
xtensa_lazy_hifi_save:
	entry a1, 32
	/* Spill registers onto stack */
	call8 xthal_window_spill

	/* A2 should be address of hifi storage; A3 is scratch */
	xchal_cp1_store a2 a3 a3 a3 a3  /* Only A2 and A3 are used by macro */
	retw
#endif
