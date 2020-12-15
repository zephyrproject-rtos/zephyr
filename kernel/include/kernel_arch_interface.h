/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal kernel APIs implemented at the architecture layer.
 *
 * Not all architecture-specific defines are here, APIs that are used
 * by public functions and macros are defined in include/sys/arch_interface.h.
 *
 * For all inline functions prototyped here, the implementation is expected
 * to be provided by arch/ARCH/include/kernel_arch_func.h
 */
#ifndef ZEPHYR_KERNEL_INCLUDE_KERNEL_ARCH_INTERFACE_H_
#define ZEPHYR_KERNEL_INCLUDE_KERNEL_ARCH_INTERFACE_H_

#include <kernel.h>
#include <sys/arch_interface.h>

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup arch-timing Architecture timing APIs
 * @{
 */
#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
/**
 * Architecture-specific implementation of busy-waiting
 *
 * @param usec_to_wait Wait period, in microseconds
 */
void arch_busy_wait(uint32_t usec_to_wait);
#endif

/** @} */

/**
 * @defgroup arch-threads Architecture thread APIs
 * @ingroup arch-interface
 * @{
 */

/** Handle arch-specific logic for setting up new threads
 *
 * The stack and arch-specific thread state variables must be set up
 * such that a later attempt to switch to this thread will succeed
 * and we will enter z_thread_entry with the requested thread and
 * arguments as its parameters.
 *
 * At some point in this function's implementation, z_setup_new_thread() must
 * be called with the true bounds of the available stack buffer within the
 * thread's stack object.
 *
 * The provided stack pointer is guaranteed to be properly aligned with respect
 * to the CPU and ABI requirements. There may be space reserved between the
 * stack pointer and the bounds of the stack buffer for initial stack pointer
 * randomization and thread-local storage.
 *
 * Fields in thread->base will be initialized when this is called.
 *
 * @param thread Pointer to uninitialized struct k_thread
 * @param stack Pointer to the stack object
 * @param stack_ptr Aligned initial stack pointer
 * @param entry Thread entry function
 * @param p1 1st entry point parameter
 * @param p2 2nd entry point parameter
 * @param p3 3rd entry point parameter
 */
void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3);

#ifdef CONFIG_USE_SWITCH
/**
 * Cooperatively context switch
 *
 * Architectures have considerable leeway on what the specific semantics of
 * the switch handles are, but optimal implementations should do the following
 * if possible:
 *
 * 1) Push all thread state relevant to the context switch to the current stack
 * 2) Update the switched_from parameter to contain the current stack pointer,
 *    after all context has been saved. switched_from is used as an output-
 *    only parameter and its current value is ignored (and can be NULL, see
 *    below).
 * 3) Set the stack pointer to the value provided in switch_to
 * 4) Pop off all thread state from the stack we switched to and return.
 *
 * Some arches may implement thread->switch handle as a pointer to the
 * thread itself, and save context somewhere in thread->arch. In this
 * case, on initial context switch from the dummy thread,
 * thread->switch handle for the outgoing thread is NULL. Instead of
 * dereferencing switched_from all the way to get the thread pointer,
 * subtract ___thread_t_switch_handle_OFFSET to obtain the thread
 * pointer instead.  That is, such a scheme would have behavior like
 * (in C pseudocode):
 *
 * void arch_switch(void *switch_to, void **switched_from)
 * {
 *     struct k_thread *new = switch_to;
 *     struct k_thread *old = CONTAINER_OF(switched_from, struct k_thread,
 *                                         switch_handle);
 *
 *     // save old context...
 *     *switched_from = old;
 *     // restore new context...
 * }
 *
 * Note that, regardless of the underlying handle representation, the
 * incoming switched_from pointer MUST be written through with a
 * non-NULL value after all relevant thread state has been saved.  The
 * kernel uses this as a synchronization signal to be able to wait for
 * switch completion from another CPU.
 *
 * @param switch_to Incoming thread's switch handle
 * @param switched_from Pointer to outgoing thread's switch handle storage
 *        location, which may be updated.
 */
static inline void arch_switch(void *switch_to, void **switched_from);
#else
/**
 * Cooperatively context switch
 *
 * Must be called with interrupts locked with the provided key.
 * This is the older-style context switching method, which is incompatible
 * with SMP. New arch ports, either SMP or UP, are encouraged to implement
 * arch_switch() instead.
 *
 * @param key Interrupt locking key
 * @return If woken from blocking on some kernel object, the result of that
 *         blocking operation.
 */
int arch_swap(unsigned int key);

/**
 * Set the return value for the specified thread.
 *
 * It is assumed that the specified @a thread is pending.
 *
 * @param thread Pointer to thread object
 * @param value value to set as return value
 */
static ALWAYS_INLINE void
arch_thread_return_value_set(struct k_thread *thread, unsigned int value);
#endif /* CONFIG_USE_SWITCH i*/

#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN
/**
 * Custom logic for entering main thread context at early boot
 *
 * Used by architectures where the typical trick of setting up a dummy thread
 * in early boot context to "switch out" of isn't workable.
 *
 * @param main_thread main thread object
 * @param stack_ptr Initial stack pointer
 * @param _main Entry point for application main function.
 */
void arch_switch_to_main_thread(struct k_thread *main_thread, char *stack_ptr,
				k_thread_entry_t _main);
#endif /* CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN */

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
/**
 * @brief Disable floating point context preservation
 *
 * The function is used to disable the preservation of floating
 * point context information for a particular thread.
 *
 * @note For ARM architecture, disabling floating point preservation may only
 * be requested for the current thread and cannot be requested in ISRs.
 *
 * @retval 0       On success.
 * @retval -EINVAL If the floating point disabling could not be performed.
 */
