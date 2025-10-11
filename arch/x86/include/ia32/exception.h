/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_IA32_EXCEPTION_H_
#define ZEPHYR_ARCH_X86_INCLUDE_IA32_EXCEPTION_H_

#ifndef _ASMLANGUAGE

#include <zephyr/toolchain/common.h>

#define _EXCEPTION_INTLIST(vector, dpl) \
	".pushsection .gnu.linkonce.intList.exc_" #vector "\n\t" \
	".long 1f\n\t"				/* ISR_LIST.fnc */ \
	".long -1\n\t"				/* ISR_LIST.irq */ \
	".long -1\n\t"				/* ISR_LIST.priority */ \
	".long " STRINGIFY(vector) "\n\t"	/* ISR_LIST.vec */ \
	".long " STRINGIFY(dpl) "\n\t"		/* ISR_LIST.dpl */ \
	".long 0\n\t"				/* ISR_LIST.tss */ \
	".popsection\n\t" \

/* Extra preprocessor indirection to ensure arguments get expanded before
 * concatenation takes place
 */
#define __EXCEPTION_STUB_NAME(handler, vec) \
	_ ## handler ## _vector_ ## vec ## _stub

#define _EXCEPTION_STUB_NAME(handler, vec) \
	__EXCEPTION_STUB_NAME(handler, vec) \

/* Unfortunately, GCC extended asm doesn't work at toplevel so we need
 * to stringify stuff.
 *
 * What we are doing here is generating entries in the .intList section
 * and also the assembly language stubs for the exception. We use
 * .gnu.linkonce section prefix so that the linker only includes the
 * first one of these it encounters for a particular vector. In this
 * way it's easy for applications or drivers to install custom exception
 * handlers without having to #ifdef out previous instances such as in
 * arch/x86/core/fatal.c
 */
#define __EXCEPTION_CONNECT(handler, vector, dpl, codepush) \
	__asm__ ( \
	 _EXCEPTION_INTLIST(vector, dpl)		      \
	".pushsection .gnu.linkonce.t.exc_" STRINGIFY(vector) \
		  "_stub, \"ax\"\n\t" \
	".global " STRINGIFY(_EXCEPTION_STUB_NAME(handler, vector)) "\n\t" \
	STRINGIFY(_EXCEPTION_STUB_NAME(handler, vector)) ":\n\t" \
	"1:\n\t" \
	"endbr32\n\t" \
	codepush \
	"push $" STRINGIFY(handler) "\n\t" \
	"jmp _exception_enter\n\t" \
	".popsection\n\t" \
	)


/**
 * @brief Connect an exception handler that doesn't expect error code
 *
 * Assign an exception handler to a particular vector in the IDT.
 *
 * @param handler A handler function of the prototype
 *                void handler(const struct arch_esf *esf)
 * @param vector Vector index in the IDT
 */
#define _EXCEPTION_CONNECT_NOCODE(handler, vector, dpl) \
	__EXCEPTION_CONNECT(handler, vector, dpl, "push $0\n\t")

/**
 * @brief Connect an exception handler that does expect error code
 *
 * Assign an exception handler to a particular vector in the IDT.
 * The error code will be accessible in esf->errorCode
 *
 * @param handler A handler function of the prototype
 *                void handler(const struct arch_esf *esf)
 * @param vector Vector index in the IDT
 */
#define _EXCEPTION_CONNECT_CODE(handler, vector, dpl) \
	__EXCEPTION_CONNECT(handler, vector, dpl, "")

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_IA32_EXCEPTION_H_ */
