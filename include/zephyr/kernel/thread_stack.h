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

/**
 * @brief Thread Stack APIs
 * @ingroup kernel_apis
 * @defgroup thread_stack_api Thread Stack APIs
 * @{
 * @}
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_THREAD_STACK_H
#define ZEPHYR_INCLUDE_KERNEL_THREAD_STACK_H

#if !defined(_ASMLANGUAGE)
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 * @brief Helper macro for getting a stack frame struct
 *
 * It is very common for architectures to define a struct which contains
 * all the data members that are pre-populated in arch_new_thread().
 *
 * Given a type and an initial stack pointer, return a properly cast
 * pointer to the frame struct.
 *
 * @param type Type of the initial stack frame struct
 * @param ptr Initial aligned stack pointer value
 * @return Pointer to stack frame struct within the stack buffer
 */
#define Z_STACK_PTR_TO_FRAME(type, ptr) \
	(type *)((ptr) - sizeof(type))

#ifdef ARCH_KERNEL_STACK_RESERVED
#define K_KERNEL_STACK_RESERVED	((size_t)ARCH_KERNEL_STACK_RESERVED)
#else
#define K_KERNEL_STACK_RESERVED	((size_t)0)
#endif /* ARCH_KERNEL_STACK_RESERVED */

#define Z_KERNEL_STACK_SIZE_ADJUST(size) (ROUND_UP(size, \
						   ARCH_STACK_PTR_ALIGN) + \
					  K_KERNEL_STACK_RESERVED)

#ifdef ARCH_KERNEL_STACK_OBJ_ALIGN
#define Z_KERNEL_STACK_OBJ_ALIGN	ARCH_KERNEL_STACK_OBJ_ALIGN
#else
#define Z_KERNEL_STACK_OBJ_ALIGN	ARCH_STACK_PTR_ALIGN
#endif /* ARCH_KERNEL_STACK_OBJ_ALIGN */

#define K_KERNEL_STACK_LEN(size) \
	ROUND_UP(Z_KERNEL_STACK_SIZE_ADJUST(size), Z_KERNEL_STACK_OBJ_ALIGN)

/**
 * @addtogroup thread_stack_api
 * @{
 */

/**
 * @brief Declare a reference to a thread stack
 *
 * This macro declares the symbol of a thread stack defined elsewhere in the
 * current scope.
 *
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 */
#define K_KERNEL_STACK_DECLARE(sym, size) \
	extern struct z_thread_stack_element \
		sym[Z_KERNEL_STACK_SIZE_ADJUST(size)]

/**
 * @brief Declare a reference to a thread stack array
 *
 * This macro declares the symbol of a thread stack array defined elsewhere in
 * the current scope.
 *
 * @param sym Thread stack symbol name
 * @param nmemb Number of stacks defined
 * @param size Size of the stack memory region
 */
#define K_KERNEL_STACK_ARRAY_DECLARE(sym, nmemb, size) \
	extern struct z_thread_stack_element \
		sym[nmemb][K_KERNEL_STACK_LEN(size)]

/**
 * @brief Declare a reference to a pinned thread stack array
 *
 * This macro declares the symbol of a pinned thread stack array defined
 * elsewhere in the current scope.
 *
 * @param sym Thread stack symbol name
 * @param nmemb Number of stacks defined
 * @param size Size of the stack memory region
 */
#define K_KERNEL_PINNED_STACK_ARRAY_DECLARE(sym, nmemb, size) \
	extern struct z_thread_stack_element \
		sym[nmemb][K_KERNEL_STACK_LEN(size)]

/**
 * @brief Define a toplevel kernel stack memory region in specified section
 *
 * This defines a region of memory for use as a thread stack in
 * the specified linker section.
 *
 * It is legal to precede this definition with the 'static' keyword.
 *
 * It is NOT legal to take the sizeof(sym) and pass that to the stackSize
 * parameter of k_thread_create(), it may not be the same as the
 * 'size' parameter. Use K_KERNEL_STACK_SIZEOF() instead.
 *
 * The total amount of memory allocated may be increased to accommodate
 * fixed-size stack overflow guards.
 *
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 * @param lsect Linker section for this stack
 */
