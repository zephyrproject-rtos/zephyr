/**
 * @file
 * @brief RISCV-Specific constants for ELF binaries.
 *
 * References can be found here:
 * https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-elf.adoc
 */
/*
 * Copyright (c) 2024 CISPA Helmholtz Center for Information Security gGmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_ARCH_RISCV_ELF_H
#define ZEPHYR_ARCH_RISCV_ELF_H

#include <stdint.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Relocation names for RISCV-specific relocations
 * @cond ignore
 */

#define R_RISCV_NONE              0
#define R_RISCV_32                1
#define R_RISCV_64                2
#define R_RISCV_RELATIVE          3
#define R_RISCV_COPY              4
#define R_RISCV_JUMP_SLOT         5
#define R_RISCV_TLS_DTPMOD32      6
#define R_RISCV_TLS_DTPMOD64      7
#define R_RISCV_TLS_DTPREL32      8
#define R_RISCV_TLS_DTPREL64      9
#define R_RISCV_TLS_TPREL32       10
#define R_RISCV_TLS_TPREL64       11
#define R_RISCV_TLSDESC           12
/* 13-15 reserved */
#define R_RISCV_BRANCH            16
#define R_RISCV_JAL               17
#define R_RISCV_CALL              18
#define R_RISCV_CALL_PLT          19
#define R_RISCV_GOT_HI20          20
#define R_RISCV_TLS_GOT_HI20      21
#define R_RISCV_TLS_GD_HI20       22
#define R_RISCV_PCREL_HI20        23
#define R_RISCV_PCREL_LO12_I      24
#define R_RISCV_PCREL_LO12_S      25
#define R_RISCV_HI20              26
#define R_RISCV_LO12_I            27
#define R_RISCV_LO12_S            28
#define R_RISCV_TPREL_HI20        29
#define R_RISCV_TPREL_LO12_I      30
#define R_RISCV_TPREL_LO12_S      31
#define R_RISCV_TPREL_ADD         32
#define R_RISCV_ADD8              33
#define R_RISCV_ADD16             34
#define R_RISCV_ADD32             35
#define R_RISCV_ADD64             36
#define R_RISCV_SUB8              37
#define R_RISCV_SUB16             38
#define R_RISCV_SUB32             39
#define R_RISCV_SUB64             40
#define R_RISCV_GOT32_PCREL       41
/* 42 reserved */
#define R_RISCV_ALIGN             43
/* next two refer to compressed instructions */
#define R_RISCV_RVC_BRANCH        44
#define R_RISCV_RVC_JUMP          45
/* 46-50 reserved */
#define R_RISCV_RELAX             51
#define R_RISCV_SUB6              52
#define R_RISCV_SET6              53
#define R_RISCV_SET8              54
#define R_RISCV_SET16             55
#define R_RISCV_SET32             56
#define R_RISCV_32_PCREL          57
#define R_RISCV_IRELATIVE         58
#define R_RISCV_PLT32             59
#define R_RISCV_SET_ULEB128       60
#define R_RISCV_SUB_ULEB128       61
#define R_RISCV_TLSDESC_HI20      62
#define R_RISCV_TLSDESC_LOAD_LO12 63
#define R_RISCV_TLSDESC_ADD_LO12  64
#define R_RISCV_TLSDESC_CALL      65
/* 66-190 reserved */
#define R_RISCV_VENDOR            191
/* 192-255 reserved */
/** @endcond */

/**
 * "wordclass" from RISC-V specification
 * @cond ignore
 */
#if defined(CONFIG_64BIT)
typedef uint64_t r_riscv_wordclass_t;
#else
typedef uint32_t r_riscv_wordclass_t;
#endif
/** @endcond */

/** @brief  Extract bit from immediate
 *
 * @param imm8 immediate value (usually upper 20 or lower 12 bit)
 * @param bit which bit to extract
 */
#define R_RISCV_IMM8_GET_BIT(imm8, bit) (((imm8) & BIT(bit)) >> (bit))

