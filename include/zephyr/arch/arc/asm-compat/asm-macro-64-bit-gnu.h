/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (C) 2021 Synopsys, Inc. (www.synopsys.com)
 *
 * Author: Vineet Gupta <vgupta@synopsys.com>
 *
 * pseudo-mnemonics for ALU/Memory instructions for ARC64 ISA
 */

.irp    cc,,.hi,.nz
.macro MOVR\cc d, s
	movl\cc   \d, \s
.endm
.endr

.irp    aa,,.ab,.as,.aw
.macro LDR\aa d, s, off=0
	ldl\aa \d, [\s, \off]
.endm
.endr

.irp    aa,.ab,.as,.aw
.macro STR\aa d, s, off=0
	; workaround assembler barfing for ST r, [@symb, 0]
	.if \off == 0
		stl\aa \d, [\s]
	.else
		stl\aa \d, [\s, \off]
	.endif
.endm
.endr

.macro STR d, s, off=0
	.if \off == 0
		stl \d, [\s]
	.else
		.if \off > 256
			STR.as \d, \s, \off / 8
		.else
			stl    \d, [\s, \off]
		.endif
	.endif
.endm

.macro PUSHR r
	pushl   \r
.endm

.macro POPR r
	popl    \r
.endm

.macro LRR d, aux
	lrl     \d, \aux
.endm

.macro SRR d, aux
	srl      \d, \aux
.endm

.irp    cc,,.nz
.macro ADDR\cc d, s, v
	addl\cc  \d, \s, \v
.endm
.endr

.irp    cc,,.nz
.macro ADD2R\cc d, s, v
	add2l\cc \d, \s, \v
.endm
.endr

.macro ADD3R d, s, v
	add3l \d, \s, \v
.endm

.macro SUBR d, s, v
	subl     \d, \s, \v
.endm

.macro BMSKNR d, s, v
	bmsknl   \d, \s, \v
.endm

.macro LSRR d, s, v
	lsrl	\d, \s, \v
.endm

.macro ASLR d, s, v
	asll \d, \s, \v
.endm

.macro ANDR d, s, v
	andl \d, \s, \v
.endm

.macro ORR, d, s, v
	orl \d, \s, \v
.endm

.irp    cc,ne,eq
.macro BRR\cc d, s, lbl
	br\cc\()l  \d, \s, \lbl
.endm
.endr

.macro BREQR d, s, lbl
	breql \d, \s, \lbl
.endm

.macro CMPR op1, op2
	cmpl \op1, \op2
.endm
