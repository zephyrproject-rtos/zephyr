/*
 * Copyright (c) 2021 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * Register names for o32 ABI, see [1] for details.
 *
 * [1] See MIPS Run (The Morgan Kaufmann Series in Computer
 *     Architecture and Design) 2nd Edition by Dominic Sweetman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_MIPS_INCLUDE_MIPS_REGDEF_H_
#define ZEPHYR_ARCH_MIPS_INCLUDE_MIPS_REGDEF_H_

/* always 0 */
#define zero $0

/* assembly temporary */
#define AT $1

/* subroutine return values */
#define v0 $2
#define v1 $3

/* arguments */
#define a0 $4
#define a1 $5
#define a2 $6
#define a3 $7

/* temporaries */
#define t0 $8
#define t1 $9
#define t2 $10
#define t3 $11
#define t4 $12
#define t5 $13
#define t6 $14
#define t7 $15

/* subroutine register variables */
#define s0 $16
#define s1 $17
#define s2 $18
#define s3 $19
#define s4 $20
#define s5 $21
#define s6 $22
#define s7 $23

/* temporaries */
#define t8 $24
#define t9 $25

/* interrupt/trap handler scratch registers */
#define k0 $26
#define k1 $27

/* global pointer */
#define gp $28

/* stack pointer */
#define sp $29

/* frame pointer / ninth subroutine register variable */
#define fp $30
#define s8 $30

/* return address */
#define ra $31

#endif /* ZEPHYR_ARCH_MIPS_INCLUDE_MIPS_REGDEF_H_ */
