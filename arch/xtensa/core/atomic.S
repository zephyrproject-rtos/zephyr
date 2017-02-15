/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa_context.h>
/**
 *
 * @brief Atomically clear a memory location
 *
 * This routine atomically clears the contents of <target> and returns the old
 * value that was in <target>.
 *
 * This routine can be used from both task and interrupt level.
 *
 * @return Contents of <target> before the atomic operation
 *
 * atomic_val_t atomic_clear
 *    (
 *    atomic_t *target	/@ memory location to clear @/
 *    )
 */
	.global atomic_clear
	.type   atomic_clear,@function
	.align  4
atomic_clear:
	ENTRY(48)
	movi a4, 0
.L_LoopClear:
	l32ai a3, a2, 0
	wsr a3, scompare1
	s32c1i a4, a2, 0
	bne a3, a4, .L_LoopClear
	mov a2, a3
	RET(48)

/**
 *
 * @brief Atomically set a memory location
 *
 * This routine atomically sets the contents of <target> to <value> and returns
 * the old value that was in <target>.
 *
 * This routine can be used from both task and interrupt level.
 *
 * @return Contents of <target> before the atomic operation
 *
 * atomic_val_t atomic_set
 *    (
 *    atomic_t *target,	/@ memory location to set @/
 *    atomic_val_t value	/@ set with this value @/
 *    )
 *
 */
	.global atomic_set
	.type   atomic_set,@function
	.align  4
atomic_set:
	ENTRY(48)
.L_LoopSet:
	l32ai a4, a2, 0
	wsr a4, scompare1
	s32c1i a3, a2, 0
	bne a3, a4, .L_LoopSet
	mov a2, a3
	RET(48)

/**
 *
 * @brief Get the value of a shared memory atomically
 *
 * This routine atomically retrieves the value in *target
 *
 * long atomic_get
 *     (
 *     atomic_t * target    /@ address of atom to be retrieved @/
 *     )
 *
 * @return value read from address target.
 *
 */
	.global atomic_get
	.type   atomic_get,@function
	.align  4
atomic_get:
	ENTRY(48)
	l32ai a2, a2, 0
	RET(48)

/**
 *
 * @brief Atomically increment a memory location
 *
 * This routine atomically increments the value in <target>.  The operation is
 * done using unsigned integer arithmetic.  Various CPU architectures may
 * impose restrictions with regards to the alignment and cache attributes of
 * the atomic_t type.
 *
 * This routine can be used from both task and interrupt level.
 *
 * @return Contents of <target> before the atomic operation
 *
 * atomic_val_t atomic_inc
 *    (
 *    atomic_t *target,	/@ memory location to increment @/
 *    )
 *
 */

	.global atomic_inc
	.type   atomic_inc,@function
	.align  4
atomic_inc:
	ENTRY(48)
.L_LoopInc:
	l32ai a3, a2, 0
	wsr a3, scompare1
	addi a4, a3, 1
	s32c1i a4, a2, 0
	bne a3, a4, .L_LoopInc
	mov a2, a3
	RET(48)

/**
 *
 * @brief Atomically add a value to a memory location
 *
 * This routine atomically adds the contents of <target> and <value>, placing
 * the result in <target>. The operation is done using signed integer
 * arithmetic.  Various CPU architectures may impose restrictions with regards
 * to the alignment and cache attributes of the atomic_t type.
 *
 * This routine can be used from both task and interrupt level.
 *
 * @return Contents of <target> before the atomic operation
 *
 * atomic_val_t atomic_add
 *    (
 *    atomic_t *target,	/@ memory location to add to @/
 *    atomic_val_t value	/@ value to add @/
 *    )
 */
	.global atomic_add
	.type   atomic_add,@function
	.align  4
atomic_add:
	ENTRY(48)
.L_LoopAdd:
	l32ai a4, a2, 0
	wsr a4, scompare1
	add a5, a3, a4
	s32c1i a5, a2, 0
	bne a5, a4, .L_LoopAdd
	mov a2, a5
	RET(48)

/**
 *
 * @brief Atomically decrement a memory location
 *
 * This routine atomically decrements the value in <target>.  The operation is
 * done using unsigned integer arithmetic.  Various CPU architectures may impose
 * restrictions with regards to the alignment and cache attributes of the
 * atomic_t type.
 *
 * This routine can be used from both task and interrupt level.
 *
 * @return Contents of <target> before the atomic operation
 *
 * atomic_val_t atomic_dec
 *    (
 *    atomic_t *target,	/@ memory location to decrement @/
 *    )
 *
 */

	.global atomic_dec
	.type   atomic_dec,@function
	.align  4
atomic_dec:
	ENTRY(48)
.L_LoopDec:
	l32ai a3, a2, 0
	wsr a3, scompare1
	addi a4, a3, -1
	s32c1i a4, a2, 0
	bne a3, a4, .L_LoopDec
	mov a2, a3
	RET(48)

/**
 *
 * @brief Atomically subtract a value from a memory location
 *
 * This routine atomically subtracts <value> from the contents of <target>,
 * placing the result in <target>.  The operation is done using signed integer
 * arithmetic. Various CPU architectures may impose restrictions with regards to
 * the alignment and cache attributes of the atomic_t type.
 *
 * This routine can be used from both task and interrupt level.
 *
 * @return Contents of <target> before the atomic operation
 *
 * atomic_val_t atomic_sub
 *    (
 *    atomic_t *target,	/@ memory location to subtract from @/
 *    atomic_val_t value	/@ value to subtract @/
 *    )
 *
 */

	.global atomic_sub
	.type   atomic_sub,@function
	.align  4
