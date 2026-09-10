/*
 * Copyright (c) 2026, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zsr.h>

.pushsection .iram.text, "ax"
.global _EmulateLoadStore
_EmulateLoadStore:
	/* This procedure uses registers a0-3. a0 is backed up in a spare register
	 * by the caller of this routine. a1-3 are backed up here using our "misc
	 * extra" registers.
	 */
	wsr a1, ZSR_LSE_SAVE0
	wsr a2, ZSR_LSE_SAVE1
	wsr a3, ZSR_LSE_SAVE2

	/* We need to get the instruction that caused the exception. Unfortunately
	 * we cannot read it directly from IRAM because it may be spread across
	 * multiple dwords, and we can only read whole dwords from IRAM. So here we
	 * read in both the lower and upper dword.
	 */
	rsr a3, EPC1
	movi a0, 3
	and a1, a3, a0
	sub a2, a3, a1
	addi a3, a2, 4
	l32i a2, a2, 0
	l32i a3, a3, 0
	/* A1: lower 2 bytes of the exception program counter
	 * A2: lower dword, containing none or part of the instruction
	 * A3: Upper dword, containing none or part of the instruction
	 */
	beqi a1, 1, _inst_offset1
	beqi a1, 2, _inst_offset2
	beqi a1, 3, _inst_offset3

_inst_offset0:
	j _extract_instruction_fields
_inst_offset1:
	srli a2, a2, 8
	j _extract_instruction_fields
_inst_offset2:
	extui a2, a2, 16, 16
	slli a3, a3, 16
	or a2, a2, a3
	j _extract_instruction_fields
_inst_offset3:
	extui a2, a2, 24, 8
	slli a3, a3, 8
	or a2, a2, a3


_extract_instruction_fields:
	/* A2[23:0] now contains the complete instruction, with some garbage maybe
	 * in A2[31:24]
	 */
	extui a1, a2, 4, 4
	extui a3, a2, 12, 4
	slli a3, a3, 4
	extui a2, a2, 0, 4
	or a2, a2, a3

	/* A2 now contains concatenation of minor + major opcode.
	 * Instruction: xxxx xxxx mmmm xxxx tttt MMMM
	 * A2 reg: 0000 0000 0000 0000 mmmm MMMM <- Determine opcode
	 * A1 reg: 0000 0000 0000 0000 0000 tttt <- Determine "at" (dst) reg
	 *
	 * Look for the 3-byte opcodes first:
	 */
	movi a3, 2
	beq a2, a3, _Handle_L8UI

	movi a3, 66
	beq a2, a3, _Handle_S8I

	movi a3, 34
	beq a2, a3, _Handle_L32I

	movi a3, 98
	beq a2, a3, _Handle_S32I

	movi a3, 18
	beq a2, a3, _Handle_L16UI

	movi a3, 82
	beq a2, a3, _Handle_S16I

	/* Now look for the narrow (2-byte) opcodes, which we only compare the
	 * bottom nibble of the opcode.
	 */
	extui a2, a2, 0, 4

	movi a3, 8
	beq a2, a3, _Handle_L32IN

	movi a3, 9
	beq a2, a3, _Handle_S32IN

_CannotHandle:
	/* Return to generic Xtensa exception control flow */
	rsr a0, ZSR_A0SAVE
	rsr a1, ZSR_LSE_SAVE0
	rsr a2, ZSR_LSE_SAVE1
	rsr a3, ZSR_LSE_SAVE2
	j _Level1Vector

_Handle_S32I:
	rsr a2, EPC1
	addi a2, a2, 3
	wsr a2, EPC1
	j _Handle_S32_Common

_Handle_S32IN:
	rsr a2, EPC1
	addi a2, a2, 2
	wsr a2, EPC1

_Handle_S32_Common:
	movi a3, 0
	beqz a1, _store_0
	beqi a1, 1, _store_1
	beqi a1, 2, _store_2
	beqi a1, 3, _store_3
	beqi a1, 4, _store_4
	beqi a1, 5, _store_5
	beqi a1, 6, _store_6
	beqi a1, 7, _store_7
	beqi a1, 8, _store_8
	movi a2, 9
	beq a1, a2, _store_9
	beqi a1, 10, _store_10
	movi a2, 11
	beq a1, a2, _store_11
	beqi a1, 12, _store_12
	movi a2, 13
	beq a1, a2, _store_13
	movi a2, 14
	beq a1, a2, _store_14
	movi a2, 15
	beq a1, a2, _store_15

_continue_s32:
	/* A1 contains the dword to write from the caller */
	rsr a3, excvaddr
	srli a2, a3, 2
	slli a2, a2, 2
	l32i a0, a2, 0 /* a0: containing lower dword */
	movi a2, 3
	and a3, a3, a2 /* a3 contains the byte offset (valid values 1, 2, 3) */
	beqi a3, 2, _s32_offs_2
	beqi a3, 3, _s32_offs_3
