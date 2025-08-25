/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */



.LC0:
    .long	_IFS0
    .long	_IFS1
    .long	_IFS2
    .long	_IFS3
    .long	_IFS4
    .long	_IFS5
    .long	_IFS6
    .long	_IFS7
    .long	_IFS8

    .section .text,code
    .section .isr.text._COMMONInterrupt,keep,code,keep
    .align 4
    .global __COMMONInterrupt	; export
    .type __COMMONInterrupt,@function
__COMMONInterrupt:
    .section .isr.text._COMMONInterrupt,keep,code,keep
    lnk #0x4
    mov.l w8, [w15++]
    mov.sl #__kernel,w8
    add.l [w8],#1,[w8]
    mov.w _INTTREGbits,w0
    and.l #(0x1ff&0xFFFF),w0
    sub.l w0,#9,w0
    mov.sl #__sw_isr_table,w1
    sl.l w0,#3,w0
    add.l w0,w1,w2
    mov.l [w2+4],w1
    mov.l [w2],w0


    call	w1
    call	_arch_dspic_irq_clear


    sub.l [w8],#1,w0
    mov.l w0,[w8]
    cp.l w0,#0
    bra	nz,.L2
    mov.sl #__kernel+20,w1
    mov.l [w1],w1
    mov.sl #__kernel+8,w0
    mov.l [w0],w0
    cp.l w1,w0
    bra	z,.L2
    cp0.b _z_sys_post_kernel
    bra	z,.L2
    mov.bz _INTCON1bits+1,w0
    bfext.l	#7,#1,w0,w0
    mov.l w0,[w14]
    bclr.b _INTCON1+1,#7
    mov.l [w14],w0
    mov.l [--w15], w8
    ulnk
    CTXTSWP #0x0
    mov.l w0, [w15++]
    mov.l sr, w0
    and #0xe0, w0
    bra nz, 1f
    mov.l [--w15], w0
    ulnk
    mov.l w2, [w15++]
    mov.l w1, [w15++]
    mov.l w0, [w15++]
    mov.l #_swap_working_set, w0
    mov.l [--w15], [w0++]
    mov.l [--w15], [w0++]
    mov.l [--w15], [w0]
    mov.l [--w15], w1
    mov.l sr, [w15++]
    mov.l w1, [w15++]
    mov.l [w0--], w2
    mov.l [w0--], w1
    mov.l [w0], w0
    mov.l w0, [w15++]
    1:
    mov.l [--w15], w0
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

    mov.l #__kernel, w0
    mov.l #0x8, w1
    add w0, w1, w1
    mov.l [w1], w2
    mov.l #0x28, w1
    add w2, w1, w1

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
    clr A
    clr B
    slac.l A, [W1++]
    sac.l A, [W1++]
    suac.l A, [W1++]
    slac.l B, [W1++]
    sac.l B, [W1++]
    suac.l B, [W1++]
    mov.l w15, [w1++]
    mov.l w14, [w1++]
    mov.l #SPLIM, w2
    mov.l [w2], [w1++]

    mov.sl	#__kernel+20,w0
    mov.l	[w0],w0
    mov.sl	#__kernel,w1
    mov.l	w0,[w1+8]
    mov.l	[w0+300],w0
    rcall	__set_tls

    mov.l #__kernel, w0
    mov.l #0x8, w1
    add w0, w1, w1
    mov.l [w1], w2
    mov.l #0x28, w1
    add w2, w1, w1

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
    slac.l A, [W1++]
    sac.l A, [W1++]
    suac.l A, [W1++]
    slac.l B, [W1++]
    sac.l B, [W1++]
    suac.l B, [W1++]
    mov.l [w1++], w15
    mov.l [w1++], w14
    mov.l #SPLIM, w2
    mov.l [w1++], [w2]

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
    mov.l w0, [w15++]
    mov.l sr, w0
    and #0xe0, w0
    mov.l [--w15], w0
    bra nz, 1f
    mov.l w2, [w15++]
    mov.l w1, [w15++]
    mov.l w0, [w15++]
    mov.l #_swap_working_set, w0
    mov.l [--w15], [w0++]
    mov.l [--w15], [w0++]
    mov.l [--w15], [w0]
    mov.l [--w15], w1
    mov.l [--w15], w2
    mov.l w2, sr
    mov.l w1, [w15++]
    mov.l [w0--], w2
    mov.l [w0--], w1
    mov.l [w0], w0
    lnk #0x4
    1:
    nop
    cp.l w0,#0
    bra	z,.L1
    bset.b _INTCON1+1,#7
    retfie



.L2:
    mov.l [--w15], w8
    ulnk
.L1:
    retfie


    .section .text._arch_dspic_irq_clear,code
    .align	4
    .global _arch_dspic_irq_clear ; export
    .type _arch_dspic_irq_clear,@function
_arch_dspic_irq_clear:

    lnk	#36
    mov.w _INTTREGbits,w0
    mov.sl #.LC0,w1
    repeat #9-1
    mov.l [w1++],[w14++]
    sub.l #36,w14
    sub.l #36,w1
    and.l #(0x1ff&0xFFFF),w0
    sub.l w0,#9,w0
    lsr.l w0,#5,w1
    sl.l w1,#2,w1
    add.l w1,w14,w1
    mov.l [w1],w2
    and.l w0,#(0x1f&0x7F),w0
    movs.l #0x1,w1
    sl.l w1,w0,w0
    com.l w0,w0
    and.l w0,[w2],[w2]
    ulnk
    return
