/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/offsets.h>

;;This reset version support data initialization
;;Refer reset1.S for without data initialization

.section .text, code       ; Use .text to avoid section attribute errors
.weak __start,__user_init, __has_user_init, ___tls_crt_init
.extern __custom_data_init_extended
.extern __user_init

__start:
    ;; Initialize stack pointer and limit
    mov.l  #__SP_init, w15        ; Stack pointer (W15)
    mov.l  #__SPLIM_init, w14     ; Stack limit (W14)

    ;; Data initialization
    mov.l  #__dinit_tbloffset, w0
    cp0.l  w0
    bra    eq, 1f
    clr    w1
    rcall  __custom_data_init_extended
1:
    ;; Thread-local storage init (if available)
    mov.l  #___tls_crt_init, w0
    cp0.l  w0
    bra    eq, 1f
    rcall  ___tls_crt_init
1:
    mov.l    #__user_init,w0
    cp0.l    w0                ; user init functions
    bra      eq,1f
    call     __user_init
1:
    ;; Call the main application
    call   _z_prep_c
