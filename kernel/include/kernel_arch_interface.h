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
/** Cooperative context switch primitive
 *
 * The action of arch_switch() should be to switch to a new context
 * passed in the first argument, and save a pointer to the current
 * context into the address passed in the second argument.
 *
 * The actual type and interpretation of the switch handle is specified
 * by the architecture.  It is the same data structure stored in the
 * "switch_handle" field of a newly-created thread in arch_new_thread(),
 * and passed to the kernel as the "interrupted" argument to
 * z_get_next_switch_handle().
 *
 * Note that on SMP systems, the kernel uses the store through the
 * second pointer as a synchronization point to detect when a thread
 * context is completely saved (so another CPU can know when it is
 * safe to switch).  This store must be done AFTER all relevant state
 * is saved, and must include whatever memory barriers or cache
 * management code is required to be sure another CPU will see the
 * result correctly.
 *
 * The simplest implementation of arch_switch() is generally to push
 * state onto the thread stack and use the resulting stack pointer as the
 * switch handle.  Some architectures may instead decide to use a pointer
 * into the thread struct as the "switch handle" type.  These can legally
 * assume that the second argument to arch_switch() is the address of the
 * switch_handle field of struct thread_base and can use an offset on
 * this value to find other parts of the thread struct.  For example a (C
 * pseudocode) implementation of arch_switch() might look like:
 *
 *   void arch_switch(void *switch_to, void **switched_from)
 *   {
 *       struct k_thread *new = switch_to;
 *       struct k_thread *old = CONTAINER_OF(switched_from, struct k_thread,
 *                                           switch_handle);
 *
 *       // save old context...
 *       *switched_from = old;
 *       // restore new context...
 *   }
 *
 * Note that the kernel manages the switch_handle field for
 * synchronization as described above.  So it is not legal for
 * architecture code to assume that it has any particular value at any
 * other time.  In particular it is not legal to read the field from the
 * address passed in the second argument.
 *
 * @param switch_to Incoming thread's switch handle
 * @param switched_from Pointer to outgoing thread's switch handle storage
 *        location, which must be updated.
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
 * Architectures are expected to pre-allocate page tables for the entire
 * address space, as defined by CONFIG_KERNEL_VM_BASE and
 * CONFIG_KERNEL_VM_SIZE. This operation should never require any kind of
 * allocation for paging structures.
 *
 * Validation of arguments should be done via assertions.
 *
 * This API is part of infrastructure still under development and may
 * change.
 *
 * @param dest Page-aligned Destination virtual address to map
 * @param addr Page-aligned Source physical address to map
 * @param size Page-aligned size of the mapped memory region in bytes
 * @param flags Caching, access and control flags, see K_MAP_* macros
 */
void arch_mem_map(void *dest, uintptr_t addr, size_t size, uint32_t flags);

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

#ifdef CONFIG_ARCH_HAS_RESERVED_PAGE_FRAMES
/**
 * Update page frame database with reserved pages
 *
 * Some page frames within system RAM may not be available for use. A good
 * example of this is reserved regions in the first megabyte on PC-like systems.
 *
 * Implementations of this function should mark all relavent entries in
 * z_page_frames with K_PAGE_FRAME_RESERVED. This function is called at
 * early system initialization with mm_lock held.
 */
void arch_reserved_pages_update(void);
#endif /* ARCH_HAS_RESERVED_PAGE_FRAMES */

#ifdef CONFIG_DEMAND_PAGING
/**
 * Update all page tables for a paged-out data page
 *
 * This function:
 * - Sets the data page virtual address to trigger a fault if accessed that
 *   can be distinguished from access violations or un-mapped pages.
 * - Saves the provided location value so that it can retrieved for that
 *   data page in the page fault handler.
 * - The location value semantics are undefined here but the value will be
 *   always be page-aligned. It could be 0.
 *
 * If multiple page tables are in use, this must update all page tables.
 * This function is called with interrupts locked.
 *
 * Calling this function on data pages which are already paged out is
 * undefined behavior.
 *
 * This API is part of infrastructure still under development and may change.
 */
void arch_mem_page_out(void *addr, uintptr_t location);

/**
 * Update all page tables for a paged-in data page
 *
 * This function:
 * - Maps the specified virtual data page address to the provided physical
 *   page frame address, such that future memory accesses will function as
 *   expected. Access and caching attributes are undisturbed.
 * - Clears any accounting for "accessed" and "dirty" states.
 *
 * If multiple page tables are in use, this must update all page tables.
 * This function is called with interrupts locked.
 *
 * Calling this function on data pages which are already paged in is
 * undefined behavior.
 *
 * This API is part of infrastructure still under development and may change.
 */
void arch_mem_page_in(void *addr, uintptr_t phys);

/**
 * Update current page tables for a temporary mapping
 *
 * Map a physical page frame address to a special virtual address
 * Z_SCRATCH_PAGE, with read/write access to supervisor mode, such that
 * when this function returns, the calling context can read/write the page
 * frame's contents from the Z_SCRATCH_PAGE address.
 *
 * This mapping only needs to be done on the current set of page tables,
 * as it is only used for a short period of time exclusively by the caller.
 * This function is called with interrupts locked.
 *
 * This API is part of infrastructure still under development and may change.
 */
