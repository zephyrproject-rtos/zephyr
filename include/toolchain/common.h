/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common toolchain abstraction
 *
 * Macros to abstract compiler capabilities (common to all toolchains).
 */

/* Abstract use of extern keyword for compatibility between C and C++ */
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

/* Use TASK_ENTRY_CPP to tag task entry points defined in C++ files. */

#ifdef __cplusplus
#define TASK_ENTRY_CPP  extern "C"
#endif

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
 * If the project is being built for speed (i.e. not for minimum size) then
 * align functions and branches in executable sections to improve performance.
 */

#ifdef _ASMLANGUAGE

  #ifdef CONFIG_X86

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

  #elif defined(CONFIG_NIOS2) || defined(CONFIG_RISCV32) || defined(CONFIG_XTENSA)

    #define PERFOPT_ALIGN .balign 4

  #else

    #error Architecture unsupported

  #endif

  #define GC_SECTION(sym) SECTION .text.##sym, "ax"

#endif /* _ASMLANGUAGE */

/* force inlining a function */

#if !defined(_ASMLANGUAGE)
  #define ALWAYS_INLINE inline __attribute__((always_inline))
#endif

#define _STRINGIFY(x) #x
#define STRINGIFY(s) _STRINGIFY(s)

/* Indicate that an array will be used for stack space. */

#define __stack __aligned(STACK_ALIGN)

/* concatenate the values of the arguments into one */
#define _DO_CONCAT(x, y) x ## y
#define _CONCAT(x, y) _DO_CONCAT(x, y)

/* compile-time assertion that makes the build fail */
#define BUILD_ASSERT(EXPR) typedef char __build_assert_failure[(EXPR) ? 1 : -1]

