/* Inline assembler kernel functions and macros */

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

#ifndef _ASM_INLINE_H
#define _ASM_INLINE_H

#if !defined(CONFIG_X86)
#error The arch/x86/include/asm_inline.h is only for x86 architecture
#endif

#if defined(__GNUC__)
#include <asm_inline_gcc.h>
#else
#include <asm_inline_other.h>
#endif /* __GNUC__ */

#endif /* _ASM_INLINE_H */
