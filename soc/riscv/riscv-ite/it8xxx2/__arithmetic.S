/*
 * Copyright (c) 2022 ITE Corporation.
 * SPDX-License-Identifier: Apache-2.0
 *
 * When the 'M' extension is disabled, compiler can not recognize div/mul
 * instructions. So mul/div instructions in the below integer arithmetic
 * routines are hard coded by opcodes.
 *
 * IMPORTANT:
 * The workaround requires the nop instruction, please don't optimize it.
 */

.macro __int_arithmetic func opcode
.section .text
.align 2
.globl \func
.type \func, @function
\func:
.word \opcode
nop
ret
.set \func\()_size, .-\func
.size \func, \func\()_size
.endm

.macro fnalias orig alias
.globl \alias
.type \alias, @function
.set \alias, \orig
.size \alias, \orig\()_size
.endm

/* signed 32 bit multiplication. opcode of mul a0,a0,a1 is 0x02b50533 */
__int_arithmetic __mulsi3 0x02b50533
fnalias __mulsi3 __muldi3

/* signed 32 bit division. opcode of div a0,a0,a1 is 0x02b54533 */
__int_arithmetic __divsi3 0x02b54533
fnalias __divsi3 __divdi3

/* unsigned 32 bit division. opcode of divu a0,a0,a1 is 0x02b55533 */
__int_arithmetic __udivsi3 0x02b55533
fnalias __udivsi3 __udivdi3

/*
 * This function return the remainder of the signed division.
 * opcode of rem a0,a0,a1 is 0x02b56533
 */
__int_arithmetic __modsi3 0x02b56533
fnalias __modsi3 __moddi3

/*
 * This function return the remainder of the unsigned division.
 * opcode of remu a0,a0,a1 is 0x02b57533
 */
__int_arithmetic __umodsi3 0x02b57533
fnalias __umodsi3 __umoddi3
