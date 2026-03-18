/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief eBPF instruction encoding and opcode definitions.
 *
 * Uses a 64-bit instruction format compatible with the standard Linux eBPF encoding,
 * enabling potential future use of standard tooling (clang -target bpf).
 * @ingroup ebpf
 */

#ifndef ZEPHYR_INCLUDE_EBPF_INSN_H_
#define ZEPHYR_INCLUDE_EBPF_INSN_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief eBPF instruction format (64-bit).
 *
 * Layout (little-endian):
 *   Bits  0-7:   opcode (8 bits)
 *   Bits  8-11:  dst_reg (4 bits)
 *   Bits 12-15:  src_reg (4 bits)
 *   Bits 16-31:  offset  (16 bits, signed)
 *   Bits 32-63:  imm     (32 bits, signed)
 */
struct ebpf_insn {
	uint8_t  opcode; /**< Encoded opcode describing the instruction class and operation. */
	uint8_t  regs;   /**< Packed register operands as dst_reg:4 | src_reg:4. */
	int16_t  offset; /**< Signed branch or memory offset. */
	int32_t  imm;    /**< Signed 32-bit immediate operand. */
};

/** @cond INTERNAL_HIDDEN */

/* ==================== Helper macros ==================== */
#define EBPF_INSN_DST(insn)	((insn)->regs & 0x0FU)
#define EBPF_INSN_SRC(insn)	(((insn)->regs >> 4) & 0x0FU)
#define EBPF_CLS_MASK		0x07U
#define EBPF_INSN_CLASS(opcode)	((opcode) & EBPF_CLS_MASK)

/* ==================== Instruction classes ==================== */
/* Instruction class selector (bits 0-2 of opcode) */
#define EBPF_CLS_LD		0x00U
#define EBPF_CLS_LDX		0x01U
#define EBPF_CLS_ST		0x02U
#define EBPF_CLS_STX		0x03U
#define EBPF_CLS_ALU		0x04U
#define EBPF_CLS_JMP		0x05U
#define EBPF_CLS_ALU64		0x07U

/* ALU source selector (bit 3 of opcode for ALU / ALU64 or JMP) */
#define EBPF_SRC_IMM		0x00U
#define EBPF_SRC_REG		0x08U

/* ALU operation selector (bits 4-7 of opcode for ALU / ALU64) */
#define EBPF_ALU_ADD		0x00U
#define EBPF_ALU_SUB		0x10U
#define EBPF_ALU_MUL		0x20U
#define EBPF_ALU_DIV		0x30U
#define EBPF_ALU_OR		0x40U
#define EBPF_ALU_AND		0x50U
#define EBPF_ALU_LSH		0x60U
#define EBPF_ALU_RSH		0x70U
#define EBPF_ALU_NEG		0x80U
#define EBPF_ALU_MOD		0x90U
#define EBPF_ALU_XOR		0xA0U
#define EBPF_ALU_MOV		0xB0U

/* Jump operation selector (bits 4-7 of opcode for JMP) */
#define EBPF_JMP_JA		0x00U
#define EBPF_JMP_JEQ		0x10U
#define EBPF_JMP_JGT		0x20U
#define EBPF_JMP_JGE		0x30U
#define EBPF_JMP_JSET		0x40U
#define EBPF_JMP_JNE		0x50U
#define EBPF_JMP_JLT		0xA0U
#define EBPF_JMP_JLE		0xB0U
#define EBPF_JMP_CALL		0x80U
#define EBPF_JMP_EXIT		0x90U

/* Memory access size selector (bits 3-4 of opcode for LD/ST) */
#define EBPF_SIZE_W		0x00U	/* 32-bit word */
#define EBPF_SIZE_H		0x08U	/* 16-bit half */
#define EBPF_SIZE_B		0x10U	/* 8-bit byte */
#define EBPF_SIZE_DW		0x18U	/* 64-bit double word */

/* Memory access mode selector (bits 5-7 of opcode for LD/ST) */
#define EBPF_MODE_MEM		0x60U

