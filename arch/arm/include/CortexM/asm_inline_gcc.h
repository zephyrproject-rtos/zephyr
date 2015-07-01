/* ARM CortexM GCC specific inline assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
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

#ifndef _ASM_INLINE_GCC_H
#define _ASM_INLINE_GCC_H

/*
 * The file must not be included directly
 * Include asm_inline.h instead
 */

#ifndef _ASMLANGUAGE

/**
 *
 * _IpsrGet - obtain value of IPSR register
 *
 * Obtain and return current value of IPSR register.
 *
 * RETURNS: the contents of the IPSR register
 *
 * \NOMANUAL
 */

static ALWAYS_INLINE uint32_t _IpsrGet(void)
{
	uint32_t vector;

	__asm__ volatile("mrs %0, IPSR\n\t" : "=r"(vector));
	return vector;
}

/**
 *
 * _MspSet - set the value of the Main Stack Pointer register
 *
 * Store the value of <msp> in MSP register.
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

static ALWAYS_INLINE void _MspSet(uint32_t msp /* value to store in MSP */
				  )
{
	__asm__ volatile("msr MSP, %0\n\t" :  : "r"(msp));
}

#endif /* _ASMLANGUAGE */
#endif /* _ASM_INLINE_GCC_H */
