/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_COMMON_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_COMMON_H_

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_H_
#error Please do not include toolchain-specific headers directly, use <zephyr/toolchain.h> instead
#endif

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

#ifndef ZRESTRICT
#ifndef __cplusplus
#define ZRESTRICT restrict
#else
#define ZRESTRICT
#endif
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

  #if defined(CONFIG_X86)

    #ifdef PERF_OPT
      #define PERFOPT_ALIGN .balign 16
    #else
      #define PERFOPT_ALIGN .balign  1
    #endif

  #elif defined(CONFIG_ARM) || defined(CONFIG_ARM64)

    #define PERFOPT_ALIGN .balign  4

  #elif defined(CONFIG_ARC)

    /* .align assembler directive is supposed by all ARC toolchains and it is
     * implemented in a same way across ARC toolchains.
     */
    #define PERFOPT_ALIGN .align  4

  #elif defined(CONFIG_NIOS2) || defined(CONFIG_RISCV) || \
	  defined(CONFIG_XTENSA) || defined(CONFIG_MIPS)
    #define PERFOPT_ALIGN .balign 4

  #elif defined(CONFIG_ARCH_POSIX)

  #elif defined(CONFIG_SPARC)

    #define PERFOPT_ALIGN .align  4

  #else

    #error Architecture unsupported

  #endif

  #define GC_SECTION(sym) SECTION .text.##sym, "ax"

#endif /* _ASMLANGUAGE */

/* force inlining a function */

#if !defined(_ASMLANGUAGE)
  #ifdef CONFIG_COVERAGE
    /*
     * The always_inline attribute forces a function to be inlined,
     * even ignoring -fno-inline. So for code coverage, do not
     * force inlining of these functions to keep their bodies around
     * so their number of executions can be counted.
     *
     * Note that "inline" is kept here for kobject_hash.c and
     * priv_stacks_hash.c. These are built without compiler flags
     * used for coverage. ALWAYS_INLINE cannot be empty as compiler
     * would complain about unused functions. Attaching unused
     * attribute would result in their text sections balloon more than
     * 10 times in size, as those functions are kept in text section.
     * So just keep "inline" here.
     */
    #define ALWAYS_INLINE inline
  #else
    #define ALWAYS_INLINE inline __attribute__((always_inline))
  #endif
#endif

#define Z_STRINGIFY(x) #x
#define STRINGIFY(s) Z_STRINGIFY(s)

/* concatenate the values of the arguments into one */
#define _DO_CONCAT(x, y) x ## y
#define _CONCAT(x, y) _DO_CONCAT(x, y)

/* Additionally used as a sentinel by gen_syscalls.py to identify what
 * functions are system calls
 *
 * Note POSIX unit tests don't still generate the system call stubs, so
 * until https://github.com/zephyrproject-rtos/zephyr/issues/5006 is
 * fixed via possibly #4174, we introduce this hack -- which will
 * disallow us to test system calls in POSIX unit testing (currently
 * not used).
 */
#ifndef ZTEST_UNITTEST
#define __syscall static inline
#define __syscall_always_inline static inline __attribute__((always_inline))
#else
#define __syscall
#define __syscall_always_inline
#endif /* ZTEST_UNITTEST */

/* Definitions for struct declaration tags. These are sentinel values used by
 * parse_syscalls.py to gather a list of names of struct declarations that
 * have these tags applied for them.
 */

/* Indicates this is a driver subsystem */
#define __subsystem

/* Indicates this is a network socket object */
#define __net_socket

#ifndef BUILD_ASSERT
/* Compile-time assertion that makes the build to fail.
 * Common implementation swallows the message.
 */
#define BUILD_ASSERT(EXPR, MSG...) \
	enum _CONCAT(__build_assert_enum, __COUNTER__) { \
		_CONCAT(__build_assert, __COUNTER__) = 1 / !!(EXPR) \
	}
#endif

/*
 * This is meant to be used in conjunction with __in_section() and similar
 * where scattered structure instances are concatenated together by the linker
 * and walked by the code at run time just like a contiguous array of such
 * structures.
 *
 * Assemblers and linkers may insert alignment padding by default whose
 * size is larger than the natural alignment for those structures when
 * gathering various section segments together, messing up the array walk.
 * To prevent this, we need to provide an explicit alignment not to rely
 * on the default that might just work by luck.
 *
 * Alignment statements in  linker scripts are not sufficient as
 * the assembler may add padding by itself to each segment when switching
 * between sections within the same file even if it merges many such segments
 * into a single section in the end.
 */
#define Z_DECL_ALIGN(type) __aligned(__alignof(type)) type

/* Check if a pointer is aligned for against a specific byte boundary  */
#define IS_PTR_ALIGNED_BYTES(ptr, bytes) ((((uintptr_t)ptr) % bytes) == 0)

/* Check if a pointer is aligned enough for a particular data type. */
#define IS_PTR_ALIGNED(ptr, type) IS_PTR_ALIGNED_BYTES(ptr, __alignof(type))

/** @brief Tag a symbol (e.g. function) to be kept in the binary even though it is not used.
 *
 * It prevents symbol from being removed by the linker garbage collector. It
 * is achieved by adding a pointer to that symbol to the kept memory section.
 *
 * @param symbol Symbol to keep.
 */
#define LINKER_KEEP(symbol) \
	static const void * const symbol##_ptr  __used \
	__attribute__((__section__(".symbol_to_keep"))) = (void *)&symbol

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_COMMON_H_ */
