/* Intel x86 GCC specific test inline assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
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

#ifndef _TEST_ASM_INLINE_GCC_H
#define _TEST_ASM_INLINE_GCC_H

#if !defined(__GNUC__) || !defined(CONFIG_X86)
#error test_asm_inline_gcc.h goes only with x86 GCC
#endif

#if !defined(CONFIG_X86_IAMCU)
static inline void isr_dummy(void)
{
	extern void _IntEnt(void);
	extern void _IntExit(void);

	/* compiler-generated preamble pushes & modifies EBP */
	__asm__ volatile (
		"pop     %%ebp;\n\t"
		"call    _IntEnt;\n\t"
		"pushl   $0;\n\t"
		"call    dummyIsr;\n\t"
		"addl    $4, %%esp;\n\t"
		"jmp     _IntExit;\n\t"
		: :
		);
}
#else
typedef void (*int_handler_t) (int context);

static inline void isr_dummy(void)
{
	extern void _execute_handler(int_handler_t function, int context);

	__asm__ volatile (
		"pushl %%eax \n\t"
		"pushl %%edx \n\t"
		"pushl %%ecx \n\t"
		"mov dummyIsr, %%eax \n\t"
		"movl $0, %%edx \n\t"
		"call _execute_handler \n\t"
		"pop %%ecx \n\t"
		"pop %%edx \n\t"
		"pop %%eax \n\t"
		"iret \n\t"
		: :
		);
}
#endif

#endif /* _TEST_ASM_INLINE_GCC_H */
