/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * For the Xtensa simulator target, this code sets up the C calling context
 * and calls main()  (via __clibrary_start).
 * Control arrives here at _start from the reset vector or from crt0-app.S.
 */

#include <xtensa/simboard.h>
#include <xtensa/simcall.h>
#include <xtensa/coreasm.h>


/* Exports */
.global _start
.global __start

/*
 * Imports
 *   __clibrary_init	from C library (eg. newlib or uclibc)
 *   exit		from C library
 *   main		from user application
 *   __stack		from linker script (see LSP Ref Manual)
 */

.type	__clibrary_init, @function
.type	_Cstart, @function
.type	exit, @function


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

		.data
		.weak	_start_envp	 /* allow overriding */
		.align	4
_start_envp:	.word	0	/* empty environ */



	.text
	.align 4

_start:
__start:
	/*
	 *  _start is typically NOT at the beginning of the text segment --
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
	 *
	 */
	movi	a0, 0		 /* keep this register zero. */

#if XTOS_RESET_UNNEEDED
#include "reset-unneeded.S"
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
	 * reserve stack space for
	 *    - argv array
	 *    - argument strings
	 */
	movi	a2, SYS_iss_argv_size
	simcall	/* returns size of argv[] + its strings in a2 */
#if XCHAL_HAVE_PIF
	/*
	 * The stack only needs 16-byte alignment.
	 * However, here we round up the argv size further to 128 byte multiples
	 * so that in most cases, variations in argv[0]'s path do not result in
	 * different stack allocation.  Otherwise, such variations can impact
	 * execution timing (eg. due to cache effects etc) for the same code and data.
	 * If we have a PIF, it's more likely the extra required space is okay.
	 */
	addi	a2, a2, 127
	srli	a2, a2, 7
	slli	a2, a2, 7
#else
	/* Keep stack 16-byte aligned. */
	addi	a2, a2, 15
	srli	a2, a2, 4
	slli	a2, a2, 4
#endif
	/*
	 * No need to use MOVSP because we have no caller (we're the
	 * base caller); in fact it's better not to use MOVSP in this
	 * context, to avoid unnecessary ALLOCA exceptions and copying
	 * from undefined memory:
	 *   sub     a3, sp, a2
	 *   movsp   sp, a3
	 */
	sub	sp, sp, a2


	/*
	 *  Now that sp (a1) is set, we can set PS as per the application
	 *  (user vector mode, enable interrupts, enable window exceptions if applicable).
	 */
#if XCHAL_HAVE_EXCEPTIONS
# ifdef __XTENSA_CALL0_ABI__
	movi	a3, PS_UM /* PS.WOE = 0, PS.UM = 1, PS.EXCM = 0, PS.INTLEVEL = 0 */
# else
	movi	a3, PS_UM|PS_WOE /* PS.WOE = 1, PS.UM = 1, PS.EXCM = 0, PS.INTLEVEL = 0 */
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


	/*  The new ISS simcall only appeared after RB-2007.2:  */
#if !XCHAL_HAVE_BOOTLOADER && (XCHAL_HW_MAX_VERSION > XTENSA_HWVERSION_RB_2007_2)	/* pre-LX2 cores only */
	/*
	 *  Clear the BSS (uninitialized data) segments.
	 *  This code supports multiple zeroed sections (*.bss).
	 *  For speed, we clear memory using an ISS simcall
	 *  (see crt1-boards.S for more generic BSS clearing code).
	 */
	movi 	a6, _bss_table_start
	movi 	a7, _bss_table_end
	bgeu  	a6, a7, .Lnobss
.Lbssloop:
	movi	a2, SYS_memset
	l32i 	a3, a6, 0	 /* arg1 = fill start address */
	movi	a4, 0		 /* arg2 = fill pattern */
	l32i 	a5, a6, 4	 /* get end address */
	addi   	a6, a6, 8	 /* next bss table entry */
	sub	a5, a5, a3	 /* arg3 = fill size in bytes */
	simcall			 /* memset(a3,a4,a5) */
	bltu  	a6, a7, .Lbssloop	 /* loop until end of bss table */
.Lnobss:
#endif


	/*
	 *  Call __clibrary_init to initialize the C library:
	 *
	 *  void __clibrary_init(int argc, char ** argv, char ** environ,
	 *		void(*init_func)(void), void(*fini_func)(void));
	 */

	/* Get argv with the arguments from the ISS */
	mov	a3, sp		 /* tell simcall where to write argv[] */
	movi	a2, SYS_iss_set_argv
	simcall			 /* write argv[] array at a3 */

	movi	a2, SYS_iss_argc
	simcall			 /* put argc in a2 */


	/*  Call:   int _Cstart(); */
	CALL	_Cstart
	/*  The return value is the same register as the first outgoing argument. */
	CALL	exit
	/* Does not return here. */

	.size	_start, . - _start


/*
 * Local Variables:
 * mode:fundamental
 * comment-start: "/* "
 * comment-start-skip: "/* *"
 * End:
 */
