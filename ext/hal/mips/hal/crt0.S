/*
 * crt0.S -- startup file for MIPS.
 *
 * Copyright (c) 1995, 1996, 1997, 2001 Cygnus Support
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 */

/* This file does not use any floating-point ABI.  */
	.gnu_attribute 4,0
	.set nomips16

#include <mips/regdef.h>
#include <mips/m32c0.h>
#include "abiflags.S"

#define STARTUP_STACK_SIZE	0x0400	  /* Temporary stack size to run C code */

/* This is for referencing addresses that are not in the .sdata or
   .sbss section under embedded-pic, or before we've set up gp.  */
#ifdef __mips_embedded_pic
# ifdef __mips64
#  define LA(t,x) la t,x-PICBASE ; daddu t,s0,t
# else
#  define LA(t,x) la t,x-PICBASE ; addu t,s0,t
# endif
#else /* __mips_embedded_pic */
# if (defined (__mips64) && (defined (_MIPS_SIM) && _MIPS_SIM == _ABI64) \
      || defined (__mips_eabi))
#  define LA(t,x)	dla	t,x
#  define PTR_ADDU	daddu
#  define PTR_L		ld
# else
#  define LA(t,x)	la	t,x
#  define PTR_ADDU	addu
#  define PTR_L		lw
# endif
#endif /* __mips_embedded_pic */

#if (defined (__mips64) && (_MIPS_SIM == _ABI64 || _MIPS_SIM == _ABIO64 || \
     _MIPS_SIM == _ABIN32 || defined (__mips_eabi)))
# define REGSIZE 8
# define LD ld
# define ST sd
#else
# define REGSIZE 4
# define LD lw
# define ST sw
#endif

	.section .startdata, "aw", @nobits
	.balign 16
	.space	STARTUP_STACK_SIZE
__lstack: # Points to the end of the stack
__ram_extent:
	.space 8

	.data

__temp_space:	   /* Temporary space to save arguments */
	.space	REGSIZE * 3

	.text
	.align	2

/* Without the following nop, GDB thinks __start is a data variable.
 * This is probably a bug in GDB in handling a symbol that is at the
 * start of the .text section.
 */
	nop

	.globl	hardware_hazard_hook .text
	.globl	__start
	.ent	__start
__start:
#ifdef __mips_embedded_pic
#define PICBASE start_PICBASE
	.set	noreorder
	PICBASE = .+8
        bal	PICBASE
	nop
	move	s0,$31
	.set	reorder
#endif
#if __mips<3
#  define STATUS_MASK (SR_CU1|SR_PE)
#else
/* Post-mips2 has no SR_PE bit.  */
#  ifdef __mips64
/* Turn on 64-bit addressing and additional float regs.  */
#    define STATUS_MASK (SR_CU1|SR_FR|SR_KX|SR_SX|SR_UX)
#  else
#    if __mips_fpr==32
#      define STATUS_MASK (SR_CU1)
#    else
/* Turn on additional float regs.  */
#      define STATUS_MASK (SR_CU1|SR_FR)
#    endif
#  endif
#endif

	/* Save argument registers */
	LA (t0, __temp_space)
	ST	a0, (REGSIZE * 0)(t0)
	ST	a1, (REGSIZE * 1)(t0)
	ST	a2, (REGSIZE * 2)(t0)

	/*
	 * Save k0, k1, ra and sp and register
	 * default exception handler.
	*/
	.weak	__register_excpt_handler
	LA	(t9, __register_excpt_handler)
	beqz	t9, 1f
	move	a0, ra		/* save ra */
	jalr	t9
	b	2f
1:
	/* Clear Cause register.  */
	mtc0	zero,C0_CAUSE
	nop
	move	v0,zero			/* Mask for C0_SR.  */
