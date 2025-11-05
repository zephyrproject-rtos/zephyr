/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel arch swap functions
 *
 * This file contains function helpers for dspic arch specific thread swap
 * helper functions
 */

#ifndef ZEPHYR_ARCH_DSPIC_INCLUDE_KERNEL_ARCH_SWAP_H_
#define ZEPHYR_ARCH_DSPIC_INCLUDE_KERNEL_ARCH_SWAP_H_

#include <zephyr/toolchain.h>
#include <zephyr/offsets.h>

#if defined(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK512MPS512)
#define RAM_END 0x00013FFC
#elif defined(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK128MC106)
#define RAM_END 0x00007FFC
#endif

#if defined(_ASMLANGUAGE)

.equ __kernel_current_OFFSET, __kernel + ___cpu_t_current_OFFSET
.equ __thread_t_arch_cpu_level_OFFSET, ___thread_t_arch_OFFSET + ___thread_arch_t_cpu_level_OFFSET
.equ __kernel_ready_q_cache_OFFSET, __kernel + ___kernel_t_ready_q_OFFSET + \
    ___ready_q_t_cache_OFFSET
.equ __thread_t_arch_swap_return_value_OFFSET, ___thread_t_arch_OFFSET + \
    ___thread_arch_t_swap_return_value_OFFSET
.equ __thread_t_arch_swapped_from_thread_OFFSET, ___thread_t_arch_OFFSET + \
    ___thread_arch_t_swapped_from_thread_OFFSET

.extern _swap_working_set

.macro z_dspic_save_caller_saved
    push RCOUNT
    push.l fsr
    push.l fcr
    mov.l w0, [w15++]
    mov.l w1, [w15++]
    mov.l w2, [w15++]
    mov.l w3, [w15++]
    mov.l w4, [w15++]
    mov.l w5, [w15++]
    mov.l w6, [w15++]
    mov.l w7, [w15++]
    push.l f0
    push.l f1
    push.l f2
    push.l f3
    push.l f4
    push.l f5
    push.l f6
    push.l f7
    lnk #0x4
.endm

.macro z_dspic_restore_caller_saved
    ulnk
    pop.l f7
    pop.l f6
    pop.l f5
    pop.l f4
    pop.l f3
    pop.l f2
    pop.l f1
    pop.l f0
    mov.l [--w15], w7
    mov.l [--w15], w6
    mov.l [--w15], w5
    mov.l [--w15], w4
    mov.l [--w15], w3
    mov.l [--w15], w2
    mov.l [--w15], w1
    mov.l [--w15], w0
    pop.l fcr
    pop.l fsr
    pop RCOUNT
.endm

.macro z_dspic_get_current_callee_saved
    /* Get the current thread callee_saved context
     * and store it in w1
     */
    mov.l #__kernel, w0
    mov.l [w0 + ___cpu_t_current_OFFSET], w2
    mov.l #___thread_t_callee_saved_OFFSET, w1
    add w2, w1, w1
.endm

.macro z_dspic_save_callee_saved
    z_dspic_get_current_callee_saved

    /*Save all callee saved registers*/
    mov.l w8, [w1++]
    mov.l w9, [w1++]
    mov.l w10, [w1++]
    mov.l w11, [w1++]
    mov.l w12, [w1++]
    mov.l w13, [w1++]
    mov.l w14, [w1++]

    mov.l f8, [w1++]
    mov.l f9, [w1++]
    mov.l f10, [w1++]
    mov.l f11, [w1++]
    mov.l f12, [w1++]
    mov.l f13, [w1++]
    mov.l f14, [w1++]
    mov.l f15, [w1++]
    mov.l f16, [w1++]
    mov.l f17, [w1++]
    mov.l f18, [w1++]
    mov.l f19, [w1++]
    mov.l f20, [w1++]
    mov.l f21, [w1++]
    mov.l f22, [w1++]
    mov.l f23, [w1++]
    mov.l f24, [w1++]
    mov.l f25, [w1++]
    mov.l f26, [w1++]
    mov.l f27, [w1++]
    mov.l f28, [w1++]
    mov.l f29, [w1++]
    mov.l f30, [w1++]
    mov.l f31, [w1++]

    mov.l #RCOUNT, w2
    mov.l [w2], [w1++]
    mov.l #CORCON, w2
    mov.l [w2], [w1++]
    mov.l #MODCON, w2
    mov.l [w2], [w1++]
    mov.l #XMODSRT, w2
    mov.l [w2], [w1++]
    mov.l #XMODEND, w2
    mov.l [w2], [w1++]
    mov.l #YMODSRT, w2
    mov.l [w2], [w1++]
    mov.l #YMODEND, w2
    mov.l [w2], [w1++]
    mov.l #XBREV, w2
    mov.l [w2], [w1++]

    slac.l A, [W1++]
    sac.l A, [W1++]
    suac.l A, [W1++]
    slac.l B, [W1++]
    sac.l B, [W1++]
    suac.l B, [W1++]

    mov.l #SPLIM, w2
    mov.l [w2], [w1++]
    mov.l w15, [w1++]
    mov.l w14, [w1++]
