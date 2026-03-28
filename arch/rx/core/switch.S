/*
 * Copyright (c) 2021  KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>

GTEXT(_z_rx_arch_switch)
GTEXT(_switch_isr_wrapper)

/* void z_rx_arch_switch(void *switch_to, void **switched_from)
 *
 * @brief switch between threads
 *
 * @param switch_to      (r1)  pointer to switch handle of the new thread
 * @param switched_from  (r2)  pointer to pointer to switch handle of the old
 *                             thread
 *
 * Thread-switching is treated differently depending on whether it is a
 * cooperative switch triggered by old thread itself or a preemptive switch
 * triggered by an interrupt (in this case the function has been called from an
 * ISR).
 */
.section .text._z_rx_arch_switch
.align 4
_z_rx_arch_switch:

	mvfc psw,r3
	tst #0x130000, r3		/* test if PM, U or I bit are set*/
	bz _z_rx_context_switch_isr	/* if none of them are set, this is an isr */

	mov #_coop_switch_to,r3
	mov r1,[r3]
	mov #_coop_switched_from,r3
	mov r2,[r3]

	/* trigger unconditional interrupt dedicated to thread switching. The content of r1 and r2
	 * will not change by invoking the interrupt so the parameters switch_to and switched_from
	 * will be available in _z_rx_context_switch_isr, which has been entered into the vector
	 * table as ISR for interrupt 1
	 */
	int #1

	/* at this point, r0 points to the entry point, so RTS will enter it */
	rts

/* void switch_isr_wrapper(void)
 *
 * @brief isr for interrupt 1 as wrapper for _z_rx_context_switch_isr
 *
 * _z_rx_context_switch_isr ends in rts, so it does not return from the interrupt context
 */
.section .text._switch_isr_wrapper
.align 4
_switch_isr_wrapper:
	pushm r1-r15

	/* Save the accumulator. */
	mvfachi r15 /* Accumulator high 32 bits. */
	push r15
	mvfacmi r15 /* Accumulator middle 32 bits. */
	shll #16, r15  /* Shifted left as it is restored to the low order word.*/
	push r15

	mov #_coop_switch_to,r3
	mov [r3],r1
	mov #_coop_switched_from,r3
	mov [r3],r2

	bsr _z_rx_context_switch_isr

	/* Restore the registers from the stack of the task pointed to by
	pxCurrentTCB. */
	pop r15
	mvtaclo r15 /* Accumulator low 32 bits. */
	pop r15
	mvtachi r15 /* Accumulator high 32 bits. */

	popm r1-r15
	rte

/* void z_rx_context_switch_isr(void *switch_to, void **switched_from)
 *
 * @brief switch between threads in the interrupt context
 *
 * @param switch_to      (r1)  pointer to switch handle of the new thread
 * @param switched_from  (r2)  pointer to pointer to switch handle of the old thread
 *
 * since this is part of an ISR, PSW, PC and general registers of the old thread are already
 * stored in the interrupt stack, so copy the corresponding part of the interrupt stack to the
 * stack of the interrupted thread
 */
_z_rx_context_switch_isr:

	/* store arguments switch_to and switched_from to registers r4 and r5 as
	 * registers r2 and r3 are needed for the smovf operation */
	mov r1,r4
	mov r2,r5

	/* set r2 (smovb source address) to the beginning of the interrupt stack */
	mov #(_z_interrupt_stacks + CONFIG_ISR_STACK_SIZE)-1,r2

	mvfc usp,r1	/* set r1 (smovb dest) to USP */

	sub #1,r1	/* correct by one byte to use smovb compared to push/pop */

	/* set r3 to number of bytes to move
	*  Accumulator 64bit (4byte * 2)
	*	15*4 byte for 15 general registers
	*	+ PSW (4 byte)
	*	+ PC (4 byte)
	*/
	mov #76,r3
	smovb		/* block copy from interrupt stack to old thread stack */

	add #1,r1	/* smovb leaves r1 pointing 1 byte before the stack */
	add #1,r2	/* same with r2 */

	mov r1,[r5]	/* store stack pointer of old thread in *switched_from */

	mov r2,r1	/* set r1 (smovf dest) to the beginning of the interrupt stack */

	mov r4,r2	/* set r2 (smovf source) to the sp of the new thread*/
	mov #76,r3	/* set r3 to number of bytes to move  */

	smovf		/* block copy from new thread stack to interrupt stack */

	mvtc r2,usp	/* set USP to the new thread stack */

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	bsr _z_thread_mark_switched_in
#endif

	rts