_s32_offs_1:
	movi a2, 0x000000FF
	and a0, a0, a2 /* a0: lower dword masked off */
	ssa8b a3
	sll a2, a1
	or a0, a0, a2
	rsr a2, excvaddr
	srli a2, a2, 2
	slli a2, a2, 2
	s32i a0, a2, 0 /* lower dword written back */
	addi a2, a2, 4
	l32i a0, a2, 0 /* a0: upper dword */
	movi a3, -256
	and a0, a0, a3
	extui a1, a1, 24, 8
	or a0, a0, a1
	s32i a0, a2, 0 /* upper dword written back */
	j _s32_done

_s32_offs_2:
	movi a2, -1
	srli a2, a2, 16
	and a0, a0, a2 /* a0: lower dword masked off */
	ssa8b a3
	sll a2, a1
	or a0, a0, a2
	rsr a2, excvaddr
	srli a2, a2, 2
	slli a2, a2, 2
	s32i a0, a2, 0 /* lower dword written back */
	addi a2, a2, 4
	l32i a0, a2, 0 /* a0: upper dword */
	movi a3, -1
	slli a3, a3, 16
	and a0, a0, a3
	extui a1, a1, 16, 16
	or a0, a0, a1
	s32i a0, a2, 0 /* upper dword written back */
	j _s32_done

_s32_offs_3:
	movi a2, -1
	srli a2, a2, 8
	and a0, a0, a2 /* a0: lower dword masked off */
	ssa8b a3
	sll a2, a1
	or a0, a0, a2
	rsr a2, excvaddr
	srli a2, a2, 2
	slli a2, a2, 2
	s32i a0, a2, 0 /* lower dword written back */
	addi a2, a2, 4
	l32i a0, a2, 0 /* a0: upper dword */
	movi a3, -1
	slli a3, a3, 24
	and a0, a0, a3
	srli a1, a1, 8
	or a0, a0, a1
	s32i a0, a2, 0 /* upper dword written back */
	j _s32_done

_s32_done:
	rsr a0, ZSR_A0SAVE
	rsr a1, ZSR_LSE_SAVE0
	rsr a2, ZSR_LSE_SAVE1
	rsr a3, ZSR_LSE_SAVE2
	rfe

_store_0:
	rsr a1, ZSR_A0SAVE
	j _store_continue
_store_1:
	rsr a1, ZSR_LSE_SAVE0
	j _store_continue
_store_2:
	rsr a1, ZSR_LSE_SAVE1
	j _store_continue
_store_3:
	rsr a1, ZSR_LSE_SAVE2
	j _store_continue
_store_4:
	mov a1, a4
	j _store_continue
_store_5:
	mov a1, a5
	j _store_continue
_store_6:
	mov a1, a6
	j _store_continue
_store_7:
	mov a1, a7
	j _store_continue
_store_8:
	mov a1, a8
	j _store_continue
_store_9:
	mov a1, a9
	j _store_continue
_store_10:
	mov a1, a10
	j _store_continue
_store_11:
	mov a1, a11
	j _store_continue
_store_12:
	mov a1, a12
	j _store_continue
_store_13:
	mov a1, a13
	j _store_continue
_store_14:
	mov a1, a14
	j _store_continue
_store_15:
	mov a1, a15
	j _store_continue

_store_continue:
	beqz a3, _continue_s32
	beqi a3, 1, _continue_s16
	j _continue_s8i

_Handle_S16I:
	/* Advance the program counter by 3. */
	rsr a2, EPC1
	addi a2, a2, 3
	wsr a2, EPC1

	/* We always need the lower dword */
	rsr a3, excvaddr
	srli a3, a3, 2
	slli a3, a3, 2
	l32i a0, a3, 0 /* a0: lower dword */

	/* Get the word to write from "as" register */
	movi a3, 1
	beqz a1, _store_0
	beqi a1, 1, _store_1
	beqi a1, 2, _store_2
	beqi a1, 3, _store_3
	beqi a1, 4, _store_4
	beqi a1, 5, _store_5
	beqi a1, 6, _store_6
	beqi a1, 7, _store_7
	beqi a1, 8, _store_8
	movi a2, 9
	beq a1, a2, _store_9
	beqi a1, 10, _store_10
	movi a2, 11
	beq a1, a2, _store_11
	beqi a1, 12, _store_12
	movi a2, 13
	beq a1, a2, _store_13
	movi a2, 14
	beq a1, a2, _store_14
	movi a2, 15
	beq a1, a2, _store_15

