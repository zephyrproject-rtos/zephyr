/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file Utilities to read/write the Model Specific Registers (MSRs)
 */

#include <zephyr.h>

/**
 *
 * @brief Write to a model specific register (MSR)
 *
 * This function is used to write to an MSR.
 *
 * The definitions of the so-called  "Architectural MSRs" are contained
 * in kernel_structs.h and have the format: IA32_XXX_MSR
 *
 * INTERNAL
 * 1) The 'wrmsr' instruction was introduced in the Pentium processor; executing
 *    this instruction on an earlier IA-32 processor will result in an invalid
 *    opcode exception.
 * 2) The 'wrmsr' uses the ECX, EDX, and EAX registers which matches the set of
 *    volatile registers!
 *
 * @return N/A
 */
void _MsrWrite(unsigned int msr, uint64_t msr_data)
{
	__asm__ volatile (
	    "movl %[msr], %%ecx\n\t"
	    "movl %[data_lo], %%eax\n\t"
	    "movl %[data_hi], %%edx\n\t"
	    "wrmsr"
	    :
	    : [msr] "m" (msr),
	      [data_lo] "rm" ((uint32_t)(msr_data & 0xFFFFFFFF)),
	      [data_hi] "rm" ((uint32_t)(msr_data >> 32))
	    : "eax", "ecx", "edx");
}


/**
 *
 * @brief Read from a model specific register (MSR)
 *
 * This function is used to read from an MSR.
 *
 * The definitions of the so-called  "Architectural MSRs" are contained
 * in kernel_structs.h and have the format: IA32_XXX_MSR
 *
 * INTERNAL
 * 1) The 'rdmsr' instruction was introduced in the Pentium processor; executing
 *    this instruction on an earlier IA-32 processor will result in an invalid
 *    opcode exception.
 * 2) The 'rdmsr' uses the ECX, EDX, and EAX registers which matches the set of
 *    volatile registers!
 *
 * @return N/A
 */
uint64_t _MsrRead(unsigned int msr)
{
	uint64_t ret;

	__asm__ volatile (
	    "movl %[msr], %%ecx\n\t"
	    "rdmsr"
	    : "=A" (ret)
	    : [msr] "rm" (msr)
	    : "ecx");

	return ret;
}
