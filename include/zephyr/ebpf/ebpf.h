/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief eBPF primary public API header.
 *
 * @defgroup ebpf eBPF
 * @since 4.5
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

#ifndef ZEPHYR_INCLUDE_EBPF_H_
#define ZEPHYR_INCLUDE_EBPF_H_

#include <zephyr/ebpf/ebpf_insn.h>
#include <zephyr/ebpf/ebpf_prog.h>
#include <zephyr/ebpf/ebpf_map.h>
#include <zephyr/ebpf/ebpf_helpers.h>
#include <zephyr/ebpf/ebpf_tracepoint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build an eBPF instruction literal.
 *
 * @param _op Encoded eBPF opcode.
 * @param _dst Destination register number.
 * @param _src Source register number.
 * @param _off Signed instruction offset.
 * @param _imm Signed immediate value.
 */
#define EBPF_INSN_OP(_op, _dst, _src, _off, _imm)			\
	((struct ebpf_insn){						\
		.opcode = (_op),					\
		.regs   = ((_dst) & 0xF) | (((_src) & 0xF) << 4),	\
		.offset = (_off),					\
		.imm    = (_imm),					\
	})

/** @brief Move the value from @p src into @p dst. */
#define EBPF_MOV64_REG(dst, src)	EBPF_INSN_OP(EBPF_OP_MOV64_REG, dst, src, 0, 0)
/** @brief Load the immediate value @p imm into @p dst. */
#define EBPF_MOV64_IMM(dst, imm)	EBPF_INSN_OP(EBPF_OP_MOV64_IMM, dst, 0, 0, imm)
/** @brief Add the value in @p src to @p dst. */
#define EBPF_ADD64_REG(dst, src)	EBPF_INSN_OP(EBPF_OP_ADD64_REG, dst, src, 0, 0)
/** @brief Add the immediate value @p imm to @p dst. */
#define EBPF_ADD64_IMM(dst, imm)	EBPF_INSN_OP(EBPF_OP_ADD64_IMM, dst, 0, 0, imm)
/** @brief Subtract the value in @p src from @p dst. */
#define EBPF_SUB64_REG(dst, src)	EBPF_INSN_OP(EBPF_OP_SUB64_REG, dst, src, 0, 0)
/** @brief Subtract the immediate value @p imm from @p dst. */
#define EBPF_SUB64_IMM(dst, imm)	EBPF_INSN_OP(EBPF_OP_SUB64_IMM, dst, 0, 0, imm)
/** @brief Bitwise-AND @p dst with the immediate value @p imm. */
#define EBPF_AND64_IMM(dst, imm)	EBPF_INSN_OP(EBPF_OP_AND64_IMM, dst, 0, 0, imm)
/** @brief Bitwise-OR @p dst with the immediate value @p imm. */
#define EBPF_OR64_IMM(dst, imm)		EBPF_INSN_OP(EBPF_OP_OR64_IMM, dst, 0, 0, imm)
/** @brief Bitwise-XOR @p dst with the immediate value @p imm. */
#define EBPF_XOR64_IMM(dst, imm)	EBPF_INSN_OP(EBPF_OP_XOR64_IMM, dst, 0, 0, imm)
/** @brief Shift @p dst left by the immediate value @p imm. */
#define EBPF_LSH64_IMM(dst, imm)	EBPF_INSN_OP(EBPF_OP_LSH64_IMM, dst, 0, 0, imm)
/** @brief Shift @p dst right by the immediate value @p imm. */
#define EBPF_RSH64_IMM(dst, imm)	EBPF_INSN_OP(EBPF_OP_RSH64_IMM, dst, 0, 0, imm)
/** @brief Negate the value in @p dst. */
#define EBPF_NEG64(dst)			EBPF_INSN_OP(EBPF_OP_NEG64, dst, 0, 0, 0)
/** @brief Call the helper identified by @p helper_id. */
#define EBPF_CALL_HELPER(helper_id)	EBPF_INSN_OP(EBPF_OP_CALL, 0, 0, 0, helper_id)
/** @brief Terminate program execution and return R0. */
#define EBPF_EXIT_INSN()		EBPF_INSN_OP(EBPF_OP_EXIT, 0, 0, 0, 0)