2:
	/* Read MIPS_abiflags structure and set status/config registers
	   accordingly.  */
	.weak	__MIPS_abiflags_start
	.weak	__MIPS_abiflags_end
	LA	(t0,__MIPS_abiflags_start)
	LA	(t1,__MIPS_abiflags_end)
	PTR_ADDU t1,t1,-24

	/* Branch to 1f is the .MIPS.abiflags section is not 24 bytes.  This
	   indicates it is either missing or corrupt.  */
	bne	t0,t1,1f

	/* Check isa_level.  */
	lbu	t1,ABIFlags_isa_level(t0)
	sltu	v1,t1,3			/* Is MIPS < 3?  */
	xori	t1,t1,64		/* Is MIPS64?  */
	beq	v1,zero,4f
	li	v1,SR_PE
	or	v0,v0,v1		/* Enable soft reset.  */
4:
	li	v1,(SR_KX|SR_SX|SR_UX)
	bne	t1,zero,5f
	or	v0,v0,v1		/* Enable extended addressing.  */
5:
	/* Check DSP,DSP2,MDMX ase. */
	lw      t1,ABIFlags_ases(t0)
	andi    t1,t1,(AFL_ASE_DSP|AFL_ASE_DSPR2|AFL_ASE_MDMX)
	li	v1,SR_MX
	beq	t1,zero,6f
	or	v0,v0,v1
6:
	/* Check fp_abi.  */
	lbu	t1,ABIFlags_fp_abi(t0)
	xori	t1,t1,Val_GNU_MIPS_ABI_FP_SOFT
	li	v1,SR_CU1
	beq	t1,zero,2f		/* Skip MSA and cpr1_size checks.  */
	or	v0,v0,v1		/* Enable co-processor 1.  */

	/* Check cpr1_size.  */
	lbu	t1,ABIFlags_cpr1_size(t0)
	xori	t1,t1,AFL_REG_64
	li	v1,SR_FR
	bne	t1,zero,3f
	or	v0,v0,v1		/* Enable 64-bit FPU registers.  */
3:
	/* Check MSA ase.  */
	lw	t1,ABIFlags_ases(t0)
	andi	t1,t1,AFL_ASE_MSA
	li	v1,SR_FR
	beq	t1,zero,2f
	or	v0,v0,v1		/* Enable 64-bit FPU registers.  */
	li	v1,CFG5_MSAEN
	.set	push
	.set	mips32
	mtc0	v1,C0_CONFIG,5		/* Enable MSA.  */
	.set	pop
	b	2f

1:
	/* MIPS_abiflags structure is not available.  Set status/config
	   registers based on flags defined by compiler.  */
#ifdef __mips_soft_float
	li	v0,(STATUS_MASK-(STATUS_MASK & SR_CU1))
#else
	li	v0,STATUS_MASK
#endif

2:
	/* Set C0_SR,  */
	mtc0	v0,C0_SR
	nop

	/* Avoid hazard from C0_SR changes.  */
	LA	(t0, hardware_hazard_hook)
	beq	t0,zero,2f
	jalr	t0
2:


/* Fix high bits, if any, of the PC so that exception handling doesn't get
   confused.  */
	LA (v0, 3f)
	jr	v0
3:
	LA (gp, _gp)				# set the global data pointer
	.end __start

/*
 * zero out the bss section.
 */
	.globl	_get_ram_info .text
	.globl	__stack
	.globl	__global
	.ent	zerobss
zerobss:
	LA (v0, _fbss)
	LA (v1, _end)
	beq	v0,v1,2f
1:
	PTR_ADDU v0,v0,4
	sw	zero,-4(v0)
	bne	v0,v1,1b
2:
	/* setup the stack pointer */
	LA (t0, __stack)			# is __stack set ?
	bne	t0,zero,4f

	LA (sp, __lstack)			# make a small stack so we can
						# run some C code
	li	a0,0				# no need for the ram base
	LA (a1, __ram_extent)			# storage for the extent of ram
	jal	_get_ram_range

	/* NOTE: a0[0] contains the last address+1 of memory. */
	LA (a0, __ram_extent)
	PTR_L	t0,0(a0)			# the extent of ram
	lw	$0,-4(t0)			# check for valid memory
	/* Allocate 32 bytes for the register parameters.  Allocate 16
	   bytes for a null argv and envp.  Round the result up to 64
	   bytes to preserve alignment.  */
