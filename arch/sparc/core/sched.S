/*
 * Copyright (c) 2018 ispace, inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <offsets_short.h>
#include <arch/sparc/arch.h>

/* exports */
GTEXT(arch_switch)

/* imports */
GDATA(_kernel)
GTEXT(z_thread_entry)

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


/**
 * @brief Save the context of the old thread and restore the context
 * of the given new thread.
 *
 * We allocate a context stack frame for saving the context of the old
 * thread, instead of a standard stack frame a normal function uses.
 * This is the same format used by interrupted threads.  For the
 * context stack frame, we use struct __esf.
 *
 * void arch_switch(void *switch_handle, void **switch_handle);
 *
 * This function is called by do_swap() as:
 *    z_arch_swtich(new_thread->switch_handle, &old_thread->switch_handle);
 *
 * The first argument is the switch_handle of a new thread we are
 * switching to.  The second argument is the address of the
 * switch_handle of the old thread we are saving.  Note that
 * _kernel.current, or _current, points to the new thread when you get
 * here.  When you need to access the old thread, you have to use the
 * second argument, %o1, to get the address of
 * old_thread->callee_saved.
 *
 * The bottom half of this function, where new thread is restored, is
 * the same as _isr_wrapper.  We haven't merged them just yet.
 */
arch_switch:
	/* * Save old thread's context */
	/* ** Allocate a context stack frame */
        /*    (aka. Exception Stack Frame) on the stack */
	/*    Make space for caller-saved registers on stack */
	sub %sp, __z_arch_esf_t_SIZEOF, %sp

	/* ** Save caller-saved global registers on stack */
	st %g1, [%sp + _esf_reg(g1)]
	st %g2, [%sp + _esf_reg(g2)]
	st %g3, [%sp + _esf_reg(g3)]
	st %g4, [%sp + _esf_reg(g4)]
	st %g5, [%sp + _esf_reg(g5)]
	st %g6, [%sp + _esf_reg(g6)]
	st %g7, [%sp + _esf_reg(g7)]

	/* ** Save caller-saved out registers on stack */
	st %o0, [%sp + _esf_reg(o0)]
	st %o1, [%sp + _esf_reg(o1)]
	st %o2, [%sp + _esf_reg(o2)]
	st %o3, [%sp + _esf_reg(o3)]
	st %o4, [%sp + _esf_reg(o4)]
	st %o5, [%sp + _esf_reg(o5)]
	/* No %sp.  We'll save it on switch_handle. */
	st %o7, [%sp + _esf_reg(o7)]

	/* ** Save special registers */
	/*    We use _trampoline to go back to where we can from. */
	set _trampoline, %g1  /* Move _trampoline to %g1. */
	add %g1, 4, %g2       /* Move _trampoline + 4 to %g2. */
	mov %psr, %g3         /* Move %psr to %g3. */
	mov %y,   %g4         /* Move %y to %g4. */
	                      /* No need to save %WIM. */
	                      /* No need to save %TBR. */
	/* ** Save them in the context frame. */
	st %g1, [%sp + _esf_reg(pc)]  /* Save _trampoline in pc. */
	st %g2, [%sp + _esf_reg(npc)] /* Save _trampoline + 4 in npc */
	st %g3, [%sp + _esf_reg(psr)] /* Save %psr in psr. */
	st %g4, [%sp + _esf_reg(y)]   /* Save %y in y. */

	/* * Save the stack pointer in switch_handle */
	/*   old_thread->swtich_handle is pointed by */
	/*   the second argument, %o1. */
	st %sp, [%o1] /* Store %sp on switch_handle. */

	/* * Prepare saving callee-saved in and local registers */
	/*   Get the old thread's struct thread address from switch_handle. */
	/*   The address is used for thread.callee_saved */
	sub %o1, _switch_handle, %g5 /* Calculate the old thread address */

	/* * Save callee-saved in and local registers */
	/*   We have to save the callee-saved in and local */
	/*   registers we haven't saved. */
	/*   Save callee-saved registers on the k_thread.callee_saved. */
	/*   We have _kernel.current in %g5 */
	st %l0, [%g5 + _thread_offset_to_callee_saved_reg(l0)]
	st %l1, [%g5 + _thread_offset_to_callee_saved_reg(l1)]
	st %l2, [%g5 + _thread_offset_to_callee_saved_reg(l2)]
	st %l3, [%g5 + _thread_offset_to_callee_saved_reg(l3)]
	st %l4, [%g5 + _thread_offset_to_callee_saved_reg(l4)]
	st %l5, [%g5 + _thread_offset_to_callee_saved_reg(l5)]
	st %l6, [%g5 + _thread_offset_to_callee_saved_reg(l6)]
	st %l7, [%g5 + _thread_offset_to_callee_saved_reg(l7)]

	st %i0, [%g5 + _thread_offset_to_callee_saved_reg(i0)]
	st %i1, [%g5 + _thread_offset_to_callee_saved_reg(i1)]
	st %i2, [%g5 + _thread_offset_to_callee_saved_reg(i2)]
	st %i3, [%g5 + _thread_offset_to_callee_saved_reg(i3)]
	st %i4, [%g5 + _thread_offset_to_callee_saved_reg(i4)]
	st %i5, [%g5 + _thread_offset_to_callee_saved_reg(i5)]
	st %i6, [%g5 + _thread_offset_to_callee_saved_reg(i6)]
	st %i7, [%g5 + _thread_offset_to_callee_saved_reg(i7)]

	/*******************************************************
	 * At this point, the old thread context is all saved. *
	 *******************************************************/

	_fill_all_registers_except_o0_g7_for_debugging

	/* 4) Get new thread */
	/*    FIXME: does anyone have better idea to get new thread's struct? */
	set _kernel, %g1
	ld [%g1 + ___cpu_t_current_OFFSET], %g7

	/* 5) Load the new thread's context */
	/*    We have new thread in %g7 */
	/* 5-3) Restore callee-saved registers from the k_thread */
	ld [%g7 + _thread_offset_to_callee_saved_reg(i0)], %i0
	ld [%g7 + _thread_offset_to_callee_saved_reg(i1)], %i1
	ld [%g7 + _thread_offset_to_callee_saved_reg(i2)], %i2
	ld [%g7 + _thread_offset_to_callee_saved_reg(i3)], %i3
	ld [%g7 + _thread_offset_to_callee_saved_reg(i4)], %i4
	ld [%g7 + _thread_offset_to_callee_saved_reg(i5)], %i5
	ld [%g7 + _thread_offset_to_callee_saved_reg(i6)], %i6
	ld [%g7 + _thread_offset_to_callee_saved_reg(i7)], %i7

	ld [%g7 + _thread_offset_to_callee_saved_reg(l0)], %l0
	ld [%g7 + _thread_offset_to_callee_saved_reg(l1)], %l1
	ld [%g7 + _thread_offset_to_callee_saved_reg(l2)], %l2
	ld [%g7 + _thread_offset_to_callee_saved_reg(l3)], %l3
	ld [%g7 + _thread_offset_to_callee_saved_reg(l4)], %l4
	ld [%g7 + _thread_offset_to_callee_saved_reg(l5)], %l5
	ld [%g7 + _thread_offset_to_callee_saved_reg(l6)], %l6
	ld [%g7 + _thread_offset_to_callee_saved_reg(l7)], %l7

	/* Now we have in and local registers ready. */
	/* Shift window register to save them, */
	/* so that we can use some registers. */

	save
	/* We have local and out registers free to use. */

	/* 5) Switch to the next thread's stack */
	mov %i0, %sp

	/* Restore %sp (%o6) for returning thread, which is %i6. */
	/* Get rid of the context stack for the returning thread. */
	/* We still want to access %sp until we finish restoring, */
	/* so keep %sp */
	add %sp, __z_arch_esf_t_SIZEOF, %i6

	/* 5) Restore caller-saved registers from the current stack */
	/*    The sub numbers are reversed to match save procedures */
	/* 5-4) Restore out registers */
	/* Because we are in shifted window out registers are */
	/* accessible with in registers. */
	ld [%sp + __z_arch_esf_t_o0_OFFSET], %i0
	ld [%sp + __z_arch_esf_t_o1_OFFSET], %i1
	ld [%sp + __z_arch_esf_t_o2_OFFSET], %i2
	ld [%sp + __z_arch_esf_t_o3_OFFSET], %i3
	ld [%sp + __z_arch_esf_t_o4_OFFSET], %i4
	ld [%sp + __z_arch_esf_t_o5_OFFSET], %i5
	/* Do not touch %sp here.  We have already restored it above. */
	ld [%sp + __z_arch_esf_t_o7_OFFSET], %i7

	/* 5-5) restore global registers */
	ld [%sp + __z_arch_esf_t_g1_OFFSET], %g1
	ld [%sp + __z_arch_esf_t_g2_OFFSET], %g2
	ld [%sp + __z_arch_esf_t_g3_OFFSET], %g3
	ld [%sp + __z_arch_esf_t_g4_OFFSET], %g4
	ld [%sp + __z_arch_esf_t_g5_OFFSET], %g5
	ld [%sp + __z_arch_esf_t_g6_OFFSET], %g6
	ld [%sp + __z_arch_esf_t_g7_OFFSET], %g7

	/* 5-3) Restore special registers in local registers */
	/* first, move them to local registers */
	ld [%sp + __z_arch_esf_t_pc_OFFSET], %l1
	ld [%sp + __z_arch_esf_t_npc_OFFSET], %l2
	ld [%sp + __z_arch_esf_t_psr_OFFSET], %l3
	ld [%sp + __z_arch_esf_t_y_OFFSET], %l4

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
	wr %l5, %l3, %psr         /* write back to psr with xor */
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


_trampoline:
	retl
	nop