atomic_sub:
	ENTRY(48)
.L_LoopSub:
	l32ai a4, a2, 0
	wsr a4, scompare1
	sub a5, a4, a3
	s32c1i a5, a2, 0
	bne a5, a4, .L_LoopSub
	mov a2, a5
	RET(48)

/**
 *
 * @brief Atomically perform a bitwise NAND on a memory location
 *
 * This routine atomically performs a bitwise NAND operation of the contents of
 * <target> and <value>, placing the result in <target>.
 * Various CPU architectures may impose restrictions with regards to the
 * alignment and cache attributes of the atomic_t type.
 *
 * This routine can be used from both task and interrupt level.
 *
 * @return Contents of <target> before the atomic operation
 *
 * atomic_val_t atomic_nand
 *    (
 *    atomic_t *target,	/@ memory location to NAND @/
 *    atomic_val_t value	/@ NAND with this value @/
 *    )
 *
 */

	.global atomic_nand
	.type   atomic_nand,@function
	.align  4
atomic_nand:
	ENTRY(48)
.L_LoopNand:
	l32ai a4, a2, 0
	wsr a4, scompare1
	and a5, a3, a4
	neg a5, a5
	addi a5, a5, -1
	s32c1i a5, a2, 0
	bne a5, a4, .L_LoopNand
	mov a2, a4
	RET(48)

/**
 *
 * @brief Atomically perform a bitwise AND on a memory location
 *
 * This routine atomically performs a bitwise AND operation of the contents of
 * <target> and <value>, placing the result in <target>.
 * Various CPU architectures may impose restrictions with regards to the
 * alignment and cache attributes of the atomic_t type.
 *
 * This routine can be used from both task and interrupt level.
 *
 * @return Contents of <target> before the atomic operation
 *
 * atomic_val_t atomic_and
 *    (
 *    atomic_t *target,	/@ memory location to AND @/
 *    atomic_val_t value	/@ AND with this value @/
 *    )
 *
 */

	.global atomic_and
	.type   atomic_and,@function
	.align  4
atomic_and:
	ENTRY(48)
.L_LoopAnd:
	l32ai a4, a2, 0
	wsr a4, scompare1
	and a5, a3, a4
	s32c1i a5, a2, 0
	bne a5, a4, .L_LoopAnd
	mov a2, a4
	RET(48)

/**
 *
 * @brief Atomically perform a bitwise OR on memory location
 *
 * This routine atomically performs a bitwise OR operation of the contents of
 * <target> and <value>, placing the result in <target>.
 * Various CPU architectures may impose restrictions with regards to the
 * alignment and cache attributes of the atomic_t type.
 *
 * This routine can be used from both task and interrupt level.
 *
 * @return Contents of <target> before the atomic operation
 *
 * atomic_val_t atomic_or
 *    (
 *    atomic_t *target,	/@ memory location to OR @/
 *    atomic_val_t value	/@ OR with this value @/
 *    )
 *
 */

	.global atomic_or
	.type   atomic_or,@function
	.align  4
atomic_or:
	ENTRY(48)
.L_LoopOr:
	l32ai a4, a2, 0
	wsr a4, scompare1
	or a5, a3, a4
	s32c1i a5, a2, 0
	bne a4, a5, .L_LoopOr
	mov a2, a4
	RET(48)

/**
 *
 * @brief Atomically perform a bitwise XOR on a memory location
 *
 * This routine atomically performs a bitwise XOR operation of the contents of
 * <target> and <value>, placing the result in <target>.
 * Various CPU architectures may impose restrictions with regards to the
 * alignment and cache attributes of the atomic_t type.
 *
 * This routine can be used from both task and interrupt level.
 *
 * @return Contents of <target> before the atomic operation
 *
 * atomic_val_t atomic_xor
 *    (
 *    atomic_t *target,	/@ memory location to XOR @/
 *    atomic_val_t value	/@ XOR with this value @/
 *    )
 *
 */

	.global atomic_xor
	.type   atomic_xor,@function
	.align  4
atomic_xor:
	ENTRY(48)
.L_LoopXor:
	l32ai a4, a2, 0
	wsr a4, scompare1
	xor a5, a3, a4
	s32c1i a5, a2, 0
	bne a5, a4, .L_LoopXor
	mov a2, a4
	RET(48)

/**
 *
 * @brief Atomically compare-and-swap the contents of a memory location
 *
 * This routine performs an atomic compare-and-swap. testing that the contents
 * of <target> contains <oldValue>, and if it does, setting the value of
 * <target> to <newValue>. Various CPU architectures may impose restrictions
 * with regards to the alignment and cache attributes of the atomic_t type.
 *
 * This routine can be used from both task and interrupt level.
 *
 * @return 1 if the swap is actually executed, 0 otherwise.
 *
 * int atomic_cas
 *    (
 *    atomic_t *target,	        /@ memory location to compare-and-swap @/
 *    atomic_val_t oldValue,	/@ compare to this value @/
 *    atomic_val_t newValue,	/@ swap with this value @/
 *    )
 *
 */
	.global atomic_cas
	.type   atomic_cas,@function
	.align  4
atomic_cas:
	ENTRY(48)
.L_LoopCas:
	l32ai a5, a2, 0
	beq a5, a3, 1f
	movi a2, 0
	j 2f
1:
	wsr a5, scompare1
	s32c1i a4, a2, 0
	bne a3, a5, .L_LoopCas
	movi a2, 1
2:
	RET(48)
