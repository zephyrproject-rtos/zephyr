/* GCC specific test inline assembler functions and macros */

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

#if !defined(__GNUC__)
#error test_asm_inline_gcc.h goes only with GCC
#endif

#if defined(CONFIG_X86)
static inline void timestamp_serialize(void)
{
	__asm__ __volatile__ (/* serialize */
	"xorl %%eax,%%eax;\n\t"
	"cpuid;\n\t"
	:
	:
	: "%eax", "%ebx", "%ecx", "%edx");
}
#elif defined(CONFIG_CPU_CORTEX_M)
static inline void timestamp_serialize(void)
{
	/* isb is avaialble in all Cortex-M  */
	__asm__ __volatile__ (
	"isb;\n\t"
	:
	:
	: "memory");
}
#elif defined(CONFIG_CPU_ARCV2)
#define timestamp_serialize()
#else
#error implementation of timestamp_serialize() not provided for your CPU target
#endif

#endif /* _TEST_ASM_INLINE_GCC_H */
