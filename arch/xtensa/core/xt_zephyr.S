/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa_context.h>
#include <xtensa_timer.h>
#include <offsets_short.h>
#include <kernel_structs.h>

	.extern _interrupt_stack
	.extern _kernel
#ifdef CONFIG_SYS_CLOCK_EXISTS
	.extern _timer_int_handler
#endif
	.set _interrupt_stack_top,  _interrupt_stack + CONFIG_ISR_STACK_SIZE

/*
 *	_zxt_dispatch(k_thread_t *_thread)
 */
	.text
	.globl  _zxt_dispatch
	.type   _zxt_dispatch,@function
	.align  4
_zxt_dispatch:
	mov a3, a2	/* a3 := _thread = _kernel.current */
	l32i sp, a3, THREAD_OFFSET(sp) /* SP := current->topOfStack; */

	/* Determine the type of stack frame. */
	l32i a2, sp, XT_STK_exit       /* exit dispatcher or solicited flag */
	bnez a2, .L_frxt_dispatch_stk

.L_frxt_dispatch_sol:
	/* Solicited stack frame. Restore retval from _Swap */
	l32i a2, a3, THREAD_OFFSET(retval)
	l32i a3, sp, XT_SOL_ps

#ifdef __XTENSA_CALL0_ABI__
	l32i a12, sp, XT_SOL_a12
	l32i a13, sp, XT_SOL_a13
	l32i a14, sp, XT_SOL_a14
	l32i a15, sp, XT_SOL_a15
#endif
	l32i a0, sp, XT_SOL_pc
#if XCHAL_CP_NUM > 0
	/* Ensure wsr.CPENABLE is complete (should be, it was cleared on entry). */
	rsync
#endif
	/* As soons as PS is restored, interrupts can happen. No need to sync PS. */
	wsr a3, PS
#ifdef __XTENSA_CALL0_ABI__
	addi sp, sp, XT_SOL_FRMSZ
	ret
#else
	retw
#endif

.L_frxt_dispatch_stk:

#if XCHAL_CP_NUM > 0
	/* Restore CPENABLE from task's co-processor save area. */
	l32i a2, a3, THREAD_OFFSET(cpStack)
	l16ui a3, a2, XT_CPENABLE
	wsr a3, CPENABLE
#endif
	/*
	 * Interrupt stack frame.
	 * Restore full context and return to exit dispatcher.
	 */
	call0   _xt_context_restore

	/* In Call0 ABI, restore callee-saved regs (A12, A13 already restored). */
#ifdef __XTENSA_CALL0_ABI__
	l32i    a14, sp, XT_STK_a14
	l32i    a15, sp, XT_STK_a15
#endif

#if XCHAL_CP_NUM > 0
	/* Ensure wsr.CPENABLE has completed. */
	rsync
#endif

	/*
	 * Must return via the exit dispatcher corresponding to the entrypoint from
	 * which this was called. Interruptee's A0, A1, PS, PC are restored and
	 * the interrupt stack frame is deallocated in the exit dispatcher.
	 */
	l32i    a0, sp, XT_STK_exit
	ret

/*
*******************************************************************************
*                                            _zxt_int_enter
*                                       void _zxt_int_enter(void)
*
* Implements the Xtensa RTOS porting layer's XT_RTOS_INT_ENTER function for
* freeRTOS. Saves the rest of the interrupt context (not already saved).
* May only be called from assembly code by the 'call0' instruction, with
* interrupts disabled.
* See the detailed description of the XT_RTOS_ENTER macro in xtensa_rtos.h.
*
*******************************************************************************
*/
	.globl  _zxt_int_enter
	.type   _zxt_int_enter,@function
	.align  4
_zxt_int_enter:

	/* Save a12-13 in the stack frame as required by _xt_context_save. */
	s32i a12, a1, XT_STK_a12
	s32i a13, a1, XT_STK_a13

	/* Save return address in a safe place (free a0). */
	mov a12, a0

	/* Save the rest of the interrupted context (preserves A12-13). */
	call0 _xt_context_save

	/*
	 * Save interrupted task's SP in TCB only if not nesting.
	 * Manage nesting directly rather than call the generic IntEnter() (in
	 * windowed ABI we can't call a C function here anyway because PS.EXCM is
	 * still set).
	 */
	movi a2, _kernel                   /* a2 := _kernel */
	l32i a3, a2, KERNEL_OFFSET(nested) /* a3 := _kernel->nested */
	addi a3, a3, 1                     /* increment nesting count */
	s32i a3, a2, KERNEL_OFFSET(nested) /* save nesting count */
	bnei a3, 1, .Lnested               /* !=0 before incr, so nested */

	l32i a3, a2, KERNEL_OFFSET(current)/* a3 := _kernel->current */
	s32i a1, a3, THREAD_OFFSET(sp)     /* save SP to Current top of stack */
	movi a1, _interrupt_stack_top      /* a1 = top of intr stack */