#define Z_KERNEL_STACK_DEFINE_IN(sym, size, lsect) \
	struct z_thread_stack_element lsect \
		__aligned(Z_KERNEL_STACK_OBJ_ALIGN) \
		sym[Z_KERNEL_STACK_SIZE_ADJUST(size)]

/**
 * @brief Define a toplevel array of kernel stack memory regions in specified section
 *
 * @param sym Kernel stack array symbol name
 * @param nmemb Number of stacks to define
 * @param size Size of the stack memory region
 * @param lsect Linker section for this array of stacks
 */
#define Z_KERNEL_STACK_ARRAY_DEFINE_IN(sym, nmemb, size, lsect) \
	struct z_thread_stack_element lsect \
		__aligned(Z_KERNEL_STACK_OBJ_ALIGN) \
		sym[nmemb][K_KERNEL_STACK_LEN(size)]

/**
 * @brief Define a toplevel kernel stack memory region
 *
 * This defines a region of memory for use as a thread stack, for threads
 * that exclusively run in supervisor mode. This is also suitable for
 * declaring special stacks for interrupt or exception handling.
 *
 * Stacks defined with this macro may not host user mode threads.
 *
 * It is legal to precede this definition with the 'static' keyword.
 *
 * It is NOT legal to take the sizeof(sym) and pass that to the stackSize
 * parameter of k_thread_create(), it may not be the same as the
 * 'size' parameter. Use K_KERNEL_STACK_SIZEOF() instead.
 *
 * The total amount of memory allocated may be increased to accommodate
 * fixed-size stack overflow guards.
 *
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 */
#define K_KERNEL_STACK_DEFINE(sym, size) \
	Z_KERNEL_STACK_DEFINE_IN(sym, size, __kstackmem)

/**
 * @brief Define a toplevel kernel stack memory region in pinned section
 *
 * See K_KERNEL_STACK_DEFINE() for more information and constraints.
 *
 * This puts the stack into the pinned noinit linker section if
 * CONFIG_LINKER_USE_PINNED_SECTION is enabled, or else it would
 * put the stack into the same section as K_KERNEL_STACK_DEFINE().
 *
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 */
#if defined(CONFIG_LINKER_USE_PINNED_SECTION)
#define K_KERNEL_PINNED_STACK_DEFINE(sym, size) \
	Z_KERNEL_STACK_DEFINE_IN(sym, size, __pinned_noinit)
#else
#define K_KERNEL_PINNED_STACK_DEFINE(sym, size) \
	Z_KERNEL_STACK_DEFINE_IN(sym, size, __kstackmem)
#endif /* CONFIG_LINKER_USE_PINNED_SECTION */

/**
 * @brief Define a toplevel array of kernel stack memory regions
 *
 * Stacks defined with this macro may not host user mode threads.
 *
 * @param sym Kernel stack array symbol name
 * @param nmemb Number of stacks to define
 * @param size Size of the stack memory region
 */
#define K_KERNEL_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	Z_KERNEL_STACK_ARRAY_DEFINE_IN(sym, nmemb, size, __kstackmem)

/**
 * @brief Define a toplevel array of kernel stack memory regions in pinned section
 *
 * See K_KERNEL_STACK_ARRAY_DEFINE() for more information and constraints.
 *
 * This puts the stack into the pinned noinit linker section if
 * CONFIG_LINKER_USE_PINNED_SECTION is enabled, or else it would
 * put the stack into the same section as K_KERNEL_STACK_ARRAY_DEFINE().
 *
 * @param sym Kernel stack array symbol name
 * @param nmemb Number of stacks to define
 * @param size Size of the stack memory region
 */
