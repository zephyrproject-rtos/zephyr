/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _ASMLANGUAGE
#include <arch/nios2/asm.h>
#include <nano_private.h>
#include <offsets.h>

/* exports */
GTEXT(_Swap)
GTEXT(_thread_entry_wrapper)

/* unsigned int _Swap(unsigned int key)
 *
 * Always called with interrupts locked
 */
SECTION_FUNC(exception.other, _Swap)

	/* Get a reference to _nanokernel in r10 */
	movhi r10, %hi(_nanokernel)
	ori   r10, r10, %lo(_nanokernel)

	/* Get the pointer to nanokernel->current */
	ldw  r11, __tNANO_current_OFFSET(r10)

	/* Store all the callee saved registers. We either got here via
	 * an exception or from a cooperative invocation of _Swap() from C
	 * domain, so all the caller-saved registers have already been
	 * saved by the exception asm or the calling C code already.
	 */
	stw r16, __tTCS_coopReg_OFFSET + __t_coop_r16_OFFSET(r11)
	stw r17, __tTCS_coopReg_OFFSET + __t_coop_r17_OFFSET(r11)
	stw r18, __tTCS_coopReg_OFFSET + __t_coop_r18_OFFSET(r11)
	stw r19, __tTCS_coopReg_OFFSET + __t_coop_r19_OFFSET(r11)
	stw r20, __tTCS_coopReg_OFFSET + __t_coop_r20_OFFSET(r11)
	stw r21, __tTCS_coopReg_OFFSET + __t_coop_r21_OFFSET(r11)
	stw r22, __tTCS_coopReg_OFFSET + __t_coop_r22_OFFSET(r11)
	stw r23, __tTCS_coopReg_OFFSET + __t_coop_r23_OFFSET(r11)
	stw r28, __tTCS_coopReg_OFFSET + __t_coop_r28_OFFSET(r11)
	stw ra,  __tTCS_coopReg_OFFSET + __t_coop_ra_OFFSET(r11)
	stw sp,  __tTCS_coopReg_OFFSET + __t_coop_sp_OFFSET(r11)

	/* r4 has the 'key' argument which is the result of irq_lock()
	 * before this was called
	 */
	stw r4, __tTCS_coopReg_OFFSET + __t_coop_key_OFFSET(r11)

	/* FIXME call _sys_k_event_logger_context_switch */

	/* Find the next context to run. Choose _nanokernel.fiber
	 * if non-NULL */
	ldw r11, __tNANO_fiber_OFFSET(r10)
	beq r11, zero, not_fiber

	/* _nanokernel.fiber = _nanokernel.fiber->link */
	ldw r14, __tTCS_link_OFFSET(r11)
	stw r14, __tNANO_fiber_OFFSET(r10)
	br next_chosen

BRANCH_LABEL(not_fiber)
	/* Fiber was NULL, we'll choose nanokernel.task */
	ldw r11, __tNANO_task_OFFSET(r10)

BRANCH_LABEL(next_chosen)
	/* Set _nanokernel.current to value we chose for r11 */
	stw r11, __tNANO_current_OFFSET(r10)

	/* Restore callee-saved registers and switch to the incoming
	 * thread's stack
	 */
	ldw r16, __tTCS_coopReg_OFFSET + __t_coop_r16_OFFSET(r11)
	ldw r17, __tTCS_coopReg_OFFSET + __t_coop_r17_OFFSET(r11)
	ldw r18, __tTCS_coopReg_OFFSET + __t_coop_r18_OFFSET(r11)
	ldw r19, __tTCS_coopReg_OFFSET + __t_coop_r19_OFFSET(r11)
	ldw r20, __tTCS_coopReg_OFFSET + __t_coop_r20_OFFSET(r11)
	ldw r21, __tTCS_coopReg_OFFSET + __t_coop_r21_OFFSET(r11)
	ldw r22, __tTCS_coopReg_OFFSET + __t_coop_r22_OFFSET(r11)
	ldw r23, __tTCS_coopReg_OFFSET + __t_coop_r23_OFFSET(r11)
	ldw r28, __tTCS_coopReg_OFFSET + __t_coop_r28_OFFSET(r11)
	ldw ra,  __tTCS_coopReg_OFFSET + __t_coop_ra_OFFSET(r11)
	ldw sp,  __tTCS_coopReg_OFFSET + __t_coop_sp_OFFSET(r11)

	/* Load return value into r2 (return value register). garbage
	 * unless someone previously called fiberRtnValueSet(). Do this
	 * before we potentially unlock interrupts.
	 */
	ldw r2, __tTCS_coopReg_OFFSET + __t_coop_retval_OFFSET(r11)

	/* irq_unlock(fiber->coopReg.key);
	 * key was supplied as argument to _Swap()
	 */
	ldw r3, __tTCS_coopReg_OFFSET + __t_coop_key_OFFSET(r11)
	andi r3, r3, NIOS2_STATUS_PIE_MSK
	beq r3, zero, no_unlock
	rdctl r3, status
	ori r3, r3, NIOS2_STATUS_PIE_MSK
	wrctl status, r3

BRANCH_LABEL(no_unlock)

	ret


/* void _thread_entry_wrapper(void)
 */
SECTION_FUNC(TEXT, _thread_entry_wrapper)
	/* This all corresponds to struct init_stack_frame defined in
	 * thread.c. We need to take this stuff off the stack and put
	 * it in the apporpriate registers
	 */

	/* Can't return from here, just put NULL in ra */
	movi ra, 0

	/* Calling convention has first 4 arguments in registers r4-r7. */
	ldw  r4, 0(sp)
	ldw  r5, 4(sp)
	ldw  r6, 8(sp)
	ldw  r7, 12(sp)

	/* pop all the stuff that we just loaded into registers */
	addi sp, sp, 16

	call _thread_entry

