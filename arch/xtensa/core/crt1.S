/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Control arrives here at _start from the reset vector or from crt0-app.S.
 */

#include <arch/xtensa/xtensa_rtos.h>

/* Exports */
.global _start

/*
 * Imports
 *   __stack			from linker script (see LSP Ref Manual)
 *   _bss_table_start		from linker script (see LSP Ref Manual)
 *   _bss_table_end		from linker script (see LSP Ref Manual)
 *   z_cstart		Entry point into Zephyr C domain
 *   __stack		from linker script (see LSP Ref Manual)
 */

.global __start
.type	z_cstart, @function
.type	z_mp_entry, @function


/* Macros to abstract away ABI differences */

#if __XTENSA_CALL0_ABI__
# define CALL	call0
# define CALLX	callx0
# define ARG1	a2	/* 1st outgoing call argument */
# define ARG2	a3	/* 2nd outgoing call argument */
# define ARG3	a4	/* 3rd outgoing call argument */
# define ARG4	a5	/* 4th outgoing call argument */
# define ARG5	a6	/* 5th outgoing call argument */
#else
# define CALL	call4
# define CALLX	callx4
# define ARG1	a6	/* 1st outgoing call argument */
# define ARG2	a7	/* 2nd outgoing call argument */
# define ARG3	a8	/* 3rd outgoing call argument */
# define ARG4	a9	/* 4th outgoing call argument */
# define ARG5	a10	/* 5th outgoing call argument */
#endif

	.text
	.align 4

_start:
	/*
	 *  _start is typically NOT at the beginning of the text segment --
	 *  it is always called from either the reset vector (__start) or other
	 *  code that does equivalent initialization (such as crt0-app.S).
	 *
	 *  Assumptions on entry to _start:
	 *	- low (level-one) and medium priority interrupts are disabled
	 *	  via PS.INTLEVEL and/or INTENABLE
	 *	- C calling context not initialized:
	 *	  - PS not initialized
	 *	  - SP not initialized
	 *	- the following are initialized:
	 *	  - LITBASE, cache attributes, WindowBase, WindowStart,
	 *	    CPENABLE, FP's FCR and FSR, EXCSAVE[n]

	 * Keep a0 zero.  It is used to initialize a few things.
	 * It is also the return address, where zero indicates
	 * that the frame used by _start is the bottommost frame.
	 *
	 */
/* not needed for Xtensa TX */
#if !XCHAL_HAVE_HALT || !XCHAL_HAVE_BOOTLOADER
	movi	a0, 0		 /* keep this register zero. */
#endif

	/*
	 * Initialize the stack pointer.
	 * See the "ABI and Software Conventions" chapter in the
	 * Xtensa ISA Reference manual for details.
	 *
	 * NOTE: Because the _start routine does not use any memory in its
	 * stack frame, and because all of its CALL instructions use a
	 * window size of 4, the stack frame for _start can be empty.
	 */
	movi	sp, __stack

	/*
	 *  Now that sp (a1) is set, we can set PS as per the application (user
	 *  vector mode, disable interrupts, enable window exceptions if
	 *  applicable).
	 */
#if XCHAL_HAVE_EXCEPTIONS
# ifdef __XTENSA_CALL0_ABI__
	/*
	 * PS.WOE = 0
	 * PS.UM = 1
	 * PS.EXCM = 0
	 * PS.INTLEVEL = XCHAL_EXCM_LEVEL
	 */
	movi	a3, PS_UM|PS_INTLEVEL(XCHAL_EXCM_LEVEL)
# else
	/*
	 * PS.WOE = 1
	 * PS.UM = 1
	 * PS.EXCM = 0
	 * PS.INTLEVEL = XCHAL_EXCM_LEVEL
	 */
	movi	a3, PS_UM|PS_WOE|PS_INTLEVEL(XCHAL_EXCM_LEVEL)
# endif
	wsr	a3, PS
	rsync
