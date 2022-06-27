/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Populated exception vector table
 *
 * Vector table with exceptions filled in. The reset vector is the system entry
 * point, ie. the first instruction executed.
 *
 * The table is populated with all the system exception handlers. No exception
 * should not be triggered until the kernel is ready to handle them.
 *
 * We are using a C file instead of an assembly file (like the ARM vector table)
 * to work around an issue with the assembler where:
 *
 *   .word <function>
 *
 * statements would end up with the two half-words of the functions' addresses
 * swapped.
 */

#include <zephyr/types.h>
#include <zephyr/toolchain.h>
#include "vector_table.h"

struct vector_table {
	uintptr_t reset;
	uintptr_t memory_error;
	uintptr_t instruction_error;
	uintptr_t ev_machine_check;
	uintptr_t ev_tlb_miss_i;
	uintptr_t ev_tlb_miss_d;
	uintptr_t ev_prot_v;
	uintptr_t ev_privilege_v;
	uintptr_t ev_swi;
	uintptr_t ev_trap;
	uintptr_t ev_extension;
	uintptr_t ev_div_zero;
	/* ev_dc_error is unused in ARCv3 and de-facto unused in ARCv2 as well */
	uintptr_t ev_dc_error;
	uintptr_t ev_maligned;
	uintptr_t unused_1;
	uintptr_t unused_2;
};

struct vector_table _VectorTable Z_GENERIC_SECTION(.exc_vector_table) = {
	(uintptr_t)__reset,
	(uintptr_t)__memory_error,
	(uintptr_t)__instruction_error,
	(uintptr_t)__ev_machine_check,
	(uintptr_t)__ev_tlb_miss_i,
	(uintptr_t)__ev_tlb_miss_d,
	(uintptr_t)__ev_prot_v,
	(uintptr_t)__ev_privilege_v,
	(uintptr_t)__ev_swi,
	(uintptr_t)__ev_trap,
	(uintptr_t)__ev_extension,
	(uintptr_t)__ev_div_zero,
	(uintptr_t)__ev_dc_error,
	(uintptr_t)__ev_maligned,
	0,
	0
};