.Lnested:
1:
	mov a0, a12                        /* restore return addr and return */
	ret

/*
 *                                            _zxt_int_exit
 *                                       void _zxt_int_exit(void)
 *
 * Implements the Xtensa RTOS porting layer's XT_RTOS_INT_EXIT function for
 * Zephyr. If required, calls vPortYieldFromInt() to perform task context
 * switching, restore the (possibly) new task's context, and return to the
 * exit dispatcher saved in the task's stack frame at XT_STK_EXIT.
 * May only be called from assembly code by the 'call0' instruction. Does not
 * return to caller.
 * See the description of the XT_RTOS_ENTER macro in xtensa_rtos.h.
 *
 */
	.globl  _zxt_int_exit
	.type   _zxt_int_exit,@function
	.align  4
_zxt_int_exit:

	rsil a0, XCHAL_EXCM_LEVEL       	/* lock out interrupts */
	movi a2, _kernel
	l32i a3, a2, KERNEL_OFFSET(nested) /* _kernel->nested */
	addi a3, a3, -1						/* decrement nesting count */
	s32i a3, a2, KERNEL_OFFSET(nested) /* save nesting count */
	bnez a3, .Lnesting					/* !=0 after decr so still nested  */

	l32i a3, a2, KERNEL_OFFSET(current) /* a3 := _kernel->current */
	beqz a3, .Lnoswitch
	l32i a1, a3, THREAD_OFFSET(sp) /* SP - stack, a3 = current TCB */

	l32i a4, a3, ___thread_arch_t_flags_OFFSET
	movi a5, 0
	and a4, a4, a5
	beqz a4, .Lnoswitch
	movi a4, 0
	s32i a4, a3, 0                  /* zero out the flag for next time */

1:
	/*
	 * When using call0 ABI callee-saved registers a12-15 need to be saved
	 * before enabling preemption. They were already saved by _zxt_int_enter().
	*/
#ifdef __XTENSA_CALL0_ABI__
	s32i a14, a1, XT_STK_a14
	s32i a15, a1, XT_STK_a15
#endif

#if XCHAL_CP_NUM > 0
	l32i a4, a3, THREAD_OFFSET(cpStack)
	rsr a5, CPENABLE
	s16i a5, a4, XT_CPENABLE         /* cp_state->cpenable = CPENABLE; */
	movi a3, 0
	wsr a3, CPENABLE                /* disable all co-processors */
#endif

	l32i a2, a2, KERNEL_OFFSET(ready_q_cache)
	/*
	 * At this point, the a2 register contains the 'k_thread *' of the
	 * thread to be swapped in.
	 */
	call0   _zxt_dispatch				/* tail-call dispatcher */
	/* Never returns here. */

.Lnoswitch:
	/*
	If we came here then about to resume the interrupted task.
	*/

.Lnesting:
	/*
	 * We come here only if there was no context switch, that is if this
	 * is a nested interrupt, or the interrupted task was not preempted.
	 * In either case there's no need to load the SP.
	*/

	/* Restore full context from interrupt stack frame */
	call0 _xt_context_restore

	/*
	 * Must return via the exit dispatcher corresponding to the entrypoint
	 * from which this was called. Interruptee's A0, A1, PS, PC are restored
	 * and the interrupt stack frame is deallocated in the exit dispatcher.
	 */
	l32i a0, a1, XT_STK_exit
	ret

/*
 *                                           _zxt_timer_int
 *                                      void _zxt_timer_int(void)
 *
 * Implements Xtensa RTOS porting layer's XT_RTOS_TIMER_INT function.
 * Called every timer interrupt.
 * Manages the tick timer and calls xPortSysTickHandler() every tick.
 * See the detailed description of the XT_RTOS_ENTER macro in xtensa_rtos.h.
 * Callable from C.
 * Implemented in assmebly code for performance.
 *
 */
	.globl  _zxt_timer_int
	.type   _zxt_timer_int,@function
	.align  4
_zxt_timer_int:

	/*
	 * Xtensa timers work by comparing a cycle counter with a preset value.
	 * Once the match occurs an interrupt is generated, and the handler has to
	 * set a new cycle count into the comparator.
	 * To avoid clock drift due to interrupt latency, the new cycle count is
	 * computed from the old, not the time the interrupt was serviced. However
	 * if a timer interrupt is ever serviced more than one tick late, it is
	 * necessary to process multiple ticks until the new cycle count is in the
	 * future, otherwise the next timer interrupt would not occur until after
	 * the cycle counter had wrapped (2^32 cycles later).
	 *
	 * do {
	 *     ticks++;
	 *     old_ccompare = read_ccompare_i();
	 *     write_ccompare_i( old_ccompare + divisor );
	 *     service one tick;
	 *     diff = read_ccount() - old_ccompare;
	 * } while ( diff > divisor );
	 */

	ENTRY(16)
.L_xt_timer_int_catchup:
#ifdef CONFIG_SYS_CLOCK_EXISTS

