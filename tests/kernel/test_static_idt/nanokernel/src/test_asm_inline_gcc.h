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

#define _trigger_isrHandler() __asm__ volatile("int %0" : : "i" (TEST_SOFT_INT) : "memory")
#define _trigger_spurHandler() __asm__ volatile("int %0" : : "i" (TEST_SPUR_INT) : "memory")

#endif /* _TEST_ASM_INLINE_GCC_H */
