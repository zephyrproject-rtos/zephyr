/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

;;This version support without data initialization
;;Refer reset0.S for the data initialization

.section .text, code
.weak __start
__start:
    ;; Initialize stack pointer and limit
    mov.l  #__SP_init, w15        ; Stack pointer (W15)
    mov.l  #__SPLIM_init, w14     ; Stack limit (W14)

    mov.l    #__user_init,w0
    cp0.l    w0                ; user init functions
    bra      eq,1f
    call     __user_init
1:
    call   _z_prep_c              ; Call main application
