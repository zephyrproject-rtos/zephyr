/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_COMMON_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_COMMON_H_
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

  #if defined(CONFIG_X86)

    #ifdef PERF_OPT
      #define PERFOPT_ALIGN .balign 16
    #else
      #define PERFOPT_ALIGN .balign  1
    #endif

  #elif defined(CONFIG_ARM)

    #define PERFOPT_ALIGN .balign  4

  #elif defined(CONFIG_ARC)

    /* .align assembler directive is supposed by all ARC toolchains and it is
     * implemented in a same way across ARC toolchains.
     */
    #define PERFOPT_ALIGN .align  4

  #elif defined(CONFIG_NIOS2) || defined(CONFIG_RISCV) || \
	  defined(CONFIG_XTENSA)
    #define PERFOPT_ALIGN .balign 4

  #elif defined(CONFIG_ARCH_POSIX)

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
     * attribute would result in their text sections ballon more than
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
#else
#define __syscall
#endif /* #ifndef ZTEST_UNITTEST */

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

#ifndef BUILD_ASSERT_MSG
#define BUILD_ASSERT_MSG(EXPR, MSG) __DEPRECATED_MACRO BUILD_ASSERT(EXPR, MSG)
#endif

/*
 * This is meant to be used in conjunction with __in_section() and similar
 * where scattered structure instances are concatened together by the linker
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

/*
 * Convenience helper combining __in_section() and Z_DECL_ALIGN().
 * The section name is the struct type prepended with an underscore.
 * The subsection is "static" and the subsubsection is the variable name.
 *
 * In the linker script, create output sections for these using
 * Z_ITERABLE_SECTION_ROM or Z_ITERABLE_SECTION_RAM.
 */
#define Z_STRUCT_SECTION_ITERABLE(struct_type, name) \
	Z_DECL_ALIGN(struct struct_type) name \
	__in_section(_##struct_type, static, name) __used

/* Special variant of Z_STRUCT_SECTION_ITERABLE, for placing alternate
 * data types within the iterable section of a specific data type. The
 * data type sizes and semantics must be equivalent!
 */
#define Z_STRUCT_SECTION_ITERABLE_ALTERNATE(out_type, struct_type, name) \
	Z_DECL_ALIGN(struct struct_type) name \
	__in_section(_##out_type, static, name) __used

/*
 * Itterator for structure instances gathered by Z_STRUCT_SECTION_ITERABLE().
 * The linker must provide a _<struct_type>_list_start symbol and a
 * _<struct_type>_list_end symbol to mark the start and the end of the
 * list of struct objects to iterate over.
 */
#define Z_STRUCT_SECTION_FOREACH(struct_type, iterator) \
	extern struct struct_type _CONCAT(_##struct_type, _list_start)[]; \
	extern struct struct_type _CONCAT(_##struct_type, _list_end)[]; \
	for (struct struct_type *iterator = \
			_CONCAT(_##struct_type, _list_start); \
	     ({ __ASSERT(iterator <= _CONCAT(_##struct_type, _list_end), \
			 "unexpected list end location"); \
		iterator < _CONCAT(_##struct_type, _list_end); }); \
	     iterator++)

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_COMMON_H_ */
