/* toolchain/common.h - common toolchain abstraction */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
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

/*
 * DESCRIPTION
 * Macros to abstract compiler capabilities (common to all toolchains).
 */

/*
 * Generate a reference to an external symbol.
 * The reference indicates to the linker that the symbol is required
 * by the module containing the reference and should be included
 * in the image if the module is in the image.
 *
 * The assembler directive ".set" is used to define a local symbol.
 * No memory is allocated, and the local symbol does not appear in
 * the symbol table.
 */

#ifdef _ASMLANGUAGE
  #define REQUIRES(sym) .set sym ## _Requires, sym
#else
  #define REQUIRES(sym) __asm__ (".set " # sym "_Requires, " # sym "\n\t");
#endif

#ifdef _ASMLANGUAGE
  #define SECTION .section
#endif

/*
 * The following definitions are used for symbol name compatibility.
 *
 * When #if 1, sources are assembled assuming the compiler
 * you are using does not generate global symbols prefixed by "_".
 * (e.g. elf/dwarf)
 *
 * When #if 0, sources are assembled assuming the compiler
 * you are using generates global symbols prefixed by "_".
 * (e.g. coff/stabs)
 */

#ifdef _ASMLANGUAGE
  #ifndef TOOL_PREPENDS_UNDERSCORE
    #define FUNC(sym) sym
  #else
    #define FUNC(sym) _##sym
  #endif
#endif

/*
 * If the project is being built for speed (i.e. not for minimum size) then
 * align functions and branches in executable sections to improve performance.
 */

#ifdef _ASMLANGUAGE

  #ifdef CONFIG_X86_32

    #ifdef PERF_OPT
      #define PERFOPT_ALIGN .balign 16
    #else
      #define PERFOPT_ALIGN .balign  1
    #endif

  #elif defined(CONFIG_ARM)

    #ifdef CONFIG_ISA_THUMB
      #define PERFOPT_ALIGN .balign  2
    #else
      #define PERFOPT_ALIGN .balign  4
    #endif

  #elif defined(CONFIG_ARC)

    #define PERFOPT_ALIGN .balign  4

  #endif

  #define GC_SECTION(sym) SECTION .text.FUNC(sym), "ax"

  #define BRANCH_LABEL(sym) FUNC(sym) :
  #define VAR(sym)          FUNC(sym)

#endif /* _ASMLANGUAGE */

/* force inlining a function */

#if !defined(_ASMLANGUAGE)
  #define ALWAYS_INLINE inline __attribute__((always_inline))
#endif

#define _STRINGIFY(x) #x

/* Indicate that an array will be used for stack space. */

#define __stack __aligned(STACK_ALIGN)

