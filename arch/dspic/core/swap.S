/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file swap.S
 *
 * @brief handles thread swap logic for dsPIC33A devices
 */
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <kernel_arch_swap_macro.S>

    .equ NUM_TEMP_REGS, 12

    .section .bss
    .align 4
    .globl _swap_working_set
_swap_working_set:
    .space NUM_TEMP_REGS*4

GTEXT(arch_swap)
SECTION_FUNC(TEXT, arch_swap)

    lnk #0x0
    mov.l w0, [w15++]

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
    rcall _z_thread_mark_switched_out
#endif
    mov.l [--w15], w2

    /* Get the kernel and thread structures
     * w0 -> current _thread_t structure
     */
    mov.l #__kernel_current_OFFSET, w0
    mov.l [w0], w1

    mov.l w2, [w1 + __thread_t_arch_cpu_level_OFFSET]

    /*move -EAGAIN to the member (2s compliment of 11)*/
    mov.l #0xfffffff5, w2
    mov.l w2, [w1 + __thread_t_arch_swap_return_value_OFFSET]
    mov.l #1, w2
    mov.l w2, [w1 + __thread_t_arch_swapped_from_thread_OFFSET]

    mov.l w1, w2
    mov.sl #__kernel_ready_q_cache_OFFSET, w0
    mov.l [w0], w1
    cp.l w1, w2
    bra z, 2f

    z_dspic_do_swap
    2:

    mov.l w1, [w15++]
    mov.l w2, [w15++]
    mov.l w0, [w15++]
#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
    rcall _z_thread_mark_switched_in
#endif
    mov.sl #__kernel_current_OFFSET, w0
    mov.l [w0], w1

    mov.l [w1 + __thread_t_arch_swapped_from_thread_OFFSET], w0
    cp.l  w0, #0
    mov.l #0, w0
    mov.l w0, [w1 + __thread_t_arch_swapped_from_thread_OFFSET]
    mov.l [--w15], w0
    bra z, 3f

    mov.l [w1 + __thread_t_arch_swap_return_value_OFFSET], w0
    3:
    mov.l [w1 + __thread_t_arch_cpu_level_OFFSET], w1
    cp.l w1, #0x0

    bra z, 2f
    bset.b 0x71, #0x7
    2:
    mov.l [--w15], w2
    mov.l [--w15], w1
    ulnk
    return
