/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief kernel swapper code for Xtensa
 *
 * This module implements the __swap() routine for the Xtensa architecture.
 */

#include <xtensa_context.h>
#include <kernel_arch_data.h>
#include <offsets_short.h>

	.extern _kernel
/* unsigned int __swap (unsigned int basepri); */
	.globl  __swap
	.type   __swap,@function
	.align  4
__swap:
#ifdef __XTENSA_CALL0_ABI__
	addi    sp,  sp, -XT_SOL_FRMSZ
#else
	entry   sp,  XT_SOL_FRMSZ
#endif
	s32i    a0,  sp, XT_SOL_pc
	s32i    a2,  sp, XT_SOL_ps
#ifdef __XTENSA_CALL0_ABI__
	s32i    a12, sp, XT_SOL_a12         /* save callee-saved registers */
	s32i    a13, sp, XT_SOL_a13
	s32i    a14, sp, XT_SOL_a14
	s32i    a15, sp, XT_SOL_a15
#else
	/* Spill register windows. Calling xthal_window_spill() causes extra
	 * spills and reloads, so we will set things up to call the _nw version
	 * instead to save cycles.
	 */
	/* spills a4-a7 if needed */
	movi    a6,  ~(PS_WOE_MASK|PS_INTLEVEL_MASK)
	and     a2,  a2, a6                           /* clear WOE, INTLEVEL */
	addi    a2,  a2, XCHAL_EXCM_LEVEL             /* set INTLEVEL */
	wsr     a2,  PS
	rsync
	call0   xthal_window_spill_nw
	l32i    a2,  sp, XT_SOL_ps                    /* restore PS */
	addi    a2, a2, XCHAL_EXCM_LEVEL
	wsr     a2,  PS
#endif
#if XCHAL_CP_NUM > 0
	/* Save coprocessor callee-saved state (if any). At this point CPENABLE
	 * should still reflect which CPs were in use (enabled).
	 */
	call0   _xt_coproc_savecs
#endif
	movi a2, _kernel
	movi a3,  0
	l32i a4, a2, KERNEL_OFFSET(current) /* a4 := _kernel->current */

	s32i a3,  sp, XT_SOL_exit       /* 0 to flag as solicited frame */
	s32i sp,  a4, THREAD_OFFSET(sp) /* current->arch.topOfStack := sp */
	/*
	 * Set __swap()'s default return code to -EAGAIN. This eliminates the
	 * need for the timeout code to set it itself.
	 */
	movi a3, -11 /* a3 := -EAGAIN. TODO: Use a macro here insted of 11 */
	s32i a3, a4, THREAD_OFFSET(retval) /* current->arch.retval := -EAGAIN */

#if XCHAL_CP_NUM > 0
	/* Clear CPENABLE, also in task's co-processor state save area. */
	movi    a3,  0
	/* a4 = _kernel->current */
	wsr     a3,  CPENABLE
	s16i    a3,  a4, THREAD_OFFSET(cpEnable) /* clear saved cpenable */
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH
	/* Register the context switch */
#ifdef __XTENSA_CALL0_ABI__
	call0 _sys_k_event_logger_context_switch
#else
	call4 _sys_k_event_logger_context_switch
#endif
#endif
	/* _thread := _kernel.ready_q.cache */
	l32i a3, a2, KERNEL_OFFSET(ready_q_cache)
	/*
	 * Swap threads if any is to be swapped in.
	 */
	call0   _zxt_dispatch /* (_kernel@a2, _thread@a3) */
	/* Never reaches here. */

