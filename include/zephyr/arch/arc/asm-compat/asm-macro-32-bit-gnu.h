/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (C) 2021 Synopsys, Inc. (www.synopsys.com)
 *
 * Author: Vineet Gupta <vgupta@synopsys.com>
 *
 * ALU/Memory instructions pseudo-mnemonics for ARCv2 and ARC32 ISA
 */

.irp    cc,,.hi,.nz
.macro MOVR\cc d, s
	mov\cc \d, \s
.endm
.endr

.irp    aa,,.ab,.as,.aw
.macro LDR\aa d, s, off=0
	ld\aa  \d, [\s, \off]
.endm
.endr

.irp    aa,,.ab,.as,.aw
.macro STR\aa d, s, off=0
	; workaround assembler barfing for ST r, [@symb, 0]
	.if     \off == 0
		st\aa  \d, [\s]
	.else
		st\aa  \d, [\s, \off]
	.endif
.endm
.endr

.macro PUSHR r
	push \r
.endm

.macro POPR r
	pop \r
.endm

.macro LRR d, aux
	lr      \d, \aux
.endm

.macro SRR d, aux
	sr      \d, \aux
.endm

.irp    cc,,.nz
.macro ADDR\cc d, s, v
	add\cc \d, \s, \v
.endm
.endr

.irp    cc,,.nz
.macro ADD2R\cc d, s, v
	add2\cc  \d, \s, \v
.endm
.endr

.macro ADD3R d, s, v
	add3	\d, \s, \v
.endm

.macro SUBR d, s, v
	sub     \d, \s, \v
.endm

.macro BMSKNR d, s, v
	bmskn   \d, \s, \v
.endm

.macro LSRR d, s, v
	lsr	\d, \s, \v
.endm

.macro ASLR d, s, v
	asl \d, \s, \v
.endm

.macro ANDR d, s, v
	and \d, \s, \v
.endm

.macro ORR, d, s, v
	or \d, \s, \v
.endm

.irp    cc,ne,eq
.macro BRR\cc d, s, lbl
	br\cc  \d, \s, \lbl
.endm
.endr

.macro BREQR d, s, lbl
	breq \d, \s, \lbl
.endm

.macro CMPR op1, op2
	cmp \op1, \op2
.endm
