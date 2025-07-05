/*
 * Copyright (c) 2025 Meta Platforms.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

/**
 * Load up a bunch of known values into registers
 * and expect them to show up in the core dump.
 * Value is register ABI name kinda spelled out,
 * followed by zeros to pad to 32 bits,
 * followed by FF00, followed by hex number of the register,
 * follwed by the "hex-coded-decimal" number of the register.
 */

GTEXT(main)
SECTION_FUNC(TEXT, main)

	li ra, 0xDADA0000FF000101

	/* SP is skipped because it can messes stuff up */

#ifndef CONFIG_RISCV_GP
	li gp, 0xDADA0000FF000101
#endif

#ifndef CONFIG_THREAD_LOCAL_STORAGE
	li tp, 0xE2E20000FF000404
#endif /* CONFIG_THREAD_LOCAL_STORAGE */

#ifndef CONFIG_TEST_RISCV_FATAL_PANIC
	/* We will load `0` (RV_ECALL_RUNTIME_EXCEPT) to `t0` */
	li t0, 0xD0FF0000FF000505
#endif /* CONFIG_TEST_RISCV_FATAL_PANIC */
	li t1, 0xD1FF0000FF000606
	li t2, 0xD2FF0000FF000707

#ifndef CONFIG_FRAME_POINTER
	li s0, 0xC0FF0000FF000808
#endif /* CONFIG_FRAME_POINTER */

	li s1, 0xC1FF0000FF000909

	li a0, 0xA0FF0000FF000A10
	li a1, 0xA1FF0000FF000B11
	li a2, 0xA2FF0000FF000C12
	li a3, 0xA3FF0000FF000D13
	li a4, 0xA4FF0000FF000E14
	li a5, 0xA5FF0000FF000F15

#ifndef CONFIG_RISCV_ISA_RV32E
	li a6, 0xA6FF0000FF001016
	li a7, 0xA7FF0000FF001117

	li s2, 0xC2FF0000FF001218
	li s3, 0xC3FF0000FF001319
	li s4, 0xC4FF0000FF001420
	li s5, 0xC5FF0000FF001521
	li s6, 0xC6FF0000FF001622
	li s7, 0xC7FF0000FF001723
	li s8, 0xC8FF0000FF001824
	li s9, 0xC9FF0000FF001925
	li s10, 0xC10FF000FF001A26
	li s11, 0xC11FF000FF001B27

	li t3, 0xD3FF0000FF001C28
	li t4, 0xD4FF0000FF001D29
	li t5, 0xD5FF0000FF001E30
	li t6, 0xD6FF0000FF001F31
#endif /* CONFIG_RISCV_ISA_RV32E */

#ifdef CONFIG_TEST_RISCV_FATAL_PANIC
	li a0, 4 /* K_ERR_KERNEL_PANIC */
	li t0, 0 /* RV_ECALL_RUNTIME_EXCEPT */
	ecall

#else /* CONFIG_TEST_RISCV_FATAL_ILLEGAL_INSTRUCTION */
	.insn 2, 0
	.insn 2, 0
#endif /* CONFIG_TEST_RISCV_FATAL_PANIC */