#endif


	/*
	 *  Do any initialization that affects the memory map, such as
	 *  setting up TLB entries, that needs to be done before we can
	 *  successfully clear BSS (e.g. if some BSS segments are in
	 *  remapped areas).
	 *
	 *  NOTE:  This hook works where the reset vector does not unpack
	 *  segments (see "ROM packing" in the LSP manual), or where
	 *  unpacking of segments is not affected by memory remapping.
	 *  If ROM unpacking is affected, TLB setup must be done in
	 *  assembler from the reset vector.
	 *
	 *  The __memmap_init() routine can be a C function, however it
	 *  does not have BSS initialized!  In particular, __memmap_init()
	 *  cannot set BSS variables, i.e. uninitialized global variables
	 *  (they'll be wiped out by the following BSS clear), nor can it
	 *  assume they are yet initialized to zero.
	 *
	 *  The __memmap_init() function is optional.  It is marked as a
	 *  weak symbol, so that it gets valued zero if not defined.
	 */
	.weak	__memmap_init
	movi	a4, __memmap_init
	beqz	a4, 1f
	CALLX	a4
1:

#if !XCHAL_HAVE_BOOTLOADER	/* boot loader takes care of zeroing BSS */

# ifdef __XTENSA_CALL0_ABI__
	/* Clear a0 again as possible CALLX to __memmap_init changed it. */
	movi	a0, 0
# endif

# if CONFIG_MP_NUM_CPUS > 1
	/* Only clear BSS when running on core 0 */
	rsr	a3, PRID
	extui	a3, a3, 0, 8	/* extract core ID */
	bnez	a3, .L3zte
# endif

	/*
	 *  Clear the BSS (uninitialized data) segments.
	 *  This code supports multiple zeroed sections (*.bss).
	 *
	 *  Register allocation:
	 *	a0 = 0
	 *	a6 = pointer to start of table, and through table
	 *	a7 = pointer to end of table
	 *	a8 = start address of bytes to be zeroed
	 *	a9 = end address of bytes to be zeroed
	 *	a10 = length of bytes to be zeroed
	 */
	movi 	a6, _bss_table_start
	movi 	a7, _bss_table_end
	bgeu  	a6, a7, .L3zte

.L0zte:	l32i 	a8, a6, 0	/* get start address, assumed multiple of 4 */
	l32i 	a9, a6, 4	/* get end address, assumed multiple of 4 */
	addi   	a6, a6, 8	/* next entry */
	sub	a10, a9, a8	/* a10 = length, assumed a multiple of 4 */
	bbci.l	a10, 2, .L1zte
	s32i	a0, a8, 0	/* clear 4 bytes to make len multiple of 8 */
	addi	a8, a8, 4
.L1zte:	bbci.l	a10, 3, .L2zte
	s32i	a0, a8, 0	/* clear 8 bytes to make len multiple of 16 */
	s32i	a0, a8, 4
	addi	a8, a8, 8
.L2zte:	srli	a10, a10, 4	/* len is now multiple of 16, divide by 16 */
	floopnez	a10, clearzte
	s32i	a0, a8,	 0	/* clear 16 bytes at a time... */
	s32i	a0, a8,	 4
	s32i	a0, a8,	 8
	s32i	a0, a8, 12
	addi	a8, a8, 16
	floopend	a10, clearzte

	bltu  	a6, a7, .L0zte	/* loop until end of table of *.bss sections */
.L3zte:

#endif /* !XCHAL_HAVE_BOOTLOADER */

#if CONFIG_MP_NUM_CPUS > 1
	/*
	 * z_cstart() is only for CPU #0.
	 * Other CPUs have different entry point.
	 */
	rsr     a3, PRID
	extui   a3, a3, 0, 8	/* extract core ID */
	beqz    a3, 2f
	CALL    z_mp_entry

2:
#endif

	/* Enter C domain, never returns from here */
	CALL	z_cstart

	.size	_start, . - _start
