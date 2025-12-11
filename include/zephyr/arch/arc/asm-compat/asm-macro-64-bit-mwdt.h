/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (C) 2022 Synopsys, Inc. (www.synopsys.com)
 *
 * ALU/Memory instructions pseudo-mnemonics for ARC64 ISA
 */

.macro MOVR, d, s
	movl\&$suffix d, s
.endm


.macro LDR, d, s, off
	.if $narg == 2
		ldl\&$suffix  d, [s]
	.else
		ldl\&$suffix  d, [s, off]
	.endif
.endm

.macro STR, d, s, off
	.if $narg == 2
		stl\&$suffix  d, [s]
	.else
		stl\&$suffix  d, [s, off]
	.endif
.endm


.macro PUSHR, r
	pushl r
.endm

.macro POPR, r
	popl r
.endm

.macro LRR, d, aux
	lrl d, aux
.endm

.macro SRR, d, aux
	srl d, aux
.endm


.macro ADDR, d, s, v
	addl\&$suffix d, s, v
.endm

.macro ADD2R, d, s, v
	add2l\&$suffix d, s, v
.endm

.macro ADD3R, d, s, v
	add3l d, s, v
.endm

.macro SUBR, d, s, v
	subl d, s, v
.endm

.macro BMSKNR, d, s, v
	bmsknl d, s, v
.endm

.macro LSRR, d, s, v
	lsrl d, s, v
.endm

.macro ASLR, d, s, v
	asll d, s, v
.endm

.macro ANDR, d, s, v
	andl d, s, v
.endm

.macro ORR, d, s, v
	orl d, s, v
.endm

.macro BRR, d, s, lbl
	br\&$suffix\l d, s, lbl
.endm

.macro BREQR, d, s, lbl
	breql d, s, lbl
.endm

.macro CMPR, op1, op2
	cmpl op1, op2
.endm
