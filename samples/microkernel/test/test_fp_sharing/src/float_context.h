/* float_context.h - common definitions for the FPU sharing test application */

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

#ifndef _FLOATCONTEXT_H
#define _FLOATCONTEXT_H

/*
 * Each architecture must define the FP_REG_SET and FP_NONVOLATILE_REG_SET
 * structures, and tailor the architecture specific implementations of
 * _LoadAllFloatRegisters(), _StoreAllFloatRegisters(), and
 * _StoreNonVolatileFloatRegisters() to read/write from these structures.
 */

#if defined(CONFIG_ISA_IA32)

/* No need to specify USE_SSE since AUTOMATIC_FP_ENABLING is in effect. */

#define FP_OPTION 0

/*
 * In the future, the struct definitions may need to be refined based on the
 * specific IA-32 processor, but for now only the Pentium4 is supported.
 *
 * There exists 8 x 80 bit floating point registers (ST[0] -> ST[7]) and
 * 8 * 128 bit XMM registers (XMM[0] -> XMM[7]). All of these registers are
 * considered volatile across a function invocation.
 */

/* a single x87 FPU register in double extended format (80 bits) */

typedef struct fpReg {
	unsigned char reg[10];
} FP_REG;

/* a single XMM register (128 bits) */

typedef struct xmmReg {
	unsigned char reg[16];
} XMM_REG;

/* the set of volatile floating point registers */

typedef struct fpVolatileRegSet {
	XMM_REG xmmRegs[8]; /* XMM[0] -> XMM[7] */
	FP_REG fpRegs[8];   /* ST[0] -> ST[7] */
} FP_VOLATILE_REG_SET;

/* the set of non-volatile floating point registers */

typedef struct fpNonVolatileRegSet {
	/* this structure has been intentionally left blank */
} FP_NONVOLATILE_REG_SET;

#define SIZEOF_FP_VOLATILE_REG_SET sizeof(FP_VOLATILE_REG_SET)
#define SIZEOF_FP_NONVOLATILE_REG_SET 0


#else /* ! CONFIG_ISA_IA32 */

#error Architecture needs to provide a definition for 'struct fpRegSet' and 'struct fpNonVolatileRegSet'
#endif /* CONFIG_ISA_IA32 */

/* the set of ALL floating point registers */

typedef struct fpRegSet {
	FP_VOLATILE_REG_SET fpVolRegSet;
	FP_NONVOLATILE_REG_SET fpNonVolRegSet;
} FP_REG_SET;

#define SIZEOF_FP_REG_SET \
	(SIZEOF_FP_VOLATILE_REG_SET + SIZEOF_FP_NONVOLATILE_REG_SET)

/*
 * The following constants define the initial byte value used by the background
 * task, and the fiber when loading up the floating point registers.
 */

#define MAIN_FLOAT_REG_CHECK_BYTE (unsigned char)0xe5
#define FIBER_FLOAT_REG_CHECK_BYTE (unsigned char)0xf9

void _StoreNonVolatileFloatRegisters(FP_NONVOLATILE_REG_SET *pToBuffer);

extern int fpu_sharing_error;

#endif /* _FLOATRCONTEXT_H */
