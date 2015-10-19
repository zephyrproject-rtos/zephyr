/* asmPrv.h - Private x86 assembler header */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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

#ifndef __INCsysX86AsmPrvh
#define __INCsysX86AsmPrvh

#include <arch/x86/asm.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Macro to generate name of stub from name of handler */
#define MK_STUB_NAME(h) h##Stub

/**
 * @brief Assembler version of the NANO_CPU_INT_REGISTER macro.
 *
 * See arch.h for details
 *
 * @param handler Routine to be connected
 * @param irq IRQ number
 * @param priority IRQ priority
 * @param vector Interrupt Vector
 * @param dpl Descriptor Privilege Level
 */
#define NANO_CPU_INT_REGISTER_ASM(handler, irq, priority, vector, dpl)     \
	.section ".intList";                                \
	MK_ISR_NAME(handler) : .long MK_STUB_NAME(handler); \
	.long irq;                                          \
	.long priority;                                     \
	.long vector;                                       \
	.long dpl;

#define NANO_CPU_EXC_CONNECT_CODE(h) \
	call _ExcEnt;                \
	call h;                      \
	jmp _ExcExit;

#define NANO_CPU_EXC_CONNECT_NO_ERR_CODE(h) \
	call _ExcEntNoErr;                  \
	call h;                             \
	jmp _ExcExit;

/**
 *
 * @brief To generate and register an exception stub
 *
 * Generates an exception stub for the handler, @a h. It is registered
 * on the vector given by @a v with the privilege level @a d; @a d should always
 * be 0.
 *
 * Use this version of the macro if the processor pushes an error code for the
 * given exception.
 */

#define NANO_CPU_EXC_CONNECT(h, v, d)                              \
	NANO_CPU_INT_REGISTER_ASM(h, -1, -1, v, d) GTEXT(MK_STUB_NAME(h)); \
	SECTION_FUNC(TEXT, MK_STUB_NAME(h)) NANO_CPU_EXC_CONNECT_CODE(h)

/**
 *
 * @brief To generate and register an exception stub
 *
 * Generates an exception stub for the handler, @a h. It is registered
 * on the vector given by @a v with the privilege level @a d; @a d should always
 * be 0.
 *
 * Use this version of the macro if the processor doesn't push an error code for
 * the given exception. The created stub pushes a dummy value of 0 to keep the
 * exception stack frame the same.
 */
#define NANO_CPU_EXC_CONNECT_NO_ERR(h, v, d)                       \
	NANO_CPU_INT_REGISTER_ASM(h, -1, -1, v, d) GTEXT(MK_STUB_NAME(h)); \
	SECTION_FUNC(TEXT, MK_STUB_NAME(h))                      \
		NANO_CPU_EXC_CONNECT_NO_ERR_CODE(h)

#ifdef __cplusplus
}
#endif

#endif /* __INCsysX86AsmPrvh */