#if defined(CONFIG_LINKER_USE_PINNED_SECTION)
#define K_KERNEL_PINNED_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	Z_KERNEL_STACK_ARRAY_DEFINE_IN(sym, nmemb, size, __pinned_noinit)
#else
#define K_KERNEL_PINNED_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	Z_KERNEL_STACK_ARRAY_DEFINE_IN(sym, nmemb, size, __kstackmem)
#endif /* CONFIG_LINKER_USE_PINNED_SECTION */

/**
 * @brief Define an embedded stack memory region
 *
 * Used for kernel stacks embedded within other data structures.
 *
 * Stacks defined with this macro may not host user mode threads.
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 */
#define K_KERNEL_STACK_MEMBER(sym, size) \
	Z_KERNEL_STACK_DEFINE_IN(sym, size,)

#define K_KERNEL_STACK_SIZEOF(sym) (sizeof(sym) - K_KERNEL_STACK_RESERVED)

/** @} */

static inline char *K_KERNEL_STACK_BUFFER(k_thread_stack_t *sym)
{
	return (char *)sym + K_KERNEL_STACK_RESERVED;
}
#ifndef CONFIG_USERSPACE
#define K_THREAD_STACK_RESERVED		K_KERNEL_STACK_RESERVED
#define K_THREAD_STACK_SIZEOF		K_KERNEL_STACK_SIZEOF
#define K_THREAD_STACK_LEN		K_KERNEL_STACK_LEN
#define K_THREAD_STACK_DEFINE		K_KERNEL_STACK_DEFINE
#define K_THREAD_STACK_ARRAY_DEFINE	K_KERNEL_STACK_ARRAY_DEFINE
#define K_THREAD_STACK_MEMBER		K_KERNEL_STACK_MEMBER
#define K_THREAD_STACK_BUFFER		K_KERNEL_STACK_BUFFER
#define K_THREAD_STACK_DECLARE		K_KERNEL_STACK_DECLARE
#define K_THREAD_STACK_ARRAY_DECLARE	K_KERNEL_STACK_ARRAY_DECLARE
#define K_THREAD_PINNED_STACK_DEFINE	K_KERNEL_PINNED_STACK_DEFINE
#define K_THREAD_PINNED_STACK_ARRAY_DEFINE \
					K_KERNEL_PINNED_STACK_ARRAY_DEFINE
#else
/**
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
#endif /* ARCH_THREAD_STACK_RESERVED */

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
#define Z_THREAD_STACK_OBJ_ALIGN(size)	\
	ARCH_THREAD_STACK_OBJ_ALIGN(Z_THREAD_STACK_SIZE_ADJUST(size))
#else
#define Z_THREAD_STACK_OBJ_ALIGN(size)	ARCH_STACK_PTR_ALIGN
#endif /* ARCH_THREAD_STACK_OBJ_ALIGN */

/**
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
	ARCH_THREAD_STACK_SIZE_ADJUST((size) + K_THREAD_STACK_RESERVED)
#else
#define Z_THREAD_STACK_SIZE_ADJUST(size) \
	(ROUND_UP((size), ARCH_STACK_PTR_ALIGN) + K_THREAD_STACK_RESERVED)
#endif /* ARCH_THREAD_STACK_SIZE_ADJUST */

/**
 * @addtogroup thread_stack_api
 * @{
 */

/**
 * @brief Declare a reference to a thread stack
 *
 * This macro declares the symbol of a thread stack defined elsewhere in the
 * current scope.
 *
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 */
#define K_THREAD_STACK_DECLARE(sym, size) \
	extern struct z_thread_stack_element \
		sym[Z_THREAD_STACK_SIZE_ADJUST(size)]

/**
 * @brief Declare a reference to a thread stack array
 *
 * This macro declares the symbol of a thread stack array defined elsewhere in
 * the current scope.
 *
 * @param sym Thread stack symbol name
 * @param nmemb Number of stacks defined
 * @param size Size of the stack memory region
 */
#define K_THREAD_STACK_ARRAY_DECLARE(sym, nmemb, size) \
	extern struct z_thread_stack_element \
		sym[nmemb][K_THREAD_STACK_LEN(size)]

