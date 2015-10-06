/* msr.S - Utilities to read/write the Model Specific Registers (MSRs) */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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

/*
DESCRIPTION
This module provides the implementation of the _MsrWrite() and _MsrRead()
utilities.
 */

#define _ASMLANGUAGE

#include <arch/x86/asm.h>

	/* exports (internal APIs) */

	GTEXT(_MsrWrite)
	GTEXT(_MsrRead)

/**
 *
 * @brief Write to a model specific register (MSR)
 *
 * This function is used to write to an MSR.
 *
 * C function prototype:
 *
 *   void _MsrWrite (unsigned int msr, uint64_t msrData);
 *
 * The definitions of the so-called  "Architectural MSRs" are contained
 * in nano_private.h and have the format: IA32_XXX_MSR
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

SECTION_FUNC(TEXT, _MsrWrite)
	movl	SP_ARG1(%esp), %ecx	/* load ECX with <msr> */
	movl	0x8(%esp), %eax  	/* load LS 32-bits of <msrData> */
	movl	0xc(%esp), %edx 	/* load MS 32-bits of <msrData> */
	wrmsr   			/* write %edx:%eax to the MSR */
	ret


/**
 *
 * @brief Read from a model specific register (MSR)
 *
 * This function is used to read from an MSR.
 *
 * C function prototype:
 *
 *   uint64_t _MsrRead (unsigned int msr);
 *
 * The definitions of the so-called  "Architectural MSRs" are contained
 * in nano_private.h and have the format: IA32_XXX_MSR
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

SECTION_FUNC(TEXT, _MsrRead)
	movl	SP_ARG1(%esp), %ecx	/* load ECX with <msr> */
	rdmsr   			/* read MSR into %edx:%eax */
	ret
