/* asmPrv.h - Private x86 assembler header */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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

#ifndef __INCsysX86AsmPrvh
#define __INCsysX86AsmPrvh

#include <arch/x86/asm.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Macro to generate name of stub from name of handler */
#define MK_STUB_NAME(h) h##Stub

/*
 * Assmembler version of the NANO_CPU_INT_REGISTER macro.
 * See arch.h for details
 */
#define NANO_CPU_INT_REGISTER_ASM(handler, vector, dpl)     \
	.section ".intList";                                \
	MK_ISR_NAME(handler) : .long MK_STUB_NAME(handler); \
	.long vector;                                       \
	.long dpl;

#define NANO_CPU_EXC_CONNECT_CODE(h) \
	call _ExcEnt;                \
	call h;                      \
	jmp _ExcExit;

#define NANO_CPU_EXC_CONNECT_NO_ERR_CODE(h) \
	pushl $0;                           \
	call _ExcEnt;                       \
	call h;                             \
	jmp _ExcExit;

/**
 *
 * NANO_CPU_EXC_CONNECT - to generate and register an exception stub
 *
 * Generates an exception stub for the handler, <h>. It is registered
 * on the vector given by <v> with the privilege level <d>; <d> should always
 * be 0.
 *
 * Use this version of the macro if the processor pushes an error code for the
 * given exception.
 */

#define NANO_CPU_EXC_CONNECT(h, v, d)                              \
	NANO_CPU_INT_REGISTER_ASM(h, v, d) GTEXT(MK_STUB_NAME(h)); \
	SECTION_FUNC(TEXT, MK_STUB_NAME(h)) NANO_CPU_EXC_CONNECT_CODE(h)

/**
 *
 * NANO_CPU_EXC_CONNECT_NO_ERR - to generate and register an exception stub
 *
 * Generates an exception stub for the handler, <h>. It is registered
 * on the vector given by <v> with the privilege level <d>; <d> should always
 * be 0.
 *
 * Use this version of the macro if the processor doesn't push an error code for
 * the given exception. The created stub pushes a dummy value of 0 to keep the
 * exception stack frame the same.
 */
#define NANO_CPU_EXC_CONNECT_NO_ERR(h, v, d)                       \
	NANO_CPU_INT_REGISTER_ASM(h, v, d) GTEXT(MK_STUB_NAME(h)); \
	SECTION_FUNC(TEXT, MK_STUB_NAME(h))                      \
		NANO_CPU_EXC_CONNECT_NO_ERR_CODE(h)

#ifdef __cplusplus
}
#endif

#endif /* __INCsysX86AsmPrvh */