/**
 * @brief Return the size in bytes of a stack memory region
 *
 * Convenience macro for passing the desired stack size to k_thread_create()
 * since the underlying implementation may actually create something larger
 * (for instance a guard area).
 *
 * The value returned here is not guaranteed to match the 'size' parameter
 * passed to K_THREAD_STACK_DEFINE and may be larger, but is always safe to
 * pass to k_thread_create() for the associated stack object.
 *
 * @param sym Stack memory symbol
 * @return Size of the stack buffer
 */
#define K_THREAD_STACK_SIZEOF(sym)	(sizeof(sym) - K_THREAD_STACK_RESERVED)

/**
 * @brief Define a toplevel thread stack memory region in specified region
 *
 * This defines a region of memory suitable for use as a thread's stack
 * in specified region.
 *
 * This is the generic, historical definition. Align to Z_THREAD_STACK_OBJ_ALIGN
 * and put in 'noinit' section so that it isn't zeroed at boot
 *
 * The defined symbol will always be a k_thread_stack_t which can be passed to
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
 * @param lsect Linker section for this stack
 */
#define Z_THREAD_STACK_DEFINE_IN(sym, size, lsect) \
	struct z_thread_stack_element lsect \
		__aligned(Z_THREAD_STACK_OBJ_ALIGN(size)) \
		sym[Z_THREAD_STACK_SIZE_ADJUST(size)]

/**
 * @brief Define a toplevel array of thread stack memory regions in specified region
 *
 * Create an array of equally sized stacks. See Z_THREAD_STACK_DEFINE_IN
 * definition for additional details and constraints.
 *
 * This is the generic, historical definition. Align to Z_THREAD_STACK_OBJ_ALIGN
 * and put in specified section so that it isn't zeroed at boot
 *
 * @param sym Thread stack symbol name
 * @param nmemb Number of stacks to define
 * @param size Size of the stack memory region
 * @param lsect Linker section for this stack
 */
#define Z_THREAD_STACK_ARRAY_DEFINE_IN(sym, nmemb, size, lsect) \
	struct z_thread_stack_element lsect \
		__aligned(Z_THREAD_STACK_OBJ_ALIGN(size)) \
		sym[nmemb][K_THREAD_STACK_LEN(size)]

/**
 * @brief Define a toplevel thread stack memory region
 *
 * This defines a region of memory suitable for use as a thread's stack.
 *
 * This is the generic, historical definition. Align to Z_THREAD_STACK_OBJ_ALIGN
 * and put in 'noinit' section so that it isn't zeroed at boot
 *
 * The defined symbol will always be a k_thread_stack_t which can be passed to
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
	Z_THREAD_STACK_DEFINE_IN(sym, size, __stackmem)

/**
 * @brief Define a toplevel thread stack memory region in pinned section
 *
 * This defines a region of memory suitable for use as a thread's stack.
 *
 * This is the generic, historical definition. Align to Z_THREAD_STACK_OBJ_ALIGN
 * and put in 'noinit' section so that it isn't zeroed at boot
 *
 * The defined symbol will always be a k_thread_stack_t which can be passed to
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
 * This puts the stack into the pinned noinit linker section if
 * CONFIG_LINKER_USE_PINNED_SECTION is enabled, or else it would
 * put the stack into the same section as K_THREAD_STACK_DEFINE().
 *
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 */
#if defined(CONFIG_LINKER_USE_PINNED_SECTION)
#define K_THREAD_PINNED_STACK_DEFINE(sym, size) \
	Z_THREAD_STACK_DEFINE_IN(sym, size, __pinned_noinit)
#else
#define K_THREAD_PINNED_STACK_DEFINE(sym, size) \
	K_THREAD_STACK_DEFINE(sym, size)
#endif /* CONFIG_LINKER_USE_PINNED_SECTION */

