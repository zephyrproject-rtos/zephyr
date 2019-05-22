/*
 * Copyright (c) 2018 ispace, inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <offsets_short.h>
#include <arch/sparc/arch.h>

/* exports */
GTEXT(_isr_wrapper)
GTEXT(window_ofl_handler)
GTEXT(window_ufl_handler)
GTEXT(nmi_handler)
GTEXT(fault_handler)

/* imports */
GTEXT(_sw_isr_table)

.macro _fill_all_registers_except_o0_g7_for_debugging
	set 0xdeadbeef, %g1
	mov %g1, %g2
	mov %g1, %g3
	mov %g1, %g4
	mov %g1, %g5
	mov %g1, %g6

	mov %g1, %i0
	mov %g1, %i1
	mov %g1, %i2
	mov %g1, %i3
	mov %g1, %i4
	mov %g1, %i5
	mov %g1, %i6
	mov %g1, %i7

	mov %g1, %l0
	mov %g1, %l1
	mov %g1, %l2
	mov %g1, %l3
	mov %g1, %l4
	mov %g1, %l5
	mov %g1, %l6
	mov %g1, %l7

	/* mov %g1, %o0 */
	mov %g1, %o1
	mov %g1, %o2
	mov %g1, %o3
	mov %g1, %o4
	mov %g1, %o5
	mov %g1, %o6
	mov %g1, %o7
.endm

/** _isr_wrapper
 *
 * A wrapper function to call registered ISR.
 *
 * This is called from trap vectors.
 * Steps to do here:
 *  1) Save caller-saved registers on the current thread's stack.
 *  2) Save the stack pointer in switch_handle.
 *  3) Switch to ISR stack.
 *  4) Reserve a standard stack frame on the stack
 *  5) Call ISR (callee-saved register is saved here)
 *  6) Throwaway the standard stack frame on the stack
 *  7) Check to see we have to switch to another thread
 *  8a) If so,
 *       8a1) Save rest of the current thread's context (callee-saved registers)
	      on k_thread
 *       8a2) Restore callee-saved registers from k_thread
 *       8a3) Get a new thread as next
 *  8b) If not
 *       8b1) get the original thread as next
 *  9) Switch back to the next thread's stack
 *  10) Restore caller-saved registers from the current stack
 *  11) Resume the current thread at where it left off
 *
 *
 * Pre-conditions
 *  - Window register is shifted by 1
 *    - The original in and local registers are hidden
 *    - The original out registers are now at in registers
 *      - That is %sp is i6 at the time of entry
 *    - local and out is basically free to use except l0, l1, l2
 *  - Hardware saves PC in r17 (local 1), and nPC in r18 (local 2)
 *  - FIXME: Instructions at vector saves IRQ number in r16 (local 0)
 *  - Invalid %sp is not handle. (ref. "Register Windows and %sp" in
 *    Appx.D Software Considerations in The SPARC Architecture Manual Version 8)
 *  - Register usages:
 *    - local 0: IRQ number by vector
 *    - local 1: PC by hardware
 *    - local 2: nPC by hardware
 *    - local 3: PSR
 *    - local 4: Y
 *    - local 5: _kernel
 *    - local 6: _kernel.nested
 *
 *    - global 5: old thread
 *    - global 6: new thread
 *    - global 7: _kernel.current
 *
 * %sp is %o6
 * %fp is %i6
 */
