/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file contains a full SPARC V8 trap table. The processor redirects
 * exection to the trap table on trap events. Each trap table entrys is four
 * instructions.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

#define BAD_TRAP \
	rd	%psr, %l0; \
	sethi	%hi(__sparc_trap_fault), %l4; \
	jmp	%l4+%lo(__sparc_trap_fault); \
	 rd	%tbr, %l6;

#define INTERRUPT_TRAP(level) \
	rd	%psr, %l0; \
	sethi	%hi(INT_HANDLER), %l4; \
	jmp	%l4+%lo(INT_HANDLER); \
	 mov	(0xf & level), %l3;

#define TRAP(handler) \
	rd	%psr, %l0; \
	sethi	%hi(handler), %l4; \
	jmp	%l4+%lo(handler); \
	 rd	%tbr, %l6;

#define RESET_TRAP(handler) \
	mov	%g0, %g4; \
	sethi	%hi(handler), %g4; \
	jmp	%g4+%lo(handler); \
	 nop;

/*
 * Generate the "trap in trap" condition which causes the processor to halt and
 * enter error mode. Typically used to stop a simulator (QEMU, TSIM) or leave
 * control to a hardware debug monitor (LEON DSU via GRMON).
 */
#define TRAP_IN_TRAP \
	ta	0x00; \
	nop; \
	nop; \
	nop;

#define SOFT_TRAP BAD_TRAP

#define WOF_TRAP                TRAP(__sparc_trap_window_overflow)
#define WUF_TRAP                TRAP(__sparc_trap_window_underflow)
#define FLW_TRAP                TRAP(__sparc_trap_flush_windows)
#define INT_HANDLER             __sparc_trap_interrupt

#ifdef CONFIG_IRQ_OFFLOAD
 #define IRQ_OFFLOAD_TRAP       TRAP(__sparc_trap_irq_offload)
#else
 #define IRQ_OFFLOAD_TRAP       BAD_TRAP
#endif

GTEXT(__sparc_trap_table)
GTEXT(__start)

