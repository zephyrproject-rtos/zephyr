/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OpenRISC (or1k) ELF relocation type definitions and helpers for LLEXT.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_OPENRISC_ELF_H_
#define ZEPHYR_INCLUDE_ARCH_OPENRISC_ELF_H_

/**
 * @brief OpenRISC relocation types
 *
 * From the official OR1K ELF specification and binutils.
 * @cond ignore
 */

/* Basic relocations */
#define R_OR1K_NONE             0
#define R_OR1K_32               1
#define R_OR1K_16               2
#define R_OR1K_8                3
#define R_OR1K_LO_16_IN_INSN    4
#define R_OR1K_HI_16_IN_INSN    5
#define R_OR1K_INSN_REL_26      6
#define R_OR1K_GNU_VTENTRY      7
#define R_OR1K_GNU_VTINHERIT    8
#define R_OR1K_32_PCREL         9
#define R_OR1K_16_PCREL         10
#define R_OR1K_8_PCREL          11

/* GOT/PLT relocations */
#define R_OR1K_GOTPC_HI16       12
#define R_OR1K_GOTPC_LO16       13
#define R_OR1K_GOT16            14
#define R_OR1K_PLT26            15
#define R_OR1K_GOTOFF_HI16      16
#define R_OR1K_GOTOFF_LO16      17

/* Dynamic relocations */
#define R_OR1K_COPY             18
#define R_OR1K_GLOB_DAT         19
#define R_OR1K_JMP_SLOT         20
#define R_OR1K_RELATIVE         21

/* TLS relocations */
#define R_OR1K_TLS_GD_HI16      22
#define R_OR1K_TLS_GD_LO16      23
#define R_OR1K_TLS_LDM_HI16     24
#define R_OR1K_TLS_LDM_LO16     25
#define R_OR1K_TLS_LDO_HI16     26
#define R_OR1K_TLS_LDO_LO16     27
#define R_OR1K_TLS_IE_HI16      28
#define R_OR1K_TLS_IE_LO16      29
#define R_OR1K_TLS_LE_HI16      30
#define R_OR1K_TLS_LE_LO16      31
#define R_OR1K_TLS_TPOFF        32
#define R_OR1K_TLS_DTPOFF       33
#define R_OR1K_TLS_DTPMOD       34

/* Adjusted high / split-immediate relocations */
#define R_OR1K_AHI16            35
#define R_OR1K_GOTOFF_AHI16     36
#define R_OR1K_TLS_IE_AHI16     37
#define R_OR1K_TLS_LE_AHI16     38
#define R_OR1K_SLO16            39
#define R_OR1K_GOTOFF_SLO16     40
#define R_OR1K_TLS_LE_SLO16     41

/* Page-relative relocations */
#define R_OR1K_PCREL_PG21       42
#define R_OR1K_GOT_PG21         43
#define R_OR1K_TLS_GD_PG21      44
#define R_OR1K_TLS_LDM_PG21     45
#define R_OR1K_TLS_IE_PG21      46
#define R_OR1K_LO13             47
#define R_OR1K_GOT_LO13         48
#define R_OR1K_TLS_GD_LO13      49
#define R_OR1K_TLS_LDM_LO13     50
#define R_OR1K_TLS_IE_LO13      51
#define R_OR1K_SLO13            52
#define R_OR1K_PLTA26           53
#define R_OR1K_GOT_AHI16        54

/** @endcond */

/**
 * @brief OR1K instruction field masks
 *
 * OR1K instructions are fixed 32-bit, big-endian.
 * Immediate fields are located in the low bits of the instruction word.
 */

/** Mask for the 26-bit jump/branch target field (bits [25:0]) */
#define R_OR1K_INSN_REL_26_MASK   0x03ffffffU

/** Mask for the 16-bit immediate field (bits [15:0]) - used by l.ori, l.addi, l.movhi, etc. */
#define R_OR1K_IMM16_MASK         0x0000ffffU

/** Mask for the 21-bit page-relative field (bits [20:0]) - used by l.adrp */
#define R_OR1K_PG21_MASK          0x001fffffU

/**
 * @brief Split-immediate mask for OR1K store instructions.
 *
 * Store instructions (l.sw, l.sb, l.sh) encode a 16-bit signed immediate
 * split across two non-contiguous fields:
 *   - Bits [15:11] of the immediate → instruction bits [25:21]
 *   - Bits [10:0] of the immediate → instruction bits [10:0]
 *
 * The combined field mask is 0x03E007FF.
 */
#define R_OR1K_SLO_FIELD_MASK     0x03e007ffU

/**
 * @brief Pack a 16-bit immediate into the OR1K split-immediate store format.
 *
 * @param insn  The existing 32-bit instruction word
 * @param imm   The 16-bit immediate value to pack
 * @return      The instruction word with the immediate inserted
 */
#define R_OR1K_SLO_PACK(insn, imm) \
	(((insn) & ~R_OR1K_SLO_FIELD_MASK) | \
	 (((uint32_t)(imm) & 0xf800U) << 10) | \
	 ((uint32_t)(imm) & 0x07ffU))

/** OR1K page size for PCREL_PG21 relocations (8 KB = 2^13) */
#define R_OR1K_PAGE_SIZE          8192U
/** OR1K page mask for PCREL_PG21 relocations (8 KB aligned) */
#define R_OR1K_PAGE_MASK          (~(R_OR1K_PAGE_SIZE - 1U))

#endif /* ZEPHYR_INCLUDE_ARCH_OPENRISC_ELF_H_ */