4:
	PTR_ADDU t0,t0,-64
	move	sp,t0				# set stack pointer
	.end	zerobss

/*
 * initialize target specific stuff. Only execute these
 * functions it they exist.
 */
	.globl	hardware_init_hook .text
	.globl	software_init_hook .text
	.globl	atexit .text
	.globl	exit .text
	.ent	init
init:
	LA (t9, hardware_init_hook)		# init the hardware if needed
	beq	t9,zero,6f
	jalr	t9
6:
	LA (t9, software_init_hook)		# init the hardware if needed
	beq	t9,zero,7f
	jalr	t9
7:
	jal	atexit

#ifdef GCRT0
	.globl	_ftext
	.globl	_extext
	LA (a0, _ftext)
	LA (a1, _etext)
	jal	monstartup
#endif

	/* restore argument registers */
	LA (t0, __temp_space)
	LD	a0,(REGSIZE * 0)(t0)
	LD	a1,(REGSIZE * 1)(t0)
	LD	a2,(REGSIZE * 2)(t0)

	/* Convert pointers potentially */
	.weak	__convert_argv_pointers
	LA (t0, __convert_argv_pointers)
	beqz	t0, 1f
	jalr	t0
1:
	/* if a0 > 0 then we have arguments ready in a0 to a2 registers */
	bgtz	a0,.Lmain
	/* if a0 == 0 then no arguments have been set up */
	beqz	a0, 1f
	/* if a0 < -1 then we have undefined behaviour so assume no
	   arguments have been set up */
	slti	a0, a0, -1
	bnez	a0, 1f

	/* a0 == -1 */
	.weak	__getargs
	LA (t0, __getargs)
	beqz	t0, 1f
	jalr	t0				# get arguments
	b	.Lmain
1:
	/* no arguments */
	move	a0,zero				# set argc to 0
	PTR_ADDU a1,sp,32			# argv = sp + 32
	PTR_ADDU a2,sp,40			# envp = sp + 40
	ST	zero,(a1)			# argv[argc] = 0
	ST	zero,(a2)			# envp[0] = 0

.Lmain:
	jal	main				# call the program start function

	# fall through to the "exit" routine
	move	a0,v0				# pass through the exit code
	jal	exit				# call libc exit to run the G++
						# destructors
	.end	init


/* Assume the PICBASE set up above is no longer valid below here.  */
#ifdef __mips_embedded_pic
#undef PICBASE
#endif

/*
 * _exit -- Exit from the application. Normally we cause a user trap
 *          to return to the ROM monitor for another run. NOTE: This is
 *	    the only other routine we provide in the crt0.o object, since
 *          it may be tied to the "_start" routine. It also allows
 *          executables that contain a complete world to be linked with
 *          just the crt0.o object.
 */
	.globl	hardware_exit_hook .text
	.globl	_exit
	.ent _exit
_exit:
7:

	# save exit code
	LA (t0, __temp_space)
	ST	a0,0(t0)

#ifdef __mips_embedded_pic
	/* Need to reinit PICBASE, since we might be called via exit()
	   rather than via a return path which would restore old s0. */
#define PICBASE exit_PICBASE
	.set	noreorder
	PICBASE = .+8
	bal	PICBASE
	nop
	move	s0,$31
	.set	reorder
#endif
#ifdef GCRT0
	LA (t0, _mcleanup)
	jalr	t0
#endif
	LA (t0, hardware_exit_hook)
	beq	t0,zero,1f
	jalr	t0
1:

	# restore return value from main
	LA (t0, __temp_space)
	LD	a0,0(t0)

	.global __exit .text
	jal	__exit

	# break instruction can cope with 0xfffff, but GAS limits the range:
	break	1023
	b	7b				# but loop back just in-case
	.end _exit

/* Assume the PICBASE set up above is no longer valid below here.  */
#ifdef __mips_embedded_pic
#undef PICBASE
#endif

/* EOF crt0.S */