/* ==================== Composed opcodes for convenience ==================== */
/* 64-bit ALU composed opcodes. */
#define EBPF_OP_ADD64_IMM	(EBPF_CLS_ALU64 | EBPF_ALU_ADD | EBPF_SRC_IMM)
#define EBPF_OP_ADD64_REG	(EBPF_CLS_ALU64 | EBPF_ALU_ADD | EBPF_SRC_REG)
#define EBPF_OP_SUB64_IMM	(EBPF_CLS_ALU64 | EBPF_ALU_SUB | EBPF_SRC_IMM)
#define EBPF_OP_SUB64_REG	(EBPF_CLS_ALU64 | EBPF_ALU_SUB | EBPF_SRC_REG)
#define EBPF_OP_MUL64_IMM	(EBPF_CLS_ALU64 | EBPF_ALU_MUL | EBPF_SRC_IMM)
#define EBPF_OP_MUL64_REG	(EBPF_CLS_ALU64 | EBPF_ALU_MUL | EBPF_SRC_REG)
#define EBPF_OP_DIV64_IMM	(EBPF_CLS_ALU64 | EBPF_ALU_DIV | EBPF_SRC_IMM)
#define EBPF_OP_DIV64_REG	(EBPF_CLS_ALU64 | EBPF_ALU_DIV | EBPF_SRC_REG)
#define EBPF_OP_OR64_IMM	(EBPF_CLS_ALU64 | EBPF_ALU_OR  | EBPF_SRC_IMM)
#define EBPF_OP_OR64_REG	(EBPF_CLS_ALU64 | EBPF_ALU_OR  | EBPF_SRC_REG)
#define EBPF_OP_AND64_IMM	(EBPF_CLS_ALU64 | EBPF_ALU_AND | EBPF_SRC_IMM)
#define EBPF_OP_AND64_REG	(EBPF_CLS_ALU64 | EBPF_ALU_AND | EBPF_SRC_REG)
#define EBPF_OP_LSH64_IMM	(EBPF_CLS_ALU64 | EBPF_ALU_LSH | EBPF_SRC_IMM)
#define EBPF_OP_LSH64_REG	(EBPF_CLS_ALU64 | EBPF_ALU_LSH | EBPF_SRC_REG)
#define EBPF_OP_RSH64_IMM	(EBPF_CLS_ALU64 | EBPF_ALU_RSH | EBPF_SRC_IMM)
#define EBPF_OP_RSH64_REG	(EBPF_CLS_ALU64 | EBPF_ALU_RSH | EBPF_SRC_REG)
#define EBPF_OP_NEG64		(EBPF_CLS_ALU64 | EBPF_ALU_NEG | EBPF_SRC_IMM)
#define EBPF_OP_MOD64_IMM	(EBPF_CLS_ALU64 | EBPF_ALU_MOD | EBPF_SRC_IMM)
#define EBPF_OP_MOD64_REG	(EBPF_CLS_ALU64 | EBPF_ALU_MOD | EBPF_SRC_REG)
#define EBPF_OP_XOR64_IMM	(EBPF_CLS_ALU64 | EBPF_ALU_XOR | EBPF_SRC_IMM)
#define EBPF_OP_XOR64_REG	(EBPF_CLS_ALU64 | EBPF_ALU_XOR | EBPF_SRC_REG)
#define EBPF_OP_MOV64_IMM	(EBPF_CLS_ALU64 | EBPF_ALU_MOV | EBPF_SRC_IMM)
#define EBPF_OP_MOV64_REG	(EBPF_CLS_ALU64 | EBPF_ALU_MOV | EBPF_SRC_REG)

