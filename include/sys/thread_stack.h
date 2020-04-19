/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Macros for declaring thread stacks
 */

#ifndef ZEPHYR_INCLUDE_SYS_THREAD_STACK_H
#define ZEPHYR_INCLUDE_SYS_THREAD_STACK_H

#if !defined(_ASMLANGUAGE)
#include <arch/cpu.h>
#include <sys/util.h>

/* Using typedef deliberately here, this is quite intended to be an opaque
 * type.
 *
 * The purpose of this data type is to clearly distinguish between the
 * declared symbol for a stack (of type k_thread_stack_t) and the underlying
 * buffer which composes the stack data actually used by the underlying
 * thread; they cannot be used interchangeably as some arches precede the
 * stack buffer region with guard areas that trigger a MPU or MMU fault
 * if written to.
 *
 * APIs that want to work with the buffer inside should continue to use
 * char *.
 *
 * Stacks should always be created with K_THREAD_STACK_DEFINE().
 */
struct __packed z_thread_stack_element {
	char data;
};

/**
 * @typedef k_thread_stack_t
 * @brief Typedef of struct z_thread_stack_element
 *
 * @see z_thread_stack_element
 */


/**
 * @brief Properly align a CPU stack pointer value
 *
 * Take the provided value and round it down such that the value is aligned
 * to the CPU and ABI requirements. This is not used for any memory protection
 * hardware requirements.
 */
#define Z_STACK_PTR_ALIGN(ptr) ROUND_DOWN((ptr), ARCH_STACK_PTR_ALIGN)

/**
 * @brief Obtain an extern reference to a stack
 *
 * This macro properly brings the symbol of a thread stack declared
 * elsewhere into scope.
 *
 * @param sym Thread stack symbol name
 */
#define K_THREAD_STACK_EXTERN(sym) extern k_thread_stack_t sym[]

/* arch/cpu.h may declare an architecture or platform-specific macro
 * for properly declaring stacks, compatible with MMU/MPU constraints if
 * enabled
 */
#ifdef ARCH_THREAD_STACK_DEFINE
#define K_THREAD_STACK_DEFINE(sym, size) ARCH_THREAD_STACK_DEFINE(sym, size)
#define K_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
		ARCH_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size)
#define K_THREAD_STACK_LEN(size) ARCH_THREAD_STACK_LEN(size)
#define K_THREAD_STACK_MEMBER(sym, size) ARCH_THREAD_STACK_MEMBER(sym, size)
#define K_THREAD_STACK_SIZEOF(sym) ARCH_THREAD_STACK_SIZEOF(sym)
#define K_THREAD_STACK_RESERVED ((size_t)ARCH_THREAD_STACK_RESERVED)
static inline char *Z_THREAD_STACK_BUFFER(k_thread_stack_t *sym)
{
	return ARCH_THREAD_STACK_BUFFER(sym);
}
#else
/**
 * @brief Declare a toplevel thread stack memory region
 *
 * This declares a region of memory suitable for use as a thread's stack.
 *
 * This is the generic, historical definition. Align to ARCH_STACK_PTR_ALIGN
 * and put in 'noinit' section so that it isn't zeroed at boot
 *
 * The declared symbol will always be a k_thread_stack_t which can be passed to
 * k_thread_create(), but should otherwise not be manipulated. If the buffer
 * inside needs to be examined, examine thread->stack_info for the associated
 * thread object to obtain the boundaries.
 *
 * It is legal to precede this definition with the 'static' keyword.
 *
 * It is NOT legal to take the sizeof(sym) and pass that to the stackSize
 * parameter of k_thread_create(), it may not be the same as the
 * 'size' parameter. Use K_THREAD_STACK_SIZEOF() instead.
 *
 * Some arches may round the size of the usable stack region up to satisfy
 * alignment constraints. K_THREAD_STACK_SIZEOF() will return the aligned
 * size.
 *
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 */
#define K_THREAD_STACK_DEFINE(sym, size) \
	struct z_thread_stack_element __noinit \
		__aligned(ARCH_STACK_PTR_ALIGN) sym[size]

/**
 * @brief Calculate size of stacks to be allocated in a stack array
 *
 * This macro calculates the size to be allocated for the stacks
 * inside a stack array. It accepts the indicated "size" as a parameter
 * and if required, pads some extra bytes (e.g. for MPU scenarios). Refer
 * K_THREAD_STACK_ARRAY_DEFINE definition to see how this is used.
 *
 * @param size Size of the stack memory region
 */
#define K_THREAD_STACK_LEN(size) (size)

/**
 * @brief Declare a toplevel array of thread stack memory regions
 *
 * Create an array of equally sized stacks. See K_THREAD_STACK_DEFINE
 * definition for additional details and constraints.
 *
 * This is the generic, historical definition. Align to ARCH_STACK_PTR_ALIGN
 * and put in 'noinit' section so that it isn't zeroed at boot
 *
 * @param sym Thread stack symbol name
 * @param nmemb Number of stacks to declare
 * @param size Size of the stack memory region
 */
#define K_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	struct z_thread_stack_element __noinit \
		__aligned(ARCH_STACK_PTR_ALIGN) \
		sym[nmemb][K_THREAD_STACK_LEN(size)]

/**
 * @brief Declare an embedded stack memory region
 *
 * Used for stacks embedded within other data structures. Use is highly
 * discouraged but in some cases necessary. For memory protection scenarios,
 * it is very important that any RAM preceding this member not be writable
 * by threads else a stack overflow will lead to silent corruption. In other
 * words, the containing data structure should live in RAM owned by the kernel.
 *
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 */
#define K_THREAD_STACK_MEMBER(sym, size) \
	struct z_thread_stack_element __aligned(ARCH_STACK_PTR_ALIGN) sym[size]

/**
 * @brief Return the size in bytes of a stack memory region
 *
 * Convenience macro for passing the desired stack size to k_thread_create()
 * since the underlying implementation may actually create something larger
 * (for instance a guard area).
 *
 * The value returned here is not guaranteed to match the 'size' parameter
 * passed to K_THREAD_STACK_DEFINE and may be larger.
 *
 * @param sym Stack memory symbol
 * @return Size of the stack
 */
#define K_THREAD_STACK_SIZEOF(sym) sizeof(sym)


/**
 * @brief Indicate how much additional memory is reserved for stack objects
 *
 * Any given stack declaration may have additional memory in it for guard
 * areas or supervisor mode stacks. This macro indicates how much space
 * is reserved for this. The memory reserved may not be contiguous within
 * the stack object, and does not account for additional space used due to
 * enforce alignment.
 */
#define K_THREAD_STACK_RESERVED		((size_t)0U)

/**
 * @brief Get a pointer to the physical stack buffer
 *
 * This macro is deprecated. If a stack buffer needs to be examined, the
 * bounds should be obtained from the associated thread's stack_info struct.
 *
 * @param sym Declared stack symbol name
 * @return The buffer itself, a char *
 */
static inline char *Z_THREAD_STACK_BUFFER(k_thread_stack_t *sym)
{
	return (char *)sym;
}
#endif /* _ARCH_DECLARE_STACK */
#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_SYS_THREAD_STACK_H */
