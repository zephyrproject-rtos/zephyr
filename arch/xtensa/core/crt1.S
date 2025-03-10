/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Control arrives here at _start from the reset vector.
 */

#if defined(CONFIG_BOARD_XT_SIM) && defined(CONFIG_BOOTARGS)
#include <xtensa/simboard.h>
#include <xtensa/simcall.h>
#endif

#include <xtensa/coreasm.h>

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
.type	z_prep_c, @function


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
	 *  code that does equivalent initialization.
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

#if defined(CONFIG_BOARD_XT_SIM) && defined(CONFIG_BOOTARGS)
	/* Reserve stack space for
	 *	- argv array
	 *	- argument strings
	 */
	movi	a2, SYS_iss_argv_size
	simcall                       /* returns size of argv[] + its strings in a2 */

	/* The stack only needs 16-byte alignment.
	 * However, here we round up the argv size further to 128 byte multiples
	 * so that in most cases, variations in argv[0]'s path do not result in
	 * different stack allocation.  Otherwise, such variations can impact
	 * execution timing (eg. due to cache effects etc) for the same code and data.
	 * If we have an outbound bus, it's more likely there's enough memory.
	 */
# if XCHAL_HAVE_PIF
	addi	a2, a2, 127
	srli	a2, a2, 7
	slli	a2, a2, 7
# else
	/* Keep stack 16-byte aligned */
	addi	a2, a2, 15
	srli	a2, a2, 4
	slli	a2, a2, 4
# endif

	/* No need to use MOVSP because we have no caller (we're the base caller).
	 * In fact it's better not to use MOVSP in this context, to avoid unnecessary
	 * ALLOCA exceptions and copying from undefined memory.
	 */
	sub	sp, sp, a2
#endif /* CONFIG_BOARD_XT_SIM && CONFIG_BOOTARGS */

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

#if defined(CONFIG_BOARD_XT_SIM)
	/* Get argv with the arguments from the ISS */
	movi	a2, SYS_iss_set_argv
	mov	a3, sp			/* tell simcall where to write argv[] */
	simcall				/* write argv[] array at a3 */
	movi	a2, SYS_iss_argc
	simcall				/* put argc in a2 */
#if defined (__XTENSA_CALL0_ABI__)
	mov	a12, a2			/* save argc (a2 is ARG1) */
#else
	mov	ARG1, a2		/* argc */
#endif
	mov	ARG2, sp		/* argv */
	/* Save argc and argv */
	CALL	z_save_bootargs
#endif /* CONFIG_BOARD_XT_SIM */

	/* Enter C domain, never returns from here */
	CALL	z_prep_c

	.size	_start, . - _start