.endm

.macro z_dspic_restore_callee_saved
    z_dspic_get_current_callee_saved

    /*restore all callee saved registers*/
    mov.l [w1++], w8
    mov.l [w1++], w9
    mov.l [w1++], w10
    mov.l [w1++], w11
    mov.l [w1++], w12
    mov.l [w1++], w13
    mov.l [w1++], w14

    mov.l [w1++], f8
    mov.l [w1++], f9
    mov.l [w1++], f10
    mov.l [w1++], f11
    mov.l [w1++], f12
    mov.l [w1++], f13
    mov.l [w1++], f14
    mov.l [w1++], f15
    mov.l [w1++], f16
    mov.l [w1++], f17
    mov.l [w1++], f18
    mov.l [w1++], f19
    mov.l [w1++], f20
    mov.l [w1++], f21
    mov.l [w1++], f22
    mov.l [w1++], f23
    mov.l [w1++], f24
    mov.l [w1++], f25
    mov.l [w1++], f26
    mov.l [w1++], f27
    mov.l [w1++], f28
    mov.l [w1++], f29
    mov.l [w1++], f30
    mov.l [w1++], f31

    mov.l #RCOUNT, w2
    mov.l [w1++], [w2]
    mov.l #CORCON, w2
    mov.l [w1++], [w2]
    mov.l #MODCON, w2
    mov.l [w1++], [w2]
    mov.l #XMODSRT, w2
    mov.l [w1++], [w2]
    mov.l #XMODEND, w2
    mov.l [w1++], [w2]
    mov.l #YMODSRT, w2
    mov.l [w1++], [w2]
    mov.l #YMODEND, w2
    mov.l [w1++], [w2]
    mov.l #XBREV, w2
    mov.l [w1++], [w2]

    clr A
    clr B
    llac.l [W1++], A
    lac.l [W1++], A
    luac.l [W1++], A
    llac.l [W1++], B
    lac.l [W1++], B
    luac.l [W1++], B

    mov.l #SPLIM, w2
    mov.l [w1++], [w2]
    mov.l [w1++], w15
    mov.l [w1++], w14
.endm

.macro z_dspic_temp_backup_working_reg
    /*Backup the working reg W0-W2*/
    mov.l w2, [w15++]
    mov.l w1, [w15++]
    mov.l w0, [w15++]
    mov.l #_swap_working_set, w0
    mov.l [--w15], [w0++]
    mov.l [--w15], [w0++]
    mov.l [--w15], [w0]
.endm

.macro z_dspic_restore_working_reg
    mov.l [w0--], w2
    mov.l [w0--], w1
    mov.l [w0], w0
.endm

.macro z_dspic_stack_thread_to_irq
    ulnk
    /*backup the working reg w0-w2*/
    z_dspic_temp_backup_working_reg
    /*Format SR and LR as its from interrupt*/
    mov.l [--w15], w1
    mov.l sr, [w15++]
    mov.l w1, [w15++]
    /*restore the working reg w0-w2*/
    z_dspic_restore_working_reg
.endm

.macro z_dspic_stack_irq_to_thread
    /*backup the working reg w0-w2*/
    z_dspic_temp_backup_working_reg
    /*Format SR and LR as its from interrupt*/
    mov.l [--w15], w1
    mov.l [--w15], w2
    mov.l w2, sr
    mov.l w1, [w15++]
    /*restore the working reg w0-w2*/
    z_dspic_restore_working_reg
    lnk #0x4
.endm

.macro z_dspic_do_swap
    /* Set the SPLIM to end of RAM
     * To avoid, stack overflow fault.
     */
	/* Switch to context 0 before starting context save and restore
	 * This Arch has 7 context and each has banked register sets for w0-w8
	 * So in interrupt we are in ctx 1 and need to go to ctx 0 for tasks
	 */
    ctxtswp #0x0

    /* Save the current context
     */
    z_dspic_stack_thread_to_irq
    z_dspic_save_caller_saved
    z_dspic_save_callee_saved

    /* Switch to next task in queue
     * No need to save and restore working registers here
     * */
    mov.l #__kernel_ready_q_cache_OFFSET, w2
    mov.l [w2], w2
    mov.l #__kernel_current_OFFSET, w1
    mov.l w2, [w1]

#ifdef CONFIG_CURRENT_THREAD_USE_TLS
    /*Switch the TLS pointer for the task to be swapped*/
    mov.l [w2 + ___thread_t_tls_OFFSET], w0
    rcall __set_tls
#endif

    /* Restore next thread context
     */
    z_dspic_restore_callee_saved
    z_dspic_restore_caller_saved
    z_dspic_stack_irq_to_thread
.endm

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_DSPIC_INCLUDE_KERNEL_ARCH_SWAP_H_ */
