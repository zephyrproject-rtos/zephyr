/*
 * Copyright (c) 2021  KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

.list
.section .text
GTEXT(__start)
__start :

/* during initialization (before the main thread is started), z_initialization_process_stacks
 * is used to do the kernel initialization.
 */
    mvtc    #(_z_initialization_process_stacks + CONFIG_INITIALIZATION_STACK_SIZE),USP

/* initialise interrupt stack pointer */
    mvtc    #(_z_interrupt_stacks + CONFIG_ISR_STACK_SIZE),ISP

/* set exception vector address (_ExceptVectors is defined in vects.c) */
#if CONFIG_HAS_EXCEPT_VECTOR_TABLE
    mvtc    #_ExceptVectors, extb
#endif

/* set interrupt vector address (_rvectors_start is defined in vects.c) */
    mvtc    #_rvectors_start, intb

/* load data section from ROM to RAM */

    mov     #_image_ram_start,r2        /* src ROM address of data section in R2 */
    mov     #__data_start,r1            /* dest start RAM address of data section in R1 */
    mov     #__data_region_end,r3       /* end RAM address of data section in R3 */
    sub    r1,r3                        /* size of data section in R3 (R3=R3-R1) */
#ifdef __RX_ALLOW_STRING_INSNS__
    smovf                   /* block copy R3 bytes from R2 to R1 */
#else
    cmp     #0, r3
    beq     2f

1:  mov.b   [r2+], r5
    mov.b   r5, [r1+]
    sub     #1, r3
    bne     1b
2:
#endif

/* bss initialisation: zero out bss */
    mov    #0,r2          /* load R2 reg with zero */
    mov    #_ebss, r3     /* store the end address of bss in R3 */
    mov    #_bss, r1      /* store the start address of bss in R1 */
    sub   r1,r3           /* size of bss section in R3 (R3=R3-R1) */
    sstr.b

#ifdef CONFIG_INIT_STACKS
	/* initialize the irq stack (it is located in the bss section) */
	mov    #0xaa,r2      			/* initialization value 0xaa */
	mov    #_z_interrupt_stacks, r1     	/* start address */
	mov    #CONFIG_ISR_STACK_SIZE, r3	/* stack size */
	sstr.b
#endif

/* setup PSW - use user stack register and lock interrupts during initialization */
    mvtc    #0x20000, psw

#ifdef CPPAPP
    bsr	   __rx_init
#endif

/* start user program */
    bsr    _z_cstart
    bsr    _exit

#ifdef CPPAPP
    .global    _rx_run_preinit_array
    .type    _rx_run_preinit_array,@function
_rx_run_preinit_array:
    mov    #__preinit_array_start,r1
    mov    #__preinit_array_end,r2
    mov    #_rx_run_inilist,r7
    jsr    r7

    .global    _rx_run_init_array
    .type    _rx_run_init_array,@function
_rx_run_init_array:
    mov    #__init_array_start,r1
    mov    #__init_array_end,r2
    mov    #4, r3
    mov    #_rx_run_inilist,r7
    jsr    r7

    .global    _rx_run_fini_array
    .type    _rx_run_fini_array,@function
_rx_run_fini_array:
    mov    #__fini_array_start,r2
    mov    #__fini_array_end,r1
    mov    #-4, r3
    /* fall through */

_rx_run_inilist:
next_inilist:
    cmp    r1,r2
    beq.b    done_inilist
    mov.l    [r1],r4
    cmp    #-1, r4
    beq.b    skip_inilist
    cmp    #0, r4
    beq.b    skip_inilist
    pushm    r1-r3
    jsr    r4
    popm    r1-r3
skip_inilist:
    add    r3,r1
    mov    #next_inilist,r7
    jsr    r7
done_inilist:
    rts

    .section    .init,"ax"
    .balign 4

    .global __rx_init
__rx_init:

    .section    .fini,"ax"
    .balign 4

    .global __rx_fini
__rx_fini:
    mov    #_rx_run_fini_array,r7
    jsr    r7

        .section .sdata
        .balign 4
        .global __gp
    .weak   __gp
__gp:

    .section .data
    .global ___dso_handle
    .weak   ___dso_handle
___dso_handle:
    .long    0

     .section        .init,"ax"
     mov    #_rx_run_preinit_array,r7
     jsr    r7
     mov    #_rx_run_init_array,r7
     jsr    r7
     rts

    .global __rx_init_end
__rx_init_end:

    .section        .fini,"ax"

    rts
    .global __rx_fini_end
__rx_fini_end:

#endif

/* call to exit*/
_exit:
    bra  _loop_here
_loop_here:
    bra _loop_here

    .text
    .end
