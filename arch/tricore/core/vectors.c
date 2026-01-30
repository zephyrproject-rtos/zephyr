/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

/*
 * AURIX TriCore trap and IRQ vector tables.
 *
 * Each trap class lives in a 32-byte BTV-indexed slot and each IRQ priority
 * in a 32-byte BIV-indexed slot. The vectors are emitted via file-scope
 * inline assembly so the compiler never owns the function body: this is
 * compiler-agnostic (GCC and Clang both emit the asm verbatim) and prevents
 * the compiler from appending an implicit `ret` that would spill out of the
 * 32-byte slot when a class grows.
 */

#define TRAP_VECTOR(n, body)                                                                       \
	__asm__(".section .vectors.trap, \"ax\"\n\t"                                               \
		".p2align 5\n\t"                                                                   \
		".global __trap_vector_tc0_class" #n "\n"                                          \
		".type __trap_vector_tc0_class" #n ", @function\n"                                 \
		"__trap_vector_tc0_class" #n ":\n\t"                                               \
		body "\n\t"                                                                        \
		".size __trap_vector_tc0_class" #n ", . - __trap_vector_tc0_class" #n "\n")

TRAP_VECTOR(0, "svlcx\n\t"
	       "mov %d4, 0\n\t"
	       "mov %d5, %d15\n\t"
	       "j z_tricore_fault");

TRAP_VECTOR(1, "svlcx\n\t"
	       "mov %d4, 1\n\t"
	       "mov %d5, %d15\n\t"
	       "j z_tricore_fault");

TRAP_VECTOR(2, "svlcx\n\t"
	       "mov %d4, 2\n\t"
	       "mov %d5, %d15\n\t"
	       "j z_tricore_fault");

TRAP_VECTOR(3, "jeq %d15, 4, 1f\n\t"
	       "svlcx\n\t"
	       "mov %d4, 3\n\t"
	       "mov %d5, %d15\n\t"
	       "j z_tricore_fault\n"
	       "1:\n\t"
	       "j z_tricore_fault_fcu");

TRAP_VECTOR(4, "svlcx\n\t"
	       "mov %d4, 4\n\t"
	       "mov %d5, %d15\n\t"
	       "j z_tricore_fault");

TRAP_VECTOR(5, "svlcx\n\t"
	       "mov %d4, 5\n\t"
	       "mov %d5, %d15\n\t"
	       "j z_tricore_fault");

TRAP_VECTOR(6, "svlcx\n\t"
	       "j _syscall_wrapper");

TRAP_VECTOR(7, "svlcx\n\t"
	       "mov %d4, 7\n\t"
	       "mov %d5, %d15\n\t"
	       "j z_tricore_fault");

#define IRQ_VECTOR(prio, _)                                                                        \
	__asm__(".section .vectors.irq, \"ax\"\n\t"                                                \
		".p2align 5\n\t"                                                                   \
		".weak __irq_vector_tc0_" #prio "\n\t"                                             \
		".type __irq_vector_tc0_" #prio ", @function\n"                                    \
		"__irq_vector_tc0_" #prio ":\n\t"                                                  \
		"svlcx\n\t"                                                                        \
		"j _isr_wrapper\n\t"                                                               \
		".size __irq_vector_tc0_" #prio ", . - __irq_vector_tc0_" #prio "\n");

LISTIFY(256, IRQ_VECTOR, ())