/* 32-bit ALU composed opcodes */
#define EBPF_OP_ADD_IMM		(EBPF_CLS_ALU | EBPF_ALU_ADD | EBPF_SRC_IMM)
#define EBPF_OP_ADD_REG		(EBPF_CLS_ALU | EBPF_ALU_ADD | EBPF_SRC_REG)
#define EBPF_OP_SUB_IMM		(EBPF_CLS_ALU | EBPF_ALU_SUB | EBPF_SRC_IMM)
#define EBPF_OP_SUB_REG		(EBPF_CLS_ALU | EBPF_ALU_SUB | EBPF_SRC_REG)
#define EBPF_OP_MUL_IMM		(EBPF_CLS_ALU | EBPF_ALU_MUL | EBPF_SRC_IMM)
#define EBPF_OP_MUL_REG		(EBPF_CLS_ALU | EBPF_ALU_MUL | EBPF_SRC_REG)
#define EBPF_OP_DIV_IMM		(EBPF_CLS_ALU | EBPF_ALU_DIV | EBPF_SRC_IMM)
#define EBPF_OP_DIV_REG		(EBPF_CLS_ALU | EBPF_ALU_DIV | EBPF_SRC_REG)
#define EBPF_OP_OR_IMM		(EBPF_CLS_ALU | EBPF_ALU_OR  | EBPF_SRC_IMM)
#define EBPF_OP_OR_REG		(EBPF_CLS_ALU | EBPF_ALU_OR  | EBPF_SRC_REG)
#define EBPF_OP_AND_IMM		(EBPF_CLS_ALU | EBPF_ALU_AND | EBPF_SRC_IMM)
#define EBPF_OP_AND_REG		(EBPF_CLS_ALU | EBPF_ALU_AND | EBPF_SRC_REG)
#define EBPF_OP_LSH_IMM		(EBPF_CLS_ALU | EBPF_ALU_LSH | EBPF_SRC_IMM)
#define EBPF_OP_LSH_REG		(EBPF_CLS_ALU | EBPF_ALU_LSH | EBPF_SRC_REG)
#define EBPF_OP_RSH_IMM		(EBPF_CLS_ALU | EBPF_ALU_RSH | EBPF_SRC_IMM)
#define EBPF_OP_RSH_REG		(EBPF_CLS_ALU | EBPF_ALU_RSH | EBPF_SRC_REG)
#define EBPF_OP_NEG		(EBPF_CLS_ALU | EBPF_ALU_NEG | EBPF_SRC_IMM)
#define EBPF_OP_MOD_IMM		(EBPF_CLS_ALU | EBPF_ALU_MOD | EBPF_SRC_IMM)
#define EBPF_OP_MOD_REG		(EBPF_CLS_ALU | EBPF_ALU_MOD | EBPF_SRC_REG)
#define EBPF_OP_XOR_IMM		(EBPF_CLS_ALU | EBPF_ALU_XOR | EBPF_SRC_IMM)
#define EBPF_OP_XOR_REG		(EBPF_CLS_ALU | EBPF_ALU_XOR | EBPF_SRC_REG)
#define EBPF_OP_MOV_IMM		(EBPF_CLS_ALU | EBPF_ALU_MOV | EBPF_SRC_IMM)
#define EBPF_OP_MOV_REG		(EBPF_CLS_ALU | EBPF_ALU_MOV | EBPF_SRC_REG)

/* Memory composed opcodes */
#define EBPF_OP_LDX_B		(EBPF_CLS_LDX | EBPF_SIZE_B  | EBPF_MODE_MEM)
#define EBPF_OP_LDX_H		(EBPF_CLS_LDX | EBPF_SIZE_H  | EBPF_MODE_MEM)
#define EBPF_OP_LDX_W		(EBPF_CLS_LDX | EBPF_SIZE_W  | EBPF_MODE_MEM)
#define EBPF_OP_LDX_DW		(EBPF_CLS_LDX | EBPF_SIZE_DW | EBPF_MODE_MEM)
#define EBPF_OP_STX_B		(EBPF_CLS_STX | EBPF_SIZE_B  | EBPF_MODE_MEM)
#define EBPF_OP_STX_H		(EBPF_CLS_STX | EBPF_SIZE_H  | EBPF_MODE_MEM)
#define EBPF_OP_STX_W		(EBPF_CLS_STX | EBPF_SIZE_W  | EBPF_MODE_MEM)
#define EBPF_OP_STX_DW		(EBPF_CLS_STX | EBPF_SIZE_DW | EBPF_MODE_MEM)
#define EBPF_OP_ST_B		(EBPF_CLS_ST  | EBPF_SIZE_B  | EBPF_MODE_MEM)
#define EBPF_OP_ST_H		(EBPF_CLS_ST  | EBPF_SIZE_H  | EBPF_MODE_MEM)
#define EBPF_OP_ST_W		(EBPF_CLS_ST  | EBPF_SIZE_W  | EBPF_MODE_MEM)
#define EBPF_OP_ST_DW		(EBPF_CLS_ST  | EBPF_SIZE_DW | EBPF_MODE_MEM)

