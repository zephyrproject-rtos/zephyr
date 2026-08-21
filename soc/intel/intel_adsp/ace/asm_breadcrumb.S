/*
 * Copyright (c) 2026 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * adsp_triple_fault_breadcrumb - record triple fault state into HP-SRAM window0.
 *
 * Strong override of the weak default in arch/xtensa/core/xtensa_asm2_util.S,
 * linked in only for the ACE SoC family (see CMakeLists.txt). Called via
 * CALL0 from the shared triple-fault handler.
 *
 * Writes EPC1, EXCCAUSE|0xdb1e0000 and DEPC to the HP-SRAM window0 uncached
 * alias so the host SOF driver can diagnose a triple fault from dmesg.
 * Constructs the base address using shifts to avoid literal generation under
 * Clang's Integrated Assembler. Clobbers a2/a3/a4.
 */
.global adsp_triple_fault_breadcrumb
.align 4
adsp_triple_fault_breadcrumb:
	/* Construct window0 uncached base without literals/l32r (Clang IAS safe) */
	movi a3, 1
	slli a3, a3, 30
	movi a4, 0x240
	slli a4, a4, 8
	add a3, a3, a4
	rsr.epc1 a2
	s32i a2, a3, 0
	/* Construct 0xdb1e0000 without literals/l32r */
	movi a2, 0xdb
	slli a2, a2, 8
	movi a4, 0x1e
	add a2, a2, a4
	slli a2, a2, 16
	rsr.exccause a4
	or a2, a2, a4
	s32i a2, a3, 4
	rsr.depc a2
	s32i a2, a3, 8
	memw
	ret
