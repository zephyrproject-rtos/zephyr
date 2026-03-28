/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (C) 2021 Synopsys, Inc. (www.synopsys.com)
 *
 * ALU/Memory instructions pseudo-mnemonics for ARCv2 and ARC32 ISA
 */

.macro MOVR, d, s
	mov\&$suffix d, s
.endm


.macro LDR, d, s, off
	.if $narg == 2
		ld\&$suffix  d, [s]
	.else
		ld\&$suffix  d, [s, off]
	.endif
.endm

.macro STR, d, s, off
	.if $narg == 2
		st\&$suffix  d, [s]
	.else
		st\&$suffix  d, [s, off]
	.endif
.endm


.macro PUSHR, r
	push r
.endm

.macro POPR, r
	pop r
.endm

.macro LRR, d, aux
	lr d, aux
.endm

.macro SRR, d, aux
	sr d, aux
.endm


.macro ADDR, d, s, v
	add\&$suffix d, s, v
.endm

.macro ADD2R, d, s, v
	add2\&$suffix d, s, v
.endm

.macro ADD3R, d, s, v
	add3 d, s, v
.endm

.macro SUBR, d, s, v
	sub d, s, v
.endm

.macro BMSKNR, d, s, v
	bmskn d, s, v
.endm

.macro LSRR, d, s, v
	lsr d, s, v
.endm

.macro ASLR, d, s, v
	asl d, s, v
.endm

.macro ANDR, d, s, v
	and d, s, v
.endm

.macro ORR, d, s, v
	or d, s, v
.endm

.macro BRR, d, s, lbl
	br\&$suffix d, s, lbl
.endm

.macro BREQR, d, s, lbl
	breq d, s, lbl
.endm

.macro CMPR, op1, op2
	cmp op1, op2
.endm