SECTION_SUBSEC_FUNC(TEXT, traptable, __sparc_trap_table)
__start:
	/*
	 * Values in the range 0 to 0x5F that are not assigned in SPARC V8
	 * specification Table 7-1 are reserved for future versions of the
	 * architecture.
	 */
	RESET_TRAP(__sparc_trap_reset); ! 00 reset
	BAD_TRAP;                       ! 01 instruction_access_exception
	BAD_TRAP;                       ! 02 illegal_instruction
	BAD_TRAP;                       ! 03 priveleged_instruction
	BAD_TRAP;                       ! 04 fp_disabled
	WOF_TRAP;                       ! 05 window_overflow
	WUF_TRAP;                       ! 06 window_underflow
	BAD_TRAP;                       ! 07 mem_address_not_aligned
	BAD_TRAP;                       ! 08 fp_exception
	BAD_TRAP;                       ! 09 data_access_exception
	BAD_TRAP;                       ! 0A tag_overflow
	BAD_TRAP;                       ! 0B watchpoint_detected
	BAD_TRAP;                       ! 0C reserved
	BAD_TRAP;                       ! 0D reserved
	BAD_TRAP;                       ! 0E reserved
	BAD_TRAP;                       ! 0F reserved
	BAD_TRAP;                       ! 10 reserved

	/* Interrupt traps */
	INTERRUPT_TRAP(1);                      ! 11 interrupt_level_1
	INTERRUPT_TRAP(2);                      ! 12 interrupt_level_2
	INTERRUPT_TRAP(3);                      ! 13 interrupt_level_3
	INTERRUPT_TRAP(4);                      ! 14 interrupt_level_4
	INTERRUPT_TRAP(5);                      ! 15 interrupt_level_5
	INTERRUPT_TRAP(6);                      ! 16 interrupt_level_6
	INTERRUPT_TRAP(7);                      ! 17 interrupt_level_7
	INTERRUPT_TRAP(8);                      ! 18 interrupt_level_8
	INTERRUPT_TRAP(9);                      ! 19 interrupt_level_9
	INTERRUPT_TRAP(10);                     ! 1A interrupt_level_1
	INTERRUPT_TRAP(11);                     ! 1B interrupt_level_11
	INTERRUPT_TRAP(12);                     ! 1C interrupt_level_12
	INTERRUPT_TRAP(13);                     ! 1D interrupt_level_13
	INTERRUPT_TRAP(14);                     ! 1E interrupt_level_14
	INTERRUPT_TRAP(15);                     ! 1F interrupt_level_15
	BAD_TRAP;                               ! 20 r_register_access_error
	BAD_TRAP;                               ! 21 instruction_access_error
	BAD_TRAP;                               ! 22 reserved
	BAD_TRAP;                               ! 23 reserved
	BAD_TRAP;                               ! 24 cp_disabled
	BAD_TRAP;                               ! 25 unimplemented_FLUSH
	BAD_TRAP;                               ! 26 reserved
	BAD_TRAP;                               ! 27 reserved
	BAD_TRAP;                               ! 28 cp_exception
	BAD_TRAP;                               ! 29 data_access_error
	BAD_TRAP;                               ! 2A division_by_zero
	BAD_TRAP;                               ! 2B data_store_error
	BAD_TRAP;                               ! 2C data_access_MMU_miss
	BAD_TRAP;                               ! 2D reserved
	BAD_TRAP;                               ! 2E reserved
	BAD_TRAP;                               ! 2F reserved
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 30 - 33 reserved
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 34 - 37 reserved
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 38 - 3B reserved
	BAD_TRAP;                               ! 3C instruction_access_MMU_miss
	BAD_TRAP;                               ! 3D reserved
	BAD_TRAP;                               ! 3E reserved
	BAD_TRAP;                               ! 3F reserved
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 40 - 43 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 44 - 47 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 48 - 4B undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 4C - 4F undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 50 - 53 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 54 - 57 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 58 - 5B undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 5C - 5F undefined

	/*
	 * tt values 0x60 to 0x7F are reserved for implementation-dependent
	 * exceptions.
	 */
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 60 - 63 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 64 - 67 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 68 - 6B undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 6C - 6F undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 70 - 73 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 74 - 77 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 78 - 7B undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 7C - 7F undefined

	/* trap_instruction 0x80 - 0xFF */
	/* NOTE: "ta 5" can be generated by compiler. */
	TRAP_IN_TRAP;                                 !  0 System calls
	SOFT_TRAP;                                    !  1 Breakpoints
	SOFT_TRAP;                                    !  2 Division by zero
	FLW_TRAP;                                     !  3 Flush windows
	SOFT_TRAP;                                    !  4 Clean windows
	SOFT_TRAP;                                    !  5 Range checking
	SOFT_TRAP;                                    !  6 Fix alignment
	SOFT_TRAP;                                    !  7 Integer overflow
	SOFT_TRAP                                     !  8 System calls
	TRAP(__sparc_trap_sw_set_pil);                !  9 Reserved for the os
	SOFT_TRAP;                                    ! 10 Reserved for the os
	SOFT_TRAP;                                    ! 11 Reserved for the os
	/* See SPARC-ABI for purpose of the following software traps */
	SOFT_TRAP;                                    ! 12
	IRQ_OFFLOAD_TRAP;                             ! 13
	SOFT_TRAP;                                    ! 14
	TRAP(__sparc_trap_except_reason);             ! 15
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! 90 - 93
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! 94 - 97
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! 98 - 9B
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! 9C - 9F
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! A0 - A3
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! A4 - A7
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! A8 - AB
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! AC - AF
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! B0 - B3
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! B4 - B7
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! B8 - BB
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! BC - BF
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! C0 - C3
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! C4 - C7
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! C8 - CB
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! CC - CF
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! D0 - D3
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! D4 - D7
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! D8 - DB
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! DC - DF
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! E0 - E3
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! E4 - E7
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! E8 - EB
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! EC - EF
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! F0 - F3
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! F4 - F7
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! F8 - FB
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! FC - FF