/** @brief  Generate mask for immediate in B-type RISC-V instruction
 *
 * @param imm8 immediate value, lower 12 bits used;
 *             due to alignment requirements, imm8[0] is implicitly 0
 *
 */
#define R_RISCV_BTYPE_IMM8_MASK(imm8)                                                              \
	((R_RISCV_IMM8_GET_BIT(imm8, 12) << 31) | (R_RISCV_IMM8_GET_BIT(imm8, 10) << 30) |         \
	 (R_RISCV_IMM8_GET_BIT(imm8, 9) << 29) | (R_RISCV_IMM8_GET_BIT(imm8, 8) << 28) |           \
	 (R_RISCV_IMM8_GET_BIT(imm8, 7) << 27) | (R_RISCV_IMM8_GET_BIT(imm8, 6) << 26) |           \
	 (R_RISCV_IMM8_GET_BIT(imm8, 5) << 25) | (R_RISCV_IMM8_GET_BIT(imm8, 4) << 11) |           \
	 (R_RISCV_IMM8_GET_BIT(imm8, 3) << 10) | (R_RISCV_IMM8_GET_BIT(imm8, 2) << 9) |            \
	 (R_RISCV_IMM8_GET_BIT(imm8, 1) << 8) | (R_RISCV_IMM8_GET_BIT(imm8, 11) << 7))

/** @brief  Generate mask for immediate in J-type RISC-V instruction
 *
 * @param imm8 immediate value, lower 21 bits used;
 *             due to alignment requirements, imm8[0] is implicitly 0
 *
 */
#define R_RISCV_JTYPE_IMM8_MASK(imm8)                                                              \
	((R_RISCV_IMM8_GET_BIT(imm8, 20) << 31) | (R_RISCV_IMM8_GET_BIT(imm8, 10) << 30) |         \
	 (R_RISCV_IMM8_GET_BIT(imm8, 9) << 29) | (R_RISCV_IMM8_GET_BIT(imm8, 8) << 28) |           \
	 (R_RISCV_IMM8_GET_BIT(imm8, 7) << 27) | (R_RISCV_IMM8_GET_BIT(imm8, 6) << 26) |           \
	 (R_RISCV_IMM8_GET_BIT(imm8, 5) << 25) | (R_RISCV_IMM8_GET_BIT(imm8, 4) << 24) |           \
	 (R_RISCV_IMM8_GET_BIT(imm8, 3) << 23) | (R_RISCV_IMM8_GET_BIT(imm8, 2) << 22) |           \
	 (R_RISCV_IMM8_GET_BIT(imm8, 1) << 21) | (R_RISCV_IMM8_GET_BIT(imm8, 11) << 20) |          \
	 (R_RISCV_IMM8_GET_BIT(imm8, 19) << 19) | (R_RISCV_IMM8_GET_BIT(imm8, 18) << 18) |         \
	 (R_RISCV_IMM8_GET_BIT(imm8, 17) << 17) | (R_RISCV_IMM8_GET_BIT(imm8, 16) << 16) |         \
	 (R_RISCV_IMM8_GET_BIT(imm8, 15) << 15) | (R_RISCV_IMM8_GET_BIT(imm8, 14) << 14) |         \
	 (R_RISCV_IMM8_GET_BIT(imm8, 13) << 13) | (R_RISCV_IMM8_GET_BIT(imm8, 12) << 12))

/** @brief  Generate mask for immediate in S-type RISC-V instruction
 *
 * @param imm8 immediate value, lower 12 bits used
 *
 */