/** @brief Load an 8-bit value from memory at @p src plus @p off into @p dst. */
#define EBPF_LDX_MEM_B(dst, src, off)	EBPF_INSN_OP(EBPF_OP_LDX_B, dst, src, off, 0)
/** @brief Load a 16-bit value from memory at @p src plus @p off into @p dst. */
#define EBPF_LDX_MEM_H(dst, src, off)	EBPF_INSN_OP(EBPF_OP_LDX_H, dst, src, off, 0)
/** @brief Load a 32-bit value from memory at @p src plus @p off into @p dst. */
#define EBPF_LDX_MEM_W(dst, src, off)	EBPF_INSN_OP(EBPF_OP_LDX_W, dst, src, off, 0)
/** @brief Load a 64-bit value from memory at @p src plus @p off into @p dst. */
#define EBPF_LDX_MEM_DW(dst, src, off)	EBPF_INSN_OP(EBPF_OP_LDX_DW, dst, src, off, 0)
/** @brief Store the low 8 bits of @p src to memory at @p dst plus @p off. */
#define EBPF_STX_MEM_B(dst, src, off)	EBPF_INSN_OP(EBPF_OP_STX_B, dst, src, off, 0)
/** @brief Store the low 16 bits of @p src to memory at @p dst plus @p off. */
#define EBPF_STX_MEM_H(dst, src, off)	EBPF_INSN_OP(EBPF_OP_STX_H, dst, src, off, 0)
/** @brief Store the low 32 bits of @p src to memory at @p dst plus @p off. */
#define EBPF_STX_MEM_W(dst, src, off)	EBPF_INSN_OP(EBPF_OP_STX_W, dst, src, off, 0)
/** @brief Store the 64-bit value in @p src to memory at @p dst plus @p off. */
#define EBPF_STX_MEM_DW(dst, src, off)	EBPF_INSN_OP(EBPF_OP_STX_DW, dst, src, off, 0)
/** @brief Store the immediate value @p imm to memory at @p dst plus @p off. */
#define EBPF_ST_MEM_W(dst, off, imm)	EBPF_INSN_OP(EBPF_OP_ST_W, dst, 0, off, imm)

/** @brief Jump unconditionally by @p off instructions. */
#define EBPF_JMP_A(off)			EBPF_INSN_OP(EBPF_OP_JA, 0, 0, off, 0)
/** @brief Jump by @p off if @p dst equals the immediate value @p imm. */
#define EBPF_JEQ_IMM(dst, imm, off)	EBPF_INSN_OP(EBPF_OP_JEQ_IMM, dst, 0, off, imm)
/** @brief Jump by @p off if @p dst equals @p src. */
#define EBPF_JEQ_REG(dst, src, off)	EBPF_INSN_OP(EBPF_OP_JEQ_REG, dst, src, off, 0)
/** @brief Jump by @p off if @p dst does not equal the immediate value @p imm. */
#define EBPF_JNE_IMM(dst, imm, off)	EBPF_INSN_OP(EBPF_OP_JNE_IMM, dst, 0, off, imm)
/** @brief Jump by @p off if @p dst is greater than the immediate value @p imm. */
#define EBPF_JGT_IMM(dst, imm, off)	EBPF_INSN_OP(EBPF_OP_JGT_IMM, dst, 0, off, imm)
/** @brief Jump by @p off if @p dst is greater than or equal to @p imm. */
#define EBPF_JGE_IMM(dst, imm, off)	EBPF_INSN_OP(EBPF_OP_JGE_IMM, dst, 0, off, imm)
/** @brief Jump by @p off if @p dst is less than the immediate value @p imm. */
#define EBPF_JLT_IMM(dst, imm, off)	EBPF_INSN_OP(EBPF_OP_JLT_IMM, dst, 0, off, imm)
/** @brief Jump by @p off if @p dst is less than or equal to @p imm. */
#define EBPF_JLE_IMM(dst, imm, off)	EBPF_INSN_OP(EBPF_OP_JLE_IMM, dst, 0, off, imm)

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_EBPF_H_ */
