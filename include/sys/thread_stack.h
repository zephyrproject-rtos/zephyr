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
 *
 * @param ptr Proposed stack pointer address
 * @return Properly aligned stack pointer address
 */
static inline char *z_stack_ptr_align(char *ptr)
{
	return (char *)ROUND_DOWN(ptr, ARCH_STACK_PTR_ALIGN);
}
#define Z_STACK_PTR_ALIGN(ptr) ((uintptr_t)z_stack_ptr_align((char *)(ptr)))

/**
 * @def K_THREAD_STACK_RESERVED
 * @brief Indicate how much additional memory is reserved for stack objects
 *
 * Any given stack declaration may have additional memory in it for guard
 * areas, supervisor mode stacks, or platform-specific data.  This macro
 * indicates how much space is reserved for this.
 *
 * This value only indicates memory that is permanently reserved in the stack
 * object. Memory that is "borrowed" from the thread's stack buffer is never
 * accounted for here.
 *
 * Reserved memory is at the beginning of the stack object. The reserved area
 * must be appropriately sized such that the stack buffer immediately following
 * it is correctly aligned.
 */
#ifdef ARCH_THREAD_STACK_RESERVED
#define K_THREAD_STACK_RESERVED		((size_t)(ARCH_THREAD_STACK_RESERVED))
#else
#define K_THREAD_STACK_RESERVED		((size_t)0U)
#endif

/**
 * @brief Properly align the lowest address of a stack object
 *
 * Return an alignment value for the lowest address of a stack object, taking
 * into consideration all alignment constraints imposed by the CPU, ABI, and
 * any memory management policies, including any alignment required by
 * reserved platform data within the stack object. This will always be at least
 * ARCH_STACK_PTR_ALIGN or an even multiple thereof.
 *
 * Depending on hardware, this is either a fixed value or a function of the
 * provided size. The requested size is significant only if
 * CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT is enabled.
 *
 * If CONFIG_USERSPACE is enabled, this determines the alignment of stacks
 * which may be used by user mode threads, or threads running in supervisor
 * mode which may later drop privileges to user mode.
 *
 * Arches define this with ARCH_THREAD_STACK_OBJ_ALIGN().
 *
 * If ARCH_THREAD_STACK_OBJ_ALIGN is not defined assume ARCH_STACK_PTR_ALIGN
 * is appropriate.
 *
 * @param size Requested size of the stack buffer (which could be ignored)
 * @return Alignment of the stack object
 */
#if defined(ARCH_THREAD_STACK_OBJ_ALIGN)
#define Z_THREAD_STACK_OBJ_ALIGN(size)	ARCH_THREAD_STACK_OBJ_ALIGN(size)
#else
#define Z_THREAD_STACK_OBJ_ALIGN(size)	ARCH_STACK_PTR_ALIGN
#endif /* ARCH_THREAD_STACK_OBJ_ALIGN */

/**
 * @def Z_THREAD_STACK_SIZE_ADJUST
 * @brief Round up a requested stack size to satisfy constraints
 *
 * Given a requested stack buffer size, return an adjusted size value for
 * the entire stack object which takes into consideration:
 *
 * - Reserved memory for platform data
 * - Alignment of stack buffer bounds to CPU/ABI constraints
 * - Alignment of stack buffer bounds to satisfy memory management hardware
 *   constraints such that a protection region can cover the stack buffer area
 *
 * If CONFIG_USERSPACE is enabled, this determines the size of stack objects
 * which  may be used by user mode threads, or threads running in supervisor
 * mode which may later drop privileges to user mode.
 *
 * Arches define this with ARCH_THREAD_STACK_SIZE_ADJUST().
 *
 * If ARCH_THREAD_STACK_SIZE_ADJUST is not defined, assume rounding up to
 * ARCH_STACK_PTR_ALIGN is appropriate.
 *
 * Any memory reserved for platform data is also included in the total
 * returned.
 *
 * @param size Requested size of the stack buffer
 * @return Adjusted size of the stack object
 */
#if defined(ARCH_THREAD_STACK_SIZE_ADJUST)
#define Z_THREAD_STACK_SIZE_ADJUST(size) \
	(ARCH_THREAD_STACK_SIZE_ADJUST(size) + K_THREAD_STACK_RESERVED)
#else
#define Z_THREAD_STACK_SIZE_ADJUST(size) \
	(ROUND_UP((size), ARCH_STACK_PTR_ALIGN) + K_THREAD_STACK_RESERVED)
#endif /* ARCH_THREAD_STACK_SIZE_ADJUST */

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
 * This is the generic, historical definition. Align to Z_THREAD_STACK_OBJ_ALIGN
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
		__aligned(Z_THREAD_STACK_OBJ_ALIGN(size)) \
		sym[size]

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
 * This is the generic, historical definition. Align to Z_THREAD_STACK_OBJ_ALIGN
 * and put in 'noinit' section so that it isn't zeroed at boot
 *
 * @param sym Thread stack symbol name
 * @param nmemb Number of stacks to declare
 * @param size Size of the stack memory region
 */
#define K_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	struct z_thread_stack_element __noinit \
		__aligned(Z_THREAD_STACK_OBJ_ALIGN(size)) \
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
	struct z_thread_stack_element \
		__aligned(Z_THREAD_STACK_OBJ_ALIGN(size)) \
		sym[size]

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