#define R_RISCV_STYPE_IMM8_MASK(imm8)                                                              \
	((R_RISCV_IMM8_GET_BIT(imm8, 11) << 31) | (R_RISCV_IMM8_GET_BIT(imm8, 10) << 30) |         \
	 (R_RISCV_IMM8_GET_BIT(imm8, 9) << 29) | (R_RISCV_IMM8_GET_BIT(imm8, 8) << 28) |           \
	 (R_RISCV_IMM8_GET_BIT(imm8, 7) << 27) | (R_RISCV_IMM8_GET_BIT(imm8, 6) << 26) |           \
	 (R_RISCV_IMM8_GET_BIT(imm8, 5) << 25) | (R_RISCV_IMM8_GET_BIT(imm8, 4) << 11) |           \
	 (R_RISCV_IMM8_GET_BIT(imm8, 3) << 10) | (R_RISCV_IMM8_GET_BIT(imm8, 2) << 9) |            \
	 (R_RISCV_IMM8_GET_BIT(imm8, 1) << 8) | (R_RISCV_IMM8_GET_BIT(imm8, 0) << 7))

/** @brief  Generate mask for immediate in compressed J-type RISC-V instruction
 *
 * @param imm8 immediate value, lower 12 bits used;
 *             due to alignment requirements, imm8[0] is implicitly 0
 *
 */
#define R_RISCV_CJTYPE_IMM8_MASK(imm8)                                                             \
	((R_RISCV_IMM8_GET_BIT(imm8, 11) << 12) | (R_RISCV_IMM8_GET_BIT(imm8, 4) << 11) |          \
	 (R_RISCV_IMM8_GET_BIT(imm8, 9) << 10) | (R_RISCV_IMM8_GET_BIT(imm8, 8) << 9) |            \
	 (R_RISCV_IMM8_GET_BIT(imm8, 10) << 8) | (R_RISCV_IMM8_GET_BIT(imm8, 6) << 7) |            \
	 (R_RISCV_IMM8_GET_BIT(imm8, 7) << 6) | (R_RISCV_IMM8_GET_BIT(imm8, 3) << 5) |             \
	 (R_RISCV_IMM8_GET_BIT(imm8, 2) << 4) | (R_RISCV_IMM8_GET_BIT(imm8, 1) << 3) |             \
	 (R_RISCV_IMM8_GET_BIT(imm8, 5) << 2))

/** @brief  Generate mask for immediate in compressed B-type RISC-V instruction
 *
 * @param imm8 immediate value, lower 9 bits used;
 *             due to alignment requirements, imm8[0] is implicitly 0
 *
 */
#define R_RISCV_CBTYPE_IMM8_MASK(imm8)                                                             \
	((R_RISCV_IMM8_GET_BIT(imm8, 8) << 12) | (R_RISCV_IMM8_GET_BIT(imm8, 4) << 11) |           \
	 (R_RISCV_IMM8_GET_BIT(imm8, 3) << 10) | (R_RISCV_IMM8_GET_BIT(imm8, 7) << 6) |            \
	 (R_RISCV_IMM8_GET_BIT(imm8, 6) << 5) | (R_RISCV_IMM8_GET_BIT(imm8, 2) << 4) |             \
	 (R_RISCV_IMM8_GET_BIT(imm8, 1) << 3) | (R_RISCV_IMM8_GET_BIT(imm8, 5) << 2))

/** @brief Clear immediate bits in B-type instruction.
 *
 * @param operand Address of RISC-V instruction, B-type
 *
 */
#define R_RISCV_CLEAR_BTYPE_IMM8(operand) ((operand) & ~R_RISCV_BTYPE_IMM8_MASK((uint32_t) -1))

/** @brief Overwrite immediate in B-type instruction
 *
 * @param operand Address of RISC-V instruction, B-type
 * @param imm8    New immediate
 *
 */
#define R_RISCV_SET_BTYPE_IMM8(operand, imm8)                                                      \
	((R_RISCV_CLEAR_BTYPE_IMM8(operand)) | R_RISCV_BTYPE_IMM8_MASK(imm8))

/** @brief Clear immediate bits in J-type instruction.
 *
 * @param operand Address of RISC-V instruction, J-type
 *
 */
#define R_RISCV_CLEAR_JTYPE_IMM8(operand) ((operand) & ~R_RISCV_JTYPE_IMM8_MASK((uint32_t) -1))

