/* msr.s - Utilities to read/write the Model Specific Registers (MSRs) */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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
This module provides the implementation of the _MsrWrite() and _MsrRead()
utilities.
*/

#define _ASMLANGUAGE

#include <nanokernel/x86/asm.h>

	/* exports (internal APIs) */

	GTEXT(_MsrWrite)
	GTEXT(_MsrRead)

/*******************************************************************************
*
* _MsrWrite - write to a model specific register (MSR)
*
* This function is used to write to an MSR.
*
* C function prototype:
*
*   void _MsrWrite (unsigned int msr, uint64_t msrData);
*
* The definitions of the so-called  "Architectural MSRs" are contained
* in nanok.h and have the format: IA32_XXX_MSR
*
* INTERNAL
* 1) The 'wrmsr' instruction was introduced in the Pentium processor; executing
*    this instruction on an earlier IA-32 processor will result in an invalid
*    opcode exception.
* 2) The 'wrmsr' uses the ECX, EDX, and EAX registers which matches the set of
*    volatile registers!  
*
* RETURNS: N/A
*/

SECTION_FUNC(TEXT, _MsrWrite)
	movl	SP_ARG1(%esp), %ecx	/* load ECX with <msr> */
	movl	0x8(%esp), %eax  	/* load LS 32-bits of <msrData> */
	movl	0xc(%esp), %edx 	/* load MS 32-bits of <msrData> */
	wrmsr   			/* write %edx:%eax to the MSR */
	ret


/*******************************************************************************
*
* _MsrRead - read from a model specific register (MSR)
*
* This function is used to read from an MSR.
*
* C function prototype:
*
*   uint64_t _MsrRead (unsigned int msr);
*
* The definitions of the so-called  "Architectural MSRs" are contained
* in nanok.h and have the format: IA32_XXX_MSR
*
* INTERNAL
* 1) The 'rdmsr' instruction was introduced in the Pentium processor; executing
*    this instruction on an earlier IA-32 processor will result in an invalid
*    opcode exception.
* 2) The 'rdmsr' uses the ECX, EDX, and EAX registers which matches the set of
*    volatile registers!  
*
* RETURNS: N/A
*/

SECTION_FUNC(TEXT, _MsrRead)
	movl	SP_ARG1(%esp), %ecx	/* load ECX with <msr> */
	rdmsr   			/* read MSR into %edx:%eax */
	ret
