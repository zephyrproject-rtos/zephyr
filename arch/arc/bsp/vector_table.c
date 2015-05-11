/* vector_table.c - populated exception vector table */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
Vector table with exceptions filled in. The reset vector is the system entry
point, ie. the first instruction executed.

The table is populated with all the system exception handlers. No exception
should not be triggered until the kernel is ready to handle them.

We are using a C file instead of an assembly file (like the ARM vector table)
to work around an issue with the assembler where:

  .word <function>

statements would end up with the two half-words of the functions' addresses
swapped.
*/

#include <stdint.h>
#include <toolchain.h>
#include "vector_table.h"

struct vector_table {
	uint32_t reset;
	uint32_t memory_error;
	uint32_t instruction_error;
	uint32_t ev_machine_check;
	uint32_t ev_tlb_miss_i;
	uint32_t ev_tlb_miss_d;
	uint32_t ev_prot_v;
	uint32_t ev_privilege_v;
	uint32_t ev_swi;
	uint32_t ev_trap;
	uint32_t ev_extension;
	uint32_t ev_div_zero;
	uint32_t ev_dc_error;
	uint32_t ev_maligned;
};

struct vector_table _VectorTable _GENERIC_SECTION(.exc_vector_table) = {
	(uint32_t)__reset,
	(uint32_t)__memory_error,
	(uint32_t)__instruction_error,
	(uint32_t)__ev_machine_check,
	(uint32_t)__ev_tlb_miss_i,
	(uint32_t)__ev_tlb_miss_d,
	(uint32_t)__ev_prot_v,
	(uint32_t)__ev_privilege_v,
	(uint32_t)__ev_swi,
	(uint32_t)__ev_trap,
	(uint32_t)__ev_extension,
	(uint32_t)__ev_div_zero,
	(uint32_t)__ev_dc_error,
	(uint32_t)__ev_maligned,
};

extern struct vector_table __start _ALIAS_OF(_VectorTable);