SECTION_FUNC(text, _isr_wrapper)

	/* * Save caller-saved registers */
	/* ** Allocate a context stack frame on the stack */
	/*    (aka. Exception Stack Frame) */
	/*    This makes space for caller-saved registers on stack. */
	sub %i6, __z_arch_esf_t_SIZEOF, %sp

	/* ** Save caller-saved global registers on stack */
	st %g1, [%sp + _esf_reg(g1)]
	st %g2, [%sp + _esf_reg(g2)]
	st %g3, [%sp + _esf_reg(g3)]
	st %g4, [%sp + _esf_reg(g4)]
	st %g5, [%sp + _esf_reg(g5)]
	st %g6, [%sp + _esf_reg(g6)]
	st %g7, [%sp + _esf_reg(g7)]

	/* ** Save caller-saved out registers on stack */
	/*    Because the trap shifted the register window by 1, */
	/*    We access out registers through in registers. */
	st %i0, [%sp + _esf_reg(o0)]
	st %i1, [%sp + _esf_reg(o1)]
	st %i2, [%sp + _esf_reg(o2)]
	st %i3, [%sp + _esf_reg(o3)]
	st %i4, [%sp + _esf_reg(o4)]
	st %i5, [%sp + _esf_reg(o5)]
	/* No %sp.  We'll save it on switch_handle. */
	st %i7, [%sp + _esf_reg(o7)]

	/* ** Save special registers */
	/*    First load special registers in local registers in order */
	/*    to save them. Because the trap shifted register window, */
	/*    we can freely use local registers. */
                      /* We have */
                      /* IRQ number in l0, */
                      /* %PC in %l1, */
                      /* %npc in %l2, */
	mov %psr, %l3 /* Move %psr to %l3. */
	mov %y,   %l4 /* Move %y to %l4. */
	              /* No need to save %WIM. */
	              /* No need to save %TBR. */
	/*    Store them in the context frame. */
	st %l1, [%sp + _esf_reg(pc)]  /* Save %pc in pc. */
	st %l2, [%sp + _esf_reg(npc)] /* Save %npc in npc. */
	st %l3, [%sp + _esf_reg(psr)] /* Save %psr in psr. */
	st %l4, [%sp + _esf_reg(y)]   /* Save %y in y. */

	/* * Reserve standard stack frame on the stack */
	/*   Before calling any C function, we must have */
	/*   a standard stack frame on the stack for spilled registers. */
	/*   We don't have _kernel.current to save the current stack pointer */
	/*   when !MULTITHREADING.  Use the current stack and create */
	/*   a std stack frame on it. */
	sub %sp, __STD_STACK_FRAME_SIZEOF, %sp

	/* * Call ISR */
	/* ** Load the argument and the function */
	/*    They are in _isr_table_entry offset by %l0 */
	/*    (8 byte per entry). */
	set _sw_isr_table, %o1 /* Load the address of _sw_isr_table. */
	sll %l0, 3, %o2        /* Calculate the offset for irq. */
	ldd [%o1 + %o2], %o0   /* Load them to %o0 and %o1, respectively. */
	call %o1               /* Call func with the first arg in %o0. */
	nop                    /* Delay slot. */
	/* ISR is now done */
	/* ISR should have restored all callee-saved registers */
	/* (ins and locals). */

	/* * Throw away standard frame on the stack */
	/*   Get rid of the standard frame we reserved before ISR. */
	add %sp, __STD_STACK_FRAME_SIZEOF, %sp

	/* 5) Restore caller-saved registers from the current stack */
	/*    The sub numbers are reversed to match save procedures */
	/* 5-4) Restore out registers */
	/* Because we are in shifted window out registers are */
	/* accessible with in registers. */
	ld [%sp + _esf_reg(o0)], %i0
	ld [%sp + _esf_reg(o1)], %i1
	ld [%sp + _esf_reg(o2)], %i2
	ld [%sp + _esf_reg(o3)], %i3
	ld [%sp + _esf_reg(o4)], %i4
	ld [%sp + _esf_reg(o5)], %i5
	/* Do not touch %sp here.  We have already restored it above. */
	ld [%sp + _esf_reg(o7)], %i7

	/* 5-5) restore global registers */
	ld [%sp + _esf_reg(g1)], %g1
	ld [%sp + _esf_reg(g2)], %g2
	ld [%sp + _esf_reg(g3)], %g3
	ld [%sp + _esf_reg(g4)], %g4
	ld [%sp + _esf_reg(g5)], %g5
	ld [%sp + _esf_reg(g6)], %g6
	ld [%sp + _esf_reg(g7)], %g7

	/* 5-3) Restore special registers in local registers */
	/* first, move them to local registers */
	ld [%sp + _esf_reg(pc)], %l1
	ld [%sp + _esf_reg(npc)], %l2
	ld [%sp + _esf_reg(psr)], %l3
	ld [%sp + _esf_reg(y)], %l4

	/* 5-2) Then, actually restore special registers %y and %psr */
	/* No need to restore WIM nor TBR */
	/* First %y */
	mov %l4, %y
	nop
	nop
	nop

	/* PSR is special */
	/* We restore icc and PS. */
	/* PSR has impl, ver, icc, EC, EF, PIL, S, PS, ET, and CWP. */
	/*  - impl, ver, reserve: We can't change them. we don't touch them. */
	/*  - icc: Restore them. This is part of CPU context. */
	/*  - EC, EF: Not supported yet. Leave them alone. */
	/*  - PIL: Must be 0 when we get back to application. */
	/*  - S, ET: RETT instruction will restore. */
	/*  - PS: Set to the original PS and let RETT restore it. */
	/*  - CWP: Not supported yet.  RETT increments CWP by 1 */
	/* ET must be 0 before calling RETT */
	/* RETT sets ET=1, along with S=PS, PC=nPC, nPC=addr, CWP */
	/* impl:ver:icc:reserve:ec:ef:pil:s:ps:et:cwp */
	/* old_icc = old_psr & (PSR_ICC | PSR_PIL | PSR_PS) */
	/* current = current & ~(PSR_ICC | PSR_PIL | PSR_PS | PSR_ET) */
	/* All new thread comes here when it starts. Thus, it's important to */
	/* restore preserved bits from the context stack, */
	/* which are usually 0 though. */
	set PSR_ICC | PSR_PIL | PSR_PS, %l4 /* set mask to %l4 */
	and %l3, %l4, %l3         /* Take ICC, PIL and PS from preserved psr */
	rd %psr, %l5              /* Get the current psr */
	or %l4, PSR_ET, %l4       /* Add ET bit to the mask */
	andn %l5, %l4, %l5        /* Clear ICC, PIL, PS, and ET from the psr */
	wr %l5, %l3, %psr         /* Write back to psr with xor */
	nop                       /* psr is delayed write */
	nop
	nop

	/* 6) Resume the current thread at where it left off */
	/* RETT does does the following:
	 *  - ET <- 1
	 *  - PC <- nPC
	 *  - nPC <- address
	 *  - CWP <- new_cwp
	 *  - S <- PS
	 */
	jmp %l1
	rett %l2
	nop


window_ofl_handler:
	ba .
	nop
	nop
	nop

window_ufl_handler:
	ba .
	nop
	nop
	nop

nmi_handler:
	ba .
	nop
	nop
	nop

fault_handler:
	ba .
	nop
	nop
	nop