int arch_float_disable(struct k_thread *thread);
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

/** @} */

/**
 * @defgroup arch-pm Architecture-specific power management APIs
 * @ingroup arch-interface
 * @{
 */
/** Halt the system, optionally propagating a reason code */
FUNC_NORETURN void arch_system_halt(unsigned int reason);

/** @} */


/**
 * @defgroup arch-irq Architecture-specific IRQ APIs
 * @ingroup arch-interface
 * @{
 */

/**
 * Test if the current context is in interrupt context
 *
 * XXX: This is inconsistently handled among arches wrt exception context
 * See: #17656
 *
 * @return true if we are in interrupt context
 */
static inline bool arch_is_in_isr(void);

/** @} */

/**
 * @defgroup arch-mmu Architecture-specific memory-mapping APIs
 * @ingroup arch-interface
 * @{
 */

#ifdef CONFIG_MMU
/**
 * Map physical memory into the virtual address space
 *
 * This is a low-level interface to mapping pages into the address space.
 * Behavior when providing unaligned addresses/sizes is undefined, these
 * are assumed to be aligned to CONFIG_MMU_PAGE_SIZE.
 *
 * The core kernel handles all management of the virtual address space;
 * by the time we invoke this function, we know exactly where this mapping
 * will be established. If the page tables already had mappings installed
 * for the virtual memory region, these will be overwritten.
 *
 * If the target architecture supports multiple page sizes, currently
 * only the smallest page size will be used.
 *
 * The memory range itself is never accessed by this operation.
 *
 * This API must be safe to call in ISRs or exception handlers. Calls
 * to this API are assumed to be serialized, and indeed all usage will
 * originate from kernel/mm.c which handles virtual memory management.
 *
 * This API is part of infrastructure still under development and may
 * change.
 *
 * @param dest Page-aligned Destination virtual address to map
 * @param addr Page-aligned Source physical address to map
 * @param size Page-aligned size of the mapped memory region in bytes
 * @param flags Caching, access and control flags, see K_MAP_* macros
 * @retval 0 Success
 * @retval -ENOTSUP Unsupported cache mode with no suitable fallback, or
 *	   unsupported flags
 * @retval -ENOMEM Memory for additional paging structures unavailable
 */
int arch_mem_map(void *dest, uintptr_t addr, size_t size, uint32_t flags);

/**
 * Remove mappings for a provided virtual address range
 *
 * This is a low-level interface for un-mapping pages from the address space.
 * When this completes, the relevant page table entries will be updated as
 * if no mapping was ever made for that memory range. No previous context
 * needs to be preserved. This function must update mappings in all active
 * page tables.
 *
 * Behavior when providing unaligned addresses/sizes is undefined, these
 * are assumed to be aligned to CONFIG_MMU_PAGE_SIZE.
 *
 * Behavior when providing an address range that is not already mapped is
 * undefined.
 *
 * This function should never require memory allocations for paging structures,
 * and it is not necessary to free any paging structures. Empty page tables
 * due to all contained entries being un-mapped may remain in place.
 *
 * Implementations must invalidate TLBs as necessary.
 *
 * This API is part of infrastructure still under development and may change.
 *
 * @param addr Page-aligned base virtual address to un-map
 * @param size Page-aligned region size
 */
void arch_mem_unmap(void *addr, size_t size);
#endif /* CONFIG_MMU */
/** @} */

/**
 * @defgroup arch-misc Miscellaneous architecture APIs
 * @ingroup arch-interface
 * @{
 */

/**
 * Early boot console output hook
 *
 * Definition of this function is optional. If implemented, any invocation
 * of printk() (or logging calls with CONFIG_LOG_MINIMAL which are backed by
 * printk) will default to sending characters to this function. It is
 * useful for early boot debugging before main serial or console drivers
 * come up.
 *
 * This can be overridden at runtime with __printk_hook_install().
 *
 * The default __weak implementation of this does nothing.
 *
 * @param c Character to print
 * @return The character printed
 */
int arch_printk_char_out(int c);

/**
 * Architecture-specific kernel initialization hook
 *
 * This function is invoked near the top of _Cstart, for additional
 * architecture-specific setup before the rest of the kernel is brought up.
 *
 * TODO: Deprecate, most arches are using a prep_c() function to do the same
 * thing in a simpler way
 */
static inline void arch_kernel_init(void);

/** Do nothing and return. Yawn. */
static inline void arch_nop(void);

/** @} */

/**
 * @defgroup arch-coredump Architecture-specific core dump APIs
 * @ingroup arch-interface
 * @{
 */

/**
 * @brief Architecture-specific handling during coredump
 *
 * This dumps architecture-specific information during coredump.
 *
 * @param esf Exception Stack Frame (arch-specific)
 */
void arch_coredump_info_dump(const z_arch_esf_t *esf);

/**
 * @brief Get the target code specified by the architecture.
 */
uint16_t arch_coredump_tgt_code_get(void);

/** @} */

/**
 * @defgroup arch-tls Architecture-specific Thread Local Storage APIs
 * @ingroup arch-interface
 * @{
 */

/**
 * @brief Setup Architecture-specific TLS area in stack
 *
 * This sets up the stack area for thread local storage.
 * The structure inside in area is architecture specific.
 *
 * @param new_thread New thread object
 * @param stack_ptr Stack pointer
 * @return Number of bytes taken by the TLS area
 */
size_t arch_tls_stack_setup(struct k_thread *new_thread, char *stack_ptr);

/** @} */

/* Include arch-specific inline function implementation */
#include <kernel_arch_func.h>

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_ARCH_INTERFACE_H_ */