/** @brief Overwrite immediate in J-type instruction
 *
 * @param operand Address of RISC-V instruction, J-type
 * @param imm8    New immediate
 *
 */
#define R_RISCV_SET_JTYPE_IMM8(operand, imm8)                                                      \
	((R_RISCV_CLEAR_JTYPE_IMM8(operand)) | R_RISCV_JTYPE_IMM8_MASK(imm8))

/** @brief Clear immediate bits in S-type instruction.
 *
 * @param operand Address of RISC-V instruction, S-type
 *
 */
#define R_RISCV_CLEAR_STYPE_IMM8(operand) ((operand) & ~R_RISCV_STYPE_IMM8_MASK((uint32_t) -1))

/** @brief Overwrite immediate in S-type instruction
 *
 * @param operand Address of RISC-V instruction, S-type
 * @param imm8    New immediate
 *
 */
#define R_RISCV_SET_STYPE_IMM8(operand, imm8)                                                      \
	((R_RISCV_CLEAR_STYPE_IMM8(operand)) | R_RISCV_STYPE_IMM8_MASK(imm8))

/** @brief Clear immediate bits in compressed J-type instruction.
 *
 * @param operand Address of RISC-V instruction, compressed-J-type
 *
 */
#define R_RISCV_CLEAR_CJTYPE_IMM8(operand) ((operand) & ~R_RISCV_CJTYPE_IMM8_MASK((uint32_t) -1))

/** @brief Overwrite immediate in compressed J-type instruction
 *
 * @param operand Address of RISC-V instruction, compressed-J-type
 * @param imm8    New immediate
 *
 */
#define R_RISCV_SET_CJTYPE_IMM8(operand, imm8)                                                     \
	((R_RISCV_CLEAR_CJTYPE_IMM8(operand)) | R_RISCV_CJTYPE_IMM8_MASK(imm8))

/** @brief Clear immediate bits in compressed B-type instruction.
 *
 * @param operand Address of RISC-V instruction, compressed-B-type
 *
 */
#define R_RISCV_CLEAR_CBTYPE_IMM8(operand) ((operand) & ~R_RISCV_CBTYPE_IMM8_MASK((uint32_t) -1))

/** @brief Overwrite immediate in compressed B-type instruction
 *
 * @param operand Address of RISC-V instruction, compressed-B-type
 * @param imm8    New immediate
 *
 */
#define R_RISCV_SET_CBTYPE_IMM8(operand, imm8)                                                     \
	((R_RISCV_CLEAR_CBTYPE_IMM8(operand)) | R_RISCV_CBTYPE_IMM8_MASK(imm8))

/** @brief Clear immediate bits in U-type instruction.
 *
 * @param operand Address of RISC-V instruction, U-type
 *
 */
#define R_RISCV_CLEAR_UTYPE_IMM8(operand) ((operand) & ~(0xFFFFF000))

/** @brief Overwrite immediate in U-type instruction
 *
 * @param operand Address of RISC-V instruction, U-type
 * @param imm8    New immediate
 *
 */
#define R_RISCV_SET_UTYPE_IMM8(operand, imm8)                                                      \
	((R_RISCV_CLEAR_UTYPE_IMM8(operand)) | ((imm8) & 0xFFFFF000))

/** @brief Clear immediate bits in I-type instruction.
 *
 * @param operand Address of RISC-V instruction, I-type
 *
 */
#define R_RISCV_CLEAR_ITYPE_IMM8(operand) ((operand) & ~(0xFFF00000))

/** @brief Overwrite immediate in I-type instruction
 *
 * @param operand Address of RISC-V instruction, I-type
 * @param imm8    New immediate
 *
 */
#define R_RISCV_SET_ITYPE_IMM8(operand, imm8) ((R_RISCV_CLEAR_ITYPE_IMM8(operand)) | ((imm8) << 20))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_RISCV_ELF_H */
