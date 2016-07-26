/*
 * Copyright (c) 2016 Intel Corporation
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

#ifndef X86_EXCEPTION_H
#define X86_EXCEPTION_H

#ifndef _ASMLANGUAGE

/* Unfortunately, GCC extended asm doesn't work at toplevel so we need
 * to stringify stuff.
 *
 * What we are doing here is generating entires in the .intList section
 * and also the assembly language stubs for the exception. We use
 * .gnu.linkonce section prefix so that the linker only includes the
 * first one of these it encounters for a particular vector. In this
 * way it's easy for applications or drivers to install custom exception
 * handlers without having to #ifdef out previous instances such as in
 * arch/x86/core/fatal.c
 */

#define _EXCEPTION_MACRO(handler, vector, enter_func) \
	__asm__ ( \
	".pushsection .gnu.linkonce.intList.exc_" #vector "\n\t" \
	".long 1f\n\t"			/* ISR_LIST.fnc */ \
	".long -1\n\t"			/* ISR_LIST.irq */ \
	".long -1\n\t"			/* ISR_LIST.priority */ \
	".long " #vector "\n\t"		/* ISR_LIST.vec */ \
	".long 0\n\t"			/* ISR_LIST.dpl */ \
	".popsection\n\t" \
	".pushsection .gnu.linkonce.t.exc_" #vector "_stub, \"ax\"\n\t" \
	".global " #handler "Stub\n\t" \
	#handler "Stub:\n\t" \
	"1:\n\t" \
	"call " #enter_func "\n\t" \
	"call " #handler "\n\t" \
	"jmp _ExcExit\n\t" \
	".popsection\n\t" \
	)

/**
 * @brief Connect an exception handler that doesn't expect error code
 *
 * Assign an exception handler to a particular vector in the IDT.
 *
 * @param handler A handler function of the prototype
 *                void handler(const NANO_ESF *esf)
 * @param vector Vector index in the IDT
 */
#define _EXCEPTION_CONNECT_NOCODE(handler, vector) \
	_EXCEPTION_MACRO(handler, vector, _ExcEntNoErr)


/**
 * @brief Connect an exception handler that does expect error code
 *
 * Assign an exception handler to a particular vector in the IDT.
 * The error code will be accessible in esf->errorCode
 *
 * @param handler A handler function of the prototype
 *                void handler(const NANO_ESF *esf)
 * @param vector Vector index in the IDT
 */
#define _EXCEPTION_CONNECT_CODE(handler, vector) \
	_EXCEPTION_MACRO(handler, vector, _ExcEnt)

#endif /* _ASMLANGUAGE */

#endif /* X86_EXCEPTION_H */
