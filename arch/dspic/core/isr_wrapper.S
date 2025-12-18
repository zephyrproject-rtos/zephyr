/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/devicetree.h>
#include <offsets.h>
#include <kernel_arch_swap_macro.S>
#define OFFSET DT_PROP(DT_NODELABEL(intc0), if_offset)

.equ IF_OFFSET, OFFSET

    .section .text,code
    .section .isr.text._COMMONInterrupt,keep,code,keep
    .align 4
    .global __COMMONInterrupt	; export
    .type __COMMONInterrupt,@function
__COMMONInterrupt:
    .section .isr.text._COMMONInterrupt,keep,code,keep
    mov.sl #__irq_vector_table+4, w0
    subr.l w0, [--w15], w0
    lsr w0, #2, w0
    lnk #0x4
    /* Handle ISR calls and clear handles IRQ
     */
    mov.l w8, [w15++]
    mov.sl #__kernel + ___cpu_t_nested_OFFSET,w8
    add.l [w8],#1,[w8]

    mov.l w0, [w15++]
    mov.sl #__sw_isr_table,w1
    sl.l w0,#3,w0
    add.l w0,w1,w2
    mov.l [w2+4],w1
    mov.l [w2],w0

    call	w1
    mov.l [--w15], w0
    lsr.l   w0,#3,w1
    add.l   #IF_OFFSET,w1       ; Will not carry out, so sets SR.C = 0
    bsw.bC  [w1], w0            ; Instruction ignores w0[31:3] bits for .b size

    /* Check and perform context switch
     */
    sub.l [w8],#1,w0
    mov.l w0,[w8]
    mov.l [--w15], w8
    bra	nz,.L2
    mov.sl #__kernel + ___kernel_t_ready_q_OFFSET,w1
    mov.l [w1],w1
    mov.sl #__kernel + ___cpu_t_current_OFFSET,w0
    mov.l [w0],w0
    cp.l w1,w0
    bra	z,.L2
    cp0.b _z_sys_post_kernel
    bra	z,.L2
    ulnk
    CTXTSWP #0x0
    nop
    nop
    z_dspic_save_caller_saved
    z_dspic_save_callee_saved

    mov.l #__kernel_ready_q_cache_OFFSET, w2
    mov.l [w2], w2
    mov.l #__kernel_current_OFFSET, w1
    mov.l w2, [w1]

#ifdef CONFIG_CURRENT_THREAD_USE_TLS
    mov.l [w2 + ___thread_t_tls_OFFSET], w0
    rcall __set_tls
#endif

    z_dspic_restore_callee_saved
    z_dspic_restore_caller_saved

    mov.l w2, [w15++]
    mov.l w1, [w15++]
    mov.l w0, [w15++]
    mov.sl #__kernel_current_OFFSET, w0
    mov.l [w0], w1
    mov.l [w1 + __thread_t_arch_swapped_from_thread_OFFSET], w0
    mov.l #0, w2
    cp.l  w2, w0
    mov.l [--w15], w0
    bra  z, 2f
    mov.l [w1 + __thread_t_arch_swap_return_value_OFFSET], w0
    mov.l w2, [w1 + __thread_t_arch_swapped_from_thread_OFFSET]
    2:
    mov.l [--w15], w1
    mov.l [--w15], w2
    retfie

.L2:
    ulnk
    retfie