/* Jump composed opcodes */
#define EBPF_OP_JA		(EBPF_CLS_JMP | EBPF_JMP_JA)
#define EBPF_OP_JEQ_IMM		(EBPF_CLS_JMP | EBPF_JMP_JEQ  | EBPF_SRC_IMM)
#define EBPF_OP_JEQ_REG		(EBPF_CLS_JMP | EBPF_JMP_JEQ  | EBPF_SRC_REG)
#define EBPF_OP_JGT_IMM		(EBPF_CLS_JMP | EBPF_JMP_JGT  | EBPF_SRC_IMM)
#define EBPF_OP_JGT_REG		(EBPF_CLS_JMP | EBPF_JMP_JGT  | EBPF_SRC_REG)
#define EBPF_OP_JGE_IMM		(EBPF_CLS_JMP | EBPF_JMP_JGE  | EBPF_SRC_IMM)
#define EBPF_OP_JGE_REG		(EBPF_CLS_JMP | EBPF_JMP_JGE  | EBPF_SRC_REG)
#define EBPF_OP_JSET_IMM	(EBPF_CLS_JMP | EBPF_JMP_JSET | EBPF_SRC_IMM)
#define EBPF_OP_JSET_REG	(EBPF_CLS_JMP | EBPF_JMP_JSET | EBPF_SRC_REG)
#define EBPF_OP_JNE_IMM		(EBPF_CLS_JMP | EBPF_JMP_JNE  | EBPF_SRC_IMM)
#define EBPF_OP_JNE_REG		(EBPF_CLS_JMP | EBPF_JMP_JNE  | EBPF_SRC_REG)
#define EBPF_OP_JLT_IMM		(EBPF_CLS_JMP | EBPF_JMP_JLT  | EBPF_SRC_IMM)
#define EBPF_OP_JLT_REG		(EBPF_CLS_JMP | EBPF_JMP_JLT  | EBPF_SRC_REG)
#define EBPF_OP_JLE_IMM		(EBPF_CLS_JMP | EBPF_JMP_JLE  | EBPF_SRC_IMM)
#define EBPF_OP_JLE_REG		(EBPF_CLS_JMP | EBPF_JMP_JLE  | EBPF_SRC_REG)
#define EBPF_OP_CALL		(EBPF_CLS_JMP | EBPF_JMP_CALL)
#define EBPF_OP_EXIT		(EBPF_CLS_JMP | EBPF_JMP_EXIT)

/** @endcond */

/* ==================== Register numbering ==================== */
#define EBPF_REG_R0		0	/**< Return value register. */
#define EBPF_REG_R1		1	/**< Argument 1 or scratch register. */
#define EBPF_REG_R2		2	/**< Argument 2 or scratch register. */
#define EBPF_REG_R3		3	/**< Argument 3 or scratch register. */
#define EBPF_REG_R4		4	/**< Argument 4 or scratch register. */
#define EBPF_REG_R5		5	/**< Argument 5 or scratch register. */
#define EBPF_REG_R6		6	/**< Callee-saved general-purpose register. */
#define EBPF_REG_R7		7	/**< Callee-saved general-purpose register. */
#define EBPF_REG_R8		8	/**< Callee-saved general-purpose register. */
#define EBPF_REG_R9		9	/**< Callee-saved general-purpose register. */
#define EBPF_REG_R10		10	/**< Read-only frame pointer register. */

#define EBPF_NUM_REGS		11	/**< Total number of architectural eBPF registers. */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_EBPF_INSN_H_ */
