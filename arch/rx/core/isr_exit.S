/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

.list
.section .text
GTEXT(_z_rx_irq_exit)

_z_rx_irq_exit:
    mov   #__kernel, r1                    ; Load the base address of _kernel into r1
    mov   r1, r3                           ; Load the base address of _kernel into r1

    add   #___cpu_t_current_OFFSET, r1     ; Add the offset for the 'current' field to r1
    mov   [r1], r2                         ; Load the value of _kernel.cpus[0].current into r2

    push r2                                ; Save old_thread to the stack

    ; Get the next thread to schedule
    mov #0,r1                         ; Use r1 to pass NULL since we haven't saved the context yet
    bsr _z_get_next_switch_handle     ; Call the function

    ; The return value of z_get_next_switch_handle will now be in r1
    ; Restore old_thread from the stack
    pop r2                            ; Restore old_thread from the stack

    ; Check if a switch is necessary
    cmp #0, r1
    bz no_switch                      ; If new_thread (in r1) is NULL, jump to no_switch

    add #___thread_t_switch_handle_OFFSET, r2

    ; Call arch_switch to perform the context switch
    bsr _z_rx_arch_switch             ; r1: new_thread->switch_handle, r2: old_thread->switch_handle

no_switch:
    rts