#if CONFIG_XTENSA_INTERNAL_TIMER || (CONFIG_XTENSA_TIMER_IRQ < 0)
	/* Update the timer comparator for the next tick. */
#ifdef XT_CLOCK_FREQ
	movi a2, XT_TICK_DIVISOR  /* a2 = comparator increment          */
#else
	movi a3, _xt_tick_divisor
	l32i a2, a3, 0            /* a2 = comparator increment          */
#endif
	rsr a3, XT_CCOMPARE       /* a3 = old comparator value          */
	add a4, a3, a2            /* a4 = new comparator value          */
	wsr a4, XT_CCOMPARE       /* update comp. and clear interrupt   */
	esync
#endif /* CONFIG_XTENSA_INTERNAL_TIMER || (CONFIG_XTENSA_TIMER_IRQ < 0) */


#ifdef __XTENSA_CALL0_ABI__
	/* Preserve a2 and a3 across C calls. */
	s32i a2, sp, 4
	s32i a3, sp, 8
	/* TODO: movi a2, _xt_interrupt_table */
	movi a3, _timer_int_handler
	/* TODO: l32i a2, a2, 0 */
	callx0   a3
	/* Restore a2 and a3. */
	l32i a2, sp, 4
	l32i a3, sp, 8
#else
	/* TODO: movi a6, _xt_interrupt_table */
	movi a7, _timer_int_handler
	/* TODO: l32i a6, a6, 0 */
	callx4   a7
#endif

#if CONFIG_XTENSA_INTERNAL_TIMER || (CONFIG_XTENSA_TIMER_IRQ < 0)
	/* Check if we need to process more ticks to catch up. */
	esync          /* ensure comparator update complete  */
	rsr a4, CCOUNT /* a4 = cycle count                   */
	sub a4, a4, a3 /* diff = ccount - old comparator     */
	blt a2, a4, .L_xt_timer_int_catchup  /* repeat while diff > divisor */
#endif /* CONFIG_XTENSA_INTERNAL_TIMER || (CONFIG_XTENSA_TIMER_IRQ < 0) */

#endif

	RET(16)

/*
 *                                           _zxt_tick_timer_init
 *                                      void _zxt_tick_timer_init(void)
 *
 * Initialize timer and timer interrrupt handler (_xt_tick_divisor_init() has
 * already been been called).
 * Callable from C (obeys ABI conventions on entry).
 *
 */
	.globl  _zxt_tick_timer_init
	.type   _zxt_tick_timer_init,@function
	.align  4
_zxt_tick_timer_init:

	ENTRY(48)
#ifdef CONFIG_SYS_CLOCK_EXISTS
#if CONFIG_XTENSA_INTERNAL_TIMER || (CONFIG_XTENSA_TIMER_IRQ < 0)

	/* Set up the periodic tick timer (assume enough time to complete init). */
#ifdef XT_CLOCK_FREQ
	movi a3, XT_TICK_DIVISOR
#else
	movi a2, _xt_tick_divisor
	l32i a3, a2, 0
#endif
	rsr a2, CCOUNT              /* current cycle count */
	add a2, a2, a3              /* time of first timer interrupt */
	wsr a2, XT_CCOMPARE         /* set the comparator */

	/*
	Enable the timer interrupt at the device level. Don't write directly
	to the INTENABLE register because it may be virtualized.
	*/
#ifdef __XTENSA_CALL0_ABI__
	movi a2, XT_TIMER_INTEN
	call0  _xt_ints_on
#else
	movi a6, XT_TIMER_INTEN
	call4 _xt_ints_on
#endif

#endif
#endif /* CONFIG_XTENSA_INTERNAL_TIMER || (CONFIG_XTENSA_TIMER_IRQ < 0) */
	RET(48)

/*
 *                                        _zxt_task_coproc_state
 *                                   void _zxt_task_coproc_state(void)
 *
 * Implements the Xtensa RTOS porting layer's XT_RTOS_CP_STATE function.
 *
 * May only be called when a task is running, not within an interrupt handler
 * (returns 0 in that case).
 * May only be called from assembly code by the 'call0' instruction.
 * Does NOT obey ABI conventions.
 * Returns in A15 a pointer to the base of the co-processor state save area
 * for the current task.
 * See the detailed description of the XT_RTOS_ENTER macro in xtensa_rtos.h.
 *
 */
#if XCHAL_CP_NUM > 0

	.globl  _zxt_task_coproc_state
	.type   _zxt_task_coproc_state,@function
	.align  4
_zxt_task_coproc_state:
	movi a2, _kernel
	l32i a15, a2, KERNEL_OFFSET(nested)
	bnez a15, 1f
	l32i a2, a2, KERNEL_OFFSET(current)
	beqz a2, 1f
	l32i a15, a2, THREAD_OFFSET(cpStack)
	ret

1:	movi    a15, 0
2:	ret
#endif /* XCHAL_CP_NUM > 0 */