void arch_mem_scratch(uintptr_t phys);

enum arch_page_location {
	ARCH_PAGE_LOCATION_PAGED_OUT,
	ARCH_PAGE_LOCATION_PAGED_IN,
	ARCH_PAGE_LOCATION_BAD
};

/**
 * Fetch location information about a page at a particular address
 *
 * The function only needs to query the current set of page tables as
 * the information it reports must be common to all of them if multiple
 * page tables are in use. If multiple page tables are active it is unnecessary
 * to iterate over all of them. This may allow certain types of optimizations
 * (such as reverse page table mapping on x86).
 *
 * This function is called with interrupts locked, so that the reported
 * information can't become stale while decisions are being made based on it.
 *
 * Unless otherwise specified, virtual data pages have the same mappings
 * across all page tables. Calling this function on data pages that are
 * exceptions to this rule (such as the scratch page) is undefined behavior.
 * Just check the currently installed page tables and return the information
 * in that.
 *
 * @param addr Virtual data page address that took the page fault
 * @param [out] location In the case of ARCH_PAGE_FAULT_PAGED_OUT, the backing
 *        store location value used to retrieve the data page. In the case of
 *        ARCH_PAGE_FAULT_PAGED_IN, the physical address the page is mapped to.
 * @retval ARCH_PAGE_FAULT_PAGED_OUT The page was evicted to the backing store.
 * @retval ARCH_PAGE_FAULT_PAGED_IN The data page is resident in memory.
 * @retval ARCH_PAGE_FAULT_BAD The page is un-mapped or otherwise has had
 *         invalid access
 */
enum arch_page_location arch_page_location_get(void *addr, uintptr_t *location);

/**
 * @def ARCH_DATA_PAGE_ACCESSED
 *
 * Bit indicating the data page was accessed since the value was last cleared.
 *
 * Used by marking eviction algorithms. Safe to set this if uncertain.
 *
 * This bit is undefined if ARCH_DATA_PAGE_LOADED is not set.
 */

 /**
  * @def ARCH_DATA_PAGE_DIRTY
  *
  * Bit indicating the data page, if evicted, will need to be paged out.
  *
  * Set if the data page was modified since it was last paged out, or if
  * it has never been paged out before. Safe to set this if uncertain.
  *
  * This bit is undefined if ARCH_DATA_PAGE_LOADED is not set.
  */

 /**
  * @def ARCH_DATA_PAGE_LOADED
  *
  * Bit indicating that the data page is loaded into a physical page frame.
  *
  * If un-set, the data page is paged out or not mapped.
  */

/**
 * @def ARCH_DATA_PAGE_NOT_MAPPED
 *
 * If ARCH_DATA_PAGE_LOADED is un-set, this will indicate that the page
 * is not mapped at all. This bit is undefined if ARCH_DATA_PAGE_LOADED is set.
 */

/**
 * Retrieve page characteristics from the page table(s)
 *
 * The architecture is responsible for maintaining "accessed" and "dirty"
 * states of data pages to support marking eviction algorithms. This can
 * either be directly supported by hardware or emulated by modifying
 * protection policy to generate faults on reads or writes. In all cases
 * the architecture must maintain this information in some way.
 *
 * For the provided virtual address, report the logical OR of the accessed
 * and dirty states for the relevant entries in all active page tables in
 * the system if the page is mapped and not paged out.
 *
 * If clear_accessed is true, the ARCH_DATA_PAGE_ACCESSED flag will be reset.
 * This function will report its prior state. If multiple page tables are in
 * use, this function clears accessed state in all of them.
 *
 * This function is called with interrupts locked, so that the reported
 * information can't become stale while decisions are being made based on it.
 *
 * The return value may have other bits set which the caller must ignore.
 *
 * Clearing accessed state for data pages that are not ARCH_DATA_PAGE_LOADED
 * is undefined behavior.
 *
 * ARCH_DATA_PAGE_DIRTY and ARCH_DATA_PAGE_ACCESSED bits in the return value
 * are only significant if ARCH_DATA_PAGE_LOADED is set, otherwise ignore
 * them.
 *
 * ARCH_DATA_PAGE_NOT_MAPPED bit in the return value is only significant
 * if ARCH_DATA_PAGE_LOADED is un-set, otherwise ignore it.
 *
 * Unless otherwise specified, virtual data pages have the same mappings
 * across all page tables. Calling this function on data pages that are
 * exceptions to this rule (such as the scratch page) is undefined behavior.
 *
 * This API is part of infrastructure still under development and may change.
 *
 * @param addr Virtual address to look up in page tables
 * @param [out] location If non-NULL, updated with either physical page frame
 *                   address or backing store location depending on
 *                   ARCH_DATA_PAGE_LOADED state. This is not touched if
 *                   ARCH_DATA_PAGE_NOT_MAPPED.
 * @param clear_accessed Whether to clear ARCH_DATA_PAGE_ACCESSED state
 * @retval Value with ARCH_DATA_PAGE_* bits set reflecting the data page
 *         configuration
 */
uintptr_t arch_page_info_get(void *addr, uintptr_t *location,
			     bool clear_accessed);
#endif /* CONFIG_DEMAND_PAGING */
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