/**
 * @brief Calculate size of stacks to be allocated in a stack array
 *
 * This macro calculates the size to be allocated for the stacks
 * inside a stack array. It accepts the indicated "size" as a parameter
 * and if required, pads some extra bytes (e.g. for MPU scenarios). Refer
 * K_THREAD_STACK_ARRAY_DEFINE definition to see how this is used.
 * The returned size ensures each array member will be aligned to the
 * required stack base alignment.
 *
 * @param size Size of the stack memory region
 * @return Appropriate size for an array member
 */
#define K_THREAD_STACK_LEN(size) \
	ROUND_UP(Z_THREAD_STACK_SIZE_ADJUST(size), \
		 Z_THREAD_STACK_OBJ_ALIGN(size))

/**
 * @brief Define a toplevel array of thread stack memory regions
 *
 * Create an array of equally sized stacks. See K_THREAD_STACK_DEFINE
 * definition for additional details and constraints.
 *
 * This is the generic, historical definition. Align to Z_THREAD_STACK_OBJ_ALIGN
 * and put in 'noinit' section so that it isn't zeroed at boot
 *
 * @param sym Thread stack symbol name
 * @param nmemb Number of stacks to define
 * @param size Size of the stack memory region
 */
#define K_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	Z_THREAD_STACK_ARRAY_DEFINE_IN(sym, nmemb, size, __stackmem)

/**
 * @brief Define a toplevel array of thread stack memory regions in pinned section
 *
 * Create an array of equally sized stacks. See K_THREAD_STACK_DEFINE
 * definition for additional details and constraints.
 *
 * This is the generic, historical definition. Align to Z_THREAD_STACK_OBJ_ALIGN
 * and put in 'noinit' section so that it isn't zeroed at boot
 *
 * This puts the stack into the pinned noinit linker section if
 * CONFIG_LINKER_USE_PINNED_SECTION is enabled, or else it would
 * put the stack into the same section as K_THREAD_STACK_DEFINE().
 *
 * @param sym Thread stack symbol name
 * @param nmemb Number of stacks to define
 * @param size Size of the stack memory region
 */
#if defined(CONFIG_LINKER_USE_PINNED_SECTION)
#define K_THREAD_PINNED_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	Z_THREAD_PINNED_STACK_DEFINE_IN(sym, nmemb, size, __pinned_noinit)
#else
#define K_THREAD_PINNED_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	K_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size)
#endif /* CONFIG_LINKER_USE_PINNED_SECTION */

/**
 * @brief Define an embedded stack memory region
 *
 * Used for stacks embedded within other data structures. Use is highly
 * discouraged but in some cases necessary. For memory protection scenarios,
 * it is very important that any RAM preceding this member not be writable
 * by threads else a stack overflow will lead to silent corruption. In other
 * words, the containing data structure should live in RAM owned by the kernel.
 *
 * A user thread can only be started with a stack defined in this way if
 * the thread starting it is in supervisor mode.
 *
 * @deprecated This is now deprecated, as stacks defined in this way are not
 *             usable from user mode. Use K_KERNEL_STACK_MEMBER.
 *
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 */
#define K_THREAD_STACK_MEMBER(sym, size) __DEPRECATED_MACRO \
	Z_THREAD_STACK_DEFINE_IN(sym, size,)

/** @} */

/**
 * @brief Get a pointer to the physical stack buffer
 *
 * Obtain a pointer to the non-reserved area of a stack object.
 * This is not guaranteed to be the beginning of the thread-writable region;
 * this does not account for any memory carved-out for MPU stack overflow
 * guards.
 *
 * Use with care. The true bounds of the stack buffer are available in the
 * stack_info member of its associated thread.
 *
 * @param sym defined stack symbol name
 * @return The buffer itself, a char *
 */
static inline char *K_THREAD_STACK_BUFFER(k_thread_stack_t *sym)
{
	return (char *)sym + K_THREAD_STACK_RESERVED;
}

#endif /* CONFIG_USERSPACE */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_KERNEL_THREAD_STACK_H */