_continue_s16:
	extui a1, a1, 0, 16 /* Mask off the upper bits of the input. */
	rsr a2, excvaddr
	mov a3, a2
	extui a2, a2, 0, 2 /* a2: byte offset of word */
	sub a3, a3, a2
	beqi a2, 1, _s16_offset_1
	beqi a2, 2, _s16_offset_2
	beqi a2, 3, _s16_offset_3
_s16_offset_0:
	movi a2, -1
	slli a2, a2, 16
	and a0, a0, a2
	or a0, a0, a1
	s32i a0, a3, 0
	j _s16_done
_s16_offset_1:
	movi a2, -1
	slli a2, a2, 24
	addi a2, a2, 0xFF
	and a0, a0, a2
	slli a1, a1, 8
	or a0, a0, a1
	s32i a0, a3, 0
	j _s16_done
_s16_offset_2:
	movi a2, -1
	srli a2, a2, 16
	and a0, a0, a2
	slli a1, a1, 16
	or a0, a0, a1
	s32i a0, a3, 0
	j _s16_done
_s16_offset_3:
	movi a2, -1
	srli a2, a2, 8
	and a0, a0, a2
	slli a2, a1, 24
	or a0, a0, a2
	s32i a0, a3, 0
	/* Now deal with the upper dword */
	addi a3, a3, 4
	l32i a0, a3, 0
	movi a2, -256
	and a0, a0, a2
	extui a1, a1, 8, 8
	or a0, a0, a1
	s32i a0, a3, 0
_s16_done:
	rsr a0, ZSR_A0SAVE
	rsr a1, ZSR_LSE_SAVE0
	rsr a2, ZSR_LSE_SAVE1
	rsr a3, ZSR_LSE_SAVE2
	rfe

_Handle_S8I:
	/* Advance the program counter by 3. */
	rsr a2, EPC1
	addi a2, a2, 3
	wsr a2, EPC1

	/* 1. Read the containing dword from memory */
	rsr a3, excvaddr
	srli a3, a3, 2
	slli a3, a3, 2
	l32i a0, a3, 0
	/* A0 is the dword containing the byte we're writing */

	movi a3, 2
	beqz a1, _store_0
	beqi a1, 1, _store_1
	beqi a1, 2, _store_2
	beqi a1, 3, _store_3
	beqi a1, 4, _store_4
	beqi a1, 5, _store_5
	beqi a1, 6, _store_6
	beqi a1, 7, _store_7
	beqi a1, 8, _store_8
	movi a2, 9
	beq a1, a2, _store_9
	beqi a1, 10, _store_10
	movi a2, 11
	beq a1, a2, _store_11
	beqi a1, 12, _store_12
	movi a2, 13
	beq a1, a2, _store_13
	movi a2, 14
	beq a1, a2, _store_14
	movi a2, 15
	beq a1, a2, _store_15

	/* 2. Fetch the byte to write from working register */
_continue_s8i:
	/* At this point, A0 has the shadow copy, A1 has the value to write
	 * Unfortunately we do not have the DEPBITS instruction, so masking /
	 * shifting is necessary.
	 */

	/* Determine byte offset in the shadow dword */
	rsr a3, excvaddr
	extui a3, a3, 0, 2
	/* A3: byte offset */

	/* Remove upper 3 bytes from input value. */
	movi a2, 0xFF
	and a1, a1, a2
	ssa8b a3
	sll a1, a1
	sll a2, a2
	movi a3, -1
	xor a2, a2, a3
	and a0, a0, a2
	or a0, a0, a1

	/* 3. Write the dword back to memory */
	rsr a3, excvaddr
	srli a3, a3, 2
	slli a3, a3, 2
	s32i a0, a3, 0

	/* Restore working register context */
	rsr a0, ZSR_A0SAVE
	rsr a1, ZSR_LSE_SAVE0
	rsr a2, ZSR_LSE_SAVE1
	rsr a3, ZSR_LSE_SAVE2
	rfe

_Handle_L32I:
	/* Advance the program counter by 3. Note that this works for all load /
	 * store instructions except for L32I.N/S32I.N, which are two bytes long.
	 */
	rsr a2, EPC1
	addi a2, a2, 3
	wsr a2, EPC1
	j _Handle_L32I_Common
_Handle_L32IN:
	rsr a2, EPC1
	addi a2, a2, 2
	wsr a2, EPC1

_Handle_L32I_Common:
	/* 32-bit load/stores cannot generate an exception due to accessing IRAM,
	 * only due to misalignment (in either IRAM or DRAM). For every exception,
	 * we need to load both upper/lower dwords, since the fault address cannot
	 * be dword-aligned. Get the lower dword in place.
	 */
	rsr a3, excvaddr
	extui a2, a3, 0, 2
	srli a3, a3, 2
	slli a3, a3, 2 /* a3: lower dword address */
	l32i a0, a3, 0 /* a0: containing lower dword */
	ssa8l a2
	srl a0, a0
	movi a3, 4
	sub a2, a3, a2 /* a2: 4-offset */
	ssa8b a2
	rsr a3, excvaddr
	add a3, a3, a2 /* a3: upper dword address */
	l32i a3, a3, 0 /* a3: containing upper dword */
	sll a3, a3
	or a0, a0, a3

	j _load_return

