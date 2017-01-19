/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
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
