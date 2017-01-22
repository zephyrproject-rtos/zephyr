/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * For most hardware / boards, this code sets up the C calling context
 * (setting up stack, PS, and clearing BSS) and jumps to __clibrary_start
 * which sets up the C library, calls constructors and registers destructors,
 * and calls main().
 *
 * Control arrives here at _start from the reset vector or from crt0-app.S.
 */

#include <xtensa/coreasm.h>
#include "xtos-internal.h"


/* Exports */
.global _start

/*
 * Imports
 *   __clibrary_init		from C library (eg. newlib or uclibc)
 *   exit			from C library
 *   main			from user application
 *   board_init			board-specific (uart/mingloss/tinygloss.c)
 *   xthal_dcache_all_writeback	from HAL library
 *   __stack			from linker script (see LSP Ref Manual)
 *   _bss_table_start		from linker script (see LSP Ref Manual)
 *   _bss_table_end		from linker script (see LSP Ref Manual)
 */

.type	main, @function

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


/**************************************************************************/

	.text
	.align 4
_start:
	/*
	 * _start is typically NOT at the beginning of the text segment --
	 *  it is always called from either the reset vector or other code
	 *  that does equivalent initialization (such as crt0-app.S).
	 *
	 *  Assumptions on entry to _start:
	 *	- low (level-one) and medium priority interrupts are disabled
	 *	  via PS.INTLEVEL and/or INTENABLE (PS.INTLEVEL is expected to
	 *	  be zeroed, to potentially enable them, before calling main)
	 *	- C calling context not initialized:
	 *	  - PS not initialized
	 *	  - SP not initialized
	 *	- the following are initialized:
	 *	  - LITBASE, cache attributes, WindowBase, WindowStart,
	 *	    CPENABLE, FP's FCR and FSR, EXCSAVE[n]

	 * Keep a0 zero.  It is used to initialize a few things.
	 * It is also the return address, where zero indicates
	 * that the frame used by _start is the bottommost frame.
	 */
#if !XCHAL_HAVE_HALT || !XCHAL_HAVE_BOOTLOADER		 /* not needed for Xtensa TX */
	movi	a0, 0		 /* keep this register zero. */
#endif

#if XTOS_RESET_UNNEEDED && !XCHAL_HAVE_HALT
#include "reset-unneeded.S"
#endif

	/*
	 * Initialize the stack pointer.
	 * See the "ABI and Software Conventions" chapter in the
	 * Xtensa ISA Reference manual for details.

	 * NOTE: Because the _start routine does not use any memory in its
	 * stack frame, and because all of its CALL instructions use a
	 * window size of 4 (or zero), the stack frame for _start can be empty.
	 */
	movi	sp, __stack

	/*
	 *  Now that sp (a1) is set, we can set PS as per the application
	 *  (user vector mode, enable interrupts, enable window exceptions if applicable).
	 */
#if XCHAL_HAVE_EXCEPTIONS
# ifdef __XTENSA_CALL0_ABI__
	movi	a3, PS_UM		/* PS.WOE = 0, PS.UM = 1, PS.EXCM = 0, PS.INTLEVEL = 0 */
# else
	movi	a3, PS_UM|PS_WOE	/* PS.WOE = 1, PS.UM = 1, PS.EXCM = 0, PS.INTLEVEL = 0 */
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
	s32i	a0, a8, 0	/* clear 4 bytes to make length multiple of 8 */
	addi	a8, a8, 4
.L1zte:	bbci.l	a10, 3, .L2zte
	s32i	a0, a8, 0	/* clear 8 bytes to make length multiple of 16 */
	s32i	a0, a8, 4
	addi	a8, a8, 8
.L2zte:	srli	a10, a10, 4	/* length is now multiple of 16, divide by 16 */
	floopnez	a10, clearzte
	s32i	a0, a8,	 0	/* clear 16 bytes at a time... */
	s32i	a0, a8,	 4
	s32i	a0, a8,	 8
	s32i	a0, a8, 12
	addi	a8, a8, 16
	floopend	a10, clearzte

	bltu  	a6, a7, .L0zte	/* loop until end of table of *.bss sections */
.L3zte:
#endif


	/*
	 * We can now call C code, the C calling environment has been initialized.
	 *
	 *  From this point on, we use ABI-specific macros to refer to registers a0 .. a15
	 *  (ARG#).
	 */

#if XCHAL_HAVE_HALT

	/*
	 * Assume minimalist environment for memory-constrained TX cores.
	 *  No C library or board initialization, no parameters passed to main
	 *  (assume declared as "void main(void)") and no call to exit().
	 */
	CALL	main
	halt

#else /* !HALT */

	.type	board_init, @function
	.type	__clibrary_init, @function
	.type	exit, @function


	 /*  Initialize the board (eg. UART, etc). */
	CALL	board_init

	/*
	 *  Call __clibrary_init to initialize the C library:
	 *
	 *  void __clibrary_init(int argc, char ** argv, char ** environ,
	 *		void(*init_func)(void), void(*fini_func)(void));
	 */

	 *  Pass an empty argv array, with an empty string as the program name. */

	movi	ARG1, _start_argc	/* argc address */
	movi	ARG2, _start_argv	/* argv = ["", 0] */
	movi	ARG3, _start_envp	/* envp = [0] */
	movi 	ARG4, _init		/* function that calls constructors */
	movi	ARG5, _fini		/* function that calls destructors */
	l32i	ARG1, ARG1, 0		/* argc = 1 */
	CALL	__clibrary_init

	/*  Call:   int main(int argc, char ** argv, char ** environ); */
	movi	ARG1, _start_argc	/* argc address */
	movi	ARG2, _start_argv	/* argv = ["", 0] */
	movi	ARG3, _start_envp	/* envp = [0] */
	l32i	ARG1, ARG1, 0		/* argc = 1 */
	CALL	main
	/*  The return value is the same register as the first outgoing argument. */
	CALL	exit			/* exit with main's return value */
	/* Does not return here. */

	.data
	/*
	 * Mark argc/argv/envp parameters as weak so that an external
	 * object file can override them.
	 */
	.weak	_start_argc, _start_argv, _start_envp
	.align	4
_start_argv:
	.word	_start_null	/* empty program name */
_start_null:
_start_envp:
	.word	0		/* end of argv array, empty string, empty environ */
_start_argc:
	.word	1		/* one argument (program name) */
	.text

#endif /* !HALT */

	.size	_start, . - _start