_Handle_L16UI:
	/* Advance program counter */
	rsr a2, EPC1
	addi a2, a2, 3
	wsr a2, EPC1

	/* Lower dword always needs to be read */
	rsr a3, excvaddr
	extui a2, a3, 0, 2
	srli a3, a3, 2
	slli a3, a3, 2 /* a3: lower dword address */
	l32i a0, a3, 0 /* a0: containing lower dword */
	beqi a2, 1, _l16_offset_1
	beqi a2, 2, _l16_offset_2
	beqi a2, 3, _l16_offset_3
_l16_offset_0:
	extui a0, a0, 0, 16
	j _load_return
_l16_offset_1:
	extui a0, a0, 8, 16
	j _load_return
_l16_offset_2:
	extui a0, a0, 16, 16
	j _load_return
_l16_offset_3:
	/* a0: lower byte of word, with all other bits masked off to 0 */
	extui a0, a0, 24, 8
	rsr a3, excvaddr
	addi a3, a3, 1
	l32i a3, a3, 0
	extui a3, a3, 0, 8 /* a3: upper dword, with 3 upper bytes masked off */
	slli a3, a3, 8
	or a0, a0, a3
	j _load_return

_Handle_L8UI:
	rsr a2, EPC1
	addi a2, a2, 3
	wsr a2, EPC1
	/* Extract the actual byte of data that we need to return */
	rsr a3, excvaddr
	srli a3, a3, 2
	slli a3, a3, 2
	l32i a0, a3, 0
	rsr a3, excvaddr
	movi a2, 3
	and a3, a3, a2
	ssa8l a3
	srl a0, a0
	extui a0, a0, 0, 8
	/* A0: data to return */

_load_return:

	beqz a1, _load_return_0
	beqi a1, 1, _load_return_1
	beqi a1, 2, _load_return_2
	beqi a1, 3, _load_return_3
	beqi a1, 4, _load_return_4
	beqi a1, 5, _load_return_5
	beqi a1, 6, _load_return_6
	beqi a1, 7, _load_return_7
	beqi a1, 8, _load_return_8
	movi a3, 9
	beq a1, a3, _load_return_9
	beqi a1, 10, _load_return_10
	movi a3, 11
	beq a1, a3, _load_return_11
	beqi a1, 12, _load_return_12
	movi a3, 13
	beq a1, a3, _load_return_13
	movi a3, 14
	beq a1, a3, _load_return_14
	movi a3, 15
	beq a1, a3, _load_return_15

_load_return_0:
	rsr a1, ZSR_LSE_SAVE0
	rsr a2, ZSR_LSE_SAVE1
	rsr a3, ZSR_LSE_SAVE2
	rfe
_load_return_1:
	mov a1, a0
	rsr a0, ZSR_A0SAVE
	rsr a2, ZSR_LSE_SAVE1
	rsr a3, ZSR_LSE_SAVE2
	rfe
_load_return_2:
	mov a2, a0
	rsr a0, ZSR_A0SAVE
	rsr a1, ZSR_LSE_SAVE0
	rsr a3, ZSR_LSE_SAVE2
	rfe
_load_return_3:
	mov a3, a0
	rsr a0, ZSR_A0SAVE
	rsr a1, ZSR_LSE_SAVE0
	rsr a2, ZSR_LSE_SAVE1
	rfe
_load_return_4:
	mov a4, a0
	j _HandleOut
_load_return_5:
	mov a5, a0
	j _HandleOut
_load_return_6:
	mov a6, a0
	j _HandleOut
_load_return_7:
	mov a7, a0
	j _HandleOut
_load_return_8:
	mov a8, a0
	j _HandleOut
_load_return_9:
	mov a9, a0
	j _HandleOut
_load_return_10:
	mov a10, a0
	j _HandleOut
_load_return_11:
	mov a11, a0
	j _HandleOut
_load_return_12:
	mov a12, a0
	j _HandleOut
_load_return_13:
	mov a13, a0
	j _HandleOut
_load_return_14:
	mov a14, a0
	j _HandleOut
_load_return_15:
	mov a15, a0

_HandleOut:
	rsr a0, ZSR_A0SAVE
	rsr a1, ZSR_LSE_SAVE0
	rsr a2, ZSR_LSE_SAVE1
	rsr a3, ZSR_LSE_SAVE2
	rfe

.popsection
