/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup arch-interface Architecture Interface
 * @brief Internal kernel APIs with public scope
 *
 * Any public kernel APIs that are implemented as inline functions and need to
 * call architecture-specific API so will have the prototypes for the
 * architecture-specific APIs here. Architecture APIs that aren't used in this
 * way go in kernel/include/kernel_arch_interface.h.
 *
 * The set of architecture-specific APIs used internally by public macros and
 * inline functions in public headers are also specified and documented.
 *
 * For all macros and inline function prototypes described herein, <arch/cpu.h>
 * must eventually pull in full definitions for all of them (the actual macro
 * defines and inline function bodies)
 *
 * include/kernel.h and other public headers depend on definitions in this
 * header.
 */
#ifndef ZEPHYR_INCLUDE_SYS_ARCH_INTERFACE_H_
#define ZEPHYR_INCLUDE_SYS_ARCH_INTERFACE_H_

#ifndef _ASMLANGUAGE
#include <toolchain.h>
#include <stddef.h>
#include <zephyr/types.h>
#include <arch/cpu.h>
#include <irq_offload.h>

#ifdef __cplusplus
extern "C" {
#endif

/* NOTE: We cannot pull in kernel.h here, need some forward declarations  */
struct k_thread;
struct k_mem_domain;

typedef struct z_thread_stack_element k_thread_stack_t;

typedef void (*k_thread_entry_t)(void *p1, void *p2, void *p3);

/**
 * @defgroup arch-timing Architecture timing APIs
 * @ingroup arch-interface
 * @{
 */

/**
 * Obtain the current cycle count, in units that are hardware-specific
 *
 * @see k_cycle_get_32()
 */
static inline uint32_t arch_k_cycle_get_32(void);

/** @} */


/**
 * @addtogroup arch-threads
 * @{
 */

/**
 * @def ARCH_THREAD_STACK_RESERVED
 *
 * @see K_THREAD_STACK_RESERVED
 */

/**
 * @def ARCH_STACK_PTR_ALIGN
 *
 * Required alignment of the CPU's stack pointer register value, dictated by
 * hardware constraints and the ABI calling convention.
 *
 * @see Z_STACK_PTR_ALIGN
 */

/**
 * @def ARCH_THREAD_STACK_OBJ_ALIGN(size)
 *
 * Required alignment of the lowest address of a stack object.
 *
 * Optional definition.
 *
 * @see Z_THREAD_STACK_OBJ_ALIGN
 */

/**
 * @def ARCH_THREAD_STACK_SIZE_ADJUST(size)
 * @brief Round up a stack buffer size to alignment constraints
 *
 * Adjust a requested stack buffer size to the true size of its underlying
 * buffer, defined as the area usable for thread stack context and thread-
 * local storage.
 *
 * The size value passed here does not include storage reserved for platform
 * data.
 *
 * The returned value is either the same size provided (if already properly
 * aligned), or rounded up to satisfy alignment constraints.  Calculations
 * performed here *must* be idempotent.
 *
 * Optional definition. If undefined, stack buffer sizes are either:
 * - Rounded up to the next power of two if user mode is enabled on an arch
 *   with an MPU that requires such alignment
 * - Rounded up to ARCH_STACK_PTR_ALIGN
 *
 * @see Z_THREAD_STACK_SIZE_ADJUST
 */

/**
 * @def ARCH_KERNEL_STACK_RESERVED
 * @brief MPU guard size for kernel-only stacks
 *
 * If MPU stack guards are used to catch stack overflows, specify the
 * amount of space reserved in kernel stack objects. If guard sizes are
 * context dependent, this should be in the minimum guard size, with
 * remaining space carved out if needed.
 *
 * Optional definition, defaults to 0.
 *
 * @see K_KERNEL_STACK_RESERVED
 */

/**
 * @def ARCH_KERNEL_STACK_OBJ_ALIGN
 * @brief Required alignment of the lowest address of a kernel-only stack.
 */

/** @} */

/**
 * @addtogroup arch-pm
 * @{
 */

/**
 * @brief Power save idle routine
 *
 * This function will be called by the kernel idle loop or possibly within
 * an implementation of z_pm_save_idle in the kernel when the
 * '_pm_save_flag' variable is non-zero.
 *
 * Architectures that do not implement power management instructions may
 * immediately return, otherwise a power-saving instruction should be
 * issued to wait for an interrupt.
 *
 * @note The function is expected to return after the interrupt that has
 * caused the CPU to exit power-saving mode has been serviced, although
 * this is not a firm requirement.
 *
 * @see k_cpu_idle()
 */
void arch_cpu_idle(void);

/**
 * @brief Atomically re-enable interrupts and enter low power mode
 *
 * The requirements for arch_cpu_atomic_idle() are as follows:
 *
 * -# Enabling interrupts and entering a low-power mode needs to be
 *    atomic, i.e. there should be no period of time where interrupts are
 *    enabled before the processor enters a low-power mode.  See the comments
 *    in k_lifo_get(), for example, of the race condition that
 *    occurs if this requirement is not met.
 *
 * -# After waking up from the low-power mode, the interrupt lockout state
 *    must be restored as indicated in the 'key' input parameter.
 *
 * @see k_cpu_atomic_idle()
 *
 * @param key Lockout key returned by previous invocation of arch_irq_lock()
 */
void arch_cpu_atomic_idle(unsigned int key);

/** @} */


/**
 * @addtogroup arch-smp
 * @{
 */

/**
 * Per-cpu entry function
 *
 * @param data context parameter, implementation specific
 */
typedef FUNC_NORETURN void (*arch_cpustart_t)(void *data);

/**
 * @brief Start a numbered CPU on a MP-capable system
 *
 * This starts and initializes a specific CPU.  The main thread on startup is
 * running on CPU zero, other processors are numbered sequentially.  On return
 * from this function, the CPU is known to have begun operating and will enter
 * the provided function.  Its interrupts will be initialized but disabled such
 * that irq_unlock() with the provided key will work to enable them.
 *
 * Normally, in SMP mode this function will be called by the kernel
 * initialization and should not be used as a user API.  But it is defined here
 * for special-purpose apps which want Zephyr running on one core and to use
 * others for design-specific processing.
 *
 * @param cpu_num Integer number of the CPU
 * @param stack Stack memory for the CPU
 * @param sz Stack buffer size, in bytes
 * @param fn Function to begin running on the CPU.
 * @param arg Untyped argument to be passed to "fn"
 */
void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg);
/** @} */


/**
 * @addtogroup arch-irq
 * @{
 */

/**
 * Lock interrupts on the current CPU
 *
 * @see irq_lock()
 */
static inline unsigned int arch_irq_lock(void);

/**
 * Unlock interrupts on the current CPU
 *
 * @see irq_unlock()
 */
static inline void arch_irq_unlock(unsigned int key);

/**
 * Test if calling arch_irq_unlock() with this key would unlock irqs
 *
 * @param key value returned by arch_irq_lock()
 * @return true if interrupts were unlocked prior to the arch_irq_lock()
 * call that produced the key argument.
 */
static inline bool arch_irq_unlocked(unsigned int key);

/**
 * Disable the specified interrupt line
 *
 * @see irq_disable()
 */
void arch_irq_disable(unsigned int irq);

/**
 * Enable the specified interrupt line
 *
 * @see irq_enable()
 */
void arch_irq_enable(unsigned int irq);

/**
 * Test if an interrupt line is enabled
 *
 * @see irq_is_enabled()
 */
int arch_irq_is_enabled(unsigned int irq);

/**
 * Arch-specific hook to install a dynamic interrupt.
 *
 * @param irq IRQ line number
 * @param priority Interrupt priority
 * @param routine Interrupt service routine
 * @param parameter ISR parameter
 * @param flags Arch-specific IRQ configuration flag
 *
 * @return The vector assigned to this interrupt
 */
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter),
			     const void *parameter, uint32_t flags);

/**
 * @def ARCH_IRQ_CONNECT(irq, pri, isr, arg, flags)
 *
 * @see IRQ_CONNECT()
 */

/**
 * @def ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p)
 *
 * @see IRQ_DIRECT_CONNECT()
 */

/**
 * @def ARCH_ISR_DIRECT_PM()
 *
 * @see ISR_DIRECT_PM()
 */

/**
 * @def ARCH_ISR_DIRECT_HEADER()
 *
 * @see ISR_DIRECT_HEADER()
 */

/**
 * @def ARCH_ISR_DIRECT_FOOTER(swap)
 *
 * @see ISR_DIRECT_FOOTER()
 */

/**
 * @def ARCH_ISR_DIRECT_DECLARE(name)
 *
 * @see ISR_DIRECT_DECLARE()
 */

/**
 * @def ARCH_EXCEPT(reason_p)
 *
 * Generate a software induced fatal error.
 *
 * If the caller is running in user mode, only K_ERR_KERNEL_OOPS or
 * K_ERR_STACK_CHK_FAIL may be induced.
 *
 * This should ideally generate a software trap, with exception context
 * indicating state when this was invoked. General purpose register state at
 * the time of trap should not be disturbed from the calling context.
 *
 * @param reason_p K_ERR_ scoped reason code for the fatal error.
 */

#ifdef CONFIG_IRQ_OFFLOAD
/**
 * Run a function in interrupt context.
 *
 * Implementations should invoke an exception such that the kernel goes through
 * its interrupt handling dispatch path, to include switching to the interrupt
 * stack, and runs the provided routine and parameter.
 *
 * The only intended use-case for this function is for test code to simulate
 * the correctness of kernel APIs in interrupt handling context. This API
 * is not intended for real applications.
 *
 * @see irq_offload()
 *
 * @param routine Function to run in interrupt context
 * @param parameter Value to pass to the function when invoked
 */
void arch_irq_offload(irq_offload_routine_t routine, const void *parameter);
#endif /* CONFIG_IRQ_OFFLOAD */

/** @} */


/**
 * @defgroup arch-smp Architecture-specific SMP APIs
 * @ingroup arch-interface
 * @{
 */
#ifdef CONFIG_SMP
/** Return the CPU struct for the currently executing CPU */
static inline struct _cpu *arch_curr_cpu(void);

/**
 * Broadcast an interrupt to all CPUs
 *
 * This will invoke z_sched_ipi() on other CPUs in the system.
 */
void arch_sched_ipi(void);
#endif /* CONFIG_SMP */

/** @} */


/**
 * @defgroup arch-userspace Architecture-specific userspace APIs
 * @ingroup arch-interface
 * @{
 */

#ifdef CONFIG_USERSPACE
/**
 * Invoke a system call with 0 arguments.
 *
 * No general-purpose register state other than return value may be preserved
 * when transitioning from supervisor mode back down to user mode for
 * security reasons.
 *
 * It is required that all arguments be stored in registers when elevating
 * privileges from user to supervisor mode.
 *
 * Processing of the syscall takes place on a separate kernel stack. Interrupts
 * should be enabled when invoking the system call marshallers from the
 * dispatch table. Thread preemption may occur when handling system calls.
 *
 * Call ids are untrusted and must be bounds-checked, as the value is used to
 * index the system call dispatch table, containing function pointers to the
 * specific system call code.
 *
 * @param call_id System call ID
 * @return Return value of the system call. Void system calls return 0 here.
 */
static inline uintptr_t arch_syscall_invoke0(uintptr_t call_id);

/**
 * Invoke a system call with 1 argument.
 *
 * @see arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static inline uintptr_t arch_syscall_invoke1(uintptr_t arg1,
					     uintptr_t call_id);

/**
 * Invoke a system call with 2 arguments.
 *
 * @see arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param arg2 Second argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static inline uintptr_t arch_syscall_invoke2(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t call_id);

/**
 * Invoke a system call with 3 arguments.
 *
 * @see arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param arg2 Second argument to the system call.
 * @param arg3 Third argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static inline uintptr_t arch_syscall_invoke3(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3,
					     uintptr_t call_id);

/**
 * Invoke a system call with 4 arguments.
 *
 * @see arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param arg2 Second argument to the system call.
 * @param arg3 Third argument to the system call.
 * @param arg4 Fourth argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static inline uintptr_t arch_syscall_invoke4(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t call_id);

/**
 * Invoke a system call with 5 arguments.
 *
 * @see arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param arg2 Second argument to the system call.
 * @param arg3 Third argument to the system call.
 * @param arg4 Fourth argument to the system call.
 * @param arg5 Fifth argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static inline uintptr_t arch_syscall_invoke5(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5,
					     uintptr_t call_id);

/**
 * Invoke a system call with 6 arguments.
 *
 * @see arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param arg2 Second argument to the system call.
 * @param arg3 Third argument to the system call.
 * @param arg4 Fourth argument to the system call.
 * @param arg5 Fifth argument to the system call.
 * @param arg6 Sixth argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static inline uintptr_t arch_syscall_invoke6(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5, uintptr_t arg6,
					     uintptr_t call_id);

/**
 * Indicate whether we are currently running in user mode
 *
 * @return true if the CPU is currently running with user permissions
 */
static inline bool arch_is_user_context(void);

/**
 * @brief Get the maximum number of partitions for a memory domain
 *
 * @return Max number of partitions, or -1 if there is no limit
 */
int arch_mem_domain_max_partitions_get(void);

#ifdef CONFIG_ARCH_MEM_DOMAIN_DATA
/**
 *
 * @brief Architecture-specific hook for memory domain initialization
 *
 * Perform any tasks needed to initialize architecture-specific data within
 * the memory domain, such as reserving memory for page tables. All members
 * of the provided memory domain aside from `arch` will be initialized when
 * this is called, but no threads will be a assigned yet.
 *
 * This function may fail if initializing the memory domain requires allocation,
 * such as for page tables.
 *
 * The associated function k_mem_domain_init() documents that making
 * multiple init calls to the same memory domain is undefined behavior,
 * but has no assertions in place to check this. If this matters, it may be
 * desirable to add checks for this in the implementation of this function.
 *
 * @param domain The memory domain to initialize
 * @retval 0 Success
 * @retval -ENOMEM Insufficient memory
 */
int arch_mem_domain_init(struct k_mem_domain *domain);
#endif /* CONFIG_ARCH_MEM_DOMAIN_DATA */

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
/**
 * @brief Add a thread to a memory domain (arch-specific)
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when the provided thread has been added to a memory domain.
 *
 * The thread->mem_domain_info.mem_domain pointer will be set to the domain to
 * be added to before this is called. Implementations may assume that the
 * thread is not already a member of this domain.
 *
 * @param thread Thread which needs to be configured.
 */
void arch_mem_domain_thread_add(struct k_thread *thread);

/**
 * @brief Remove a thread from a memory domain (arch-specific)
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when the provided thread has been removed from a memory domain.
 *
 * The thread's memory domain pointer will be the domain that the thread
 * is being removed from.
 *
 * @param thread Thread being removed from its memory domain
 */
void arch_mem_domain_thread_remove(struct k_thread *thread);

/**
 * @brief Remove a partition from the memory domain (arch-specific)
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when a memory domain has had a partition removed.
 *
 * The partition index data, and the number of partitions configured, are not
 * respectively cleared and decremented in the domain until after this function
 * runs.
 *
 * @param domain The memory domain structure
 * @param partition_id The partition index that needs to be deleted
 */
void arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				      uint32_t partition_id);

/**
 * @brief Add a partition to the memory domain
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when a memory domain has a partition added.
 *
 * @param domain The memory domain structure
 * @param partition_id The partition that needs to be added
 */
void arch_mem_domain_partition_add(struct k_mem_domain *domain,
				   uint32_t partition_id);

/**
 * @brief Remove the memory domain
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when a memory domain has been destroyed.
 *
 * Thread assignments to the memory domain are only cleared after this function
 * runs.
 *
 * @param domain The memory domain structure which needs to be deleted.
 */
void arch_mem_domain_destroy(struct k_mem_domain *domain);
#endif /* CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API */

/**
 * @brief Check memory region permissions
 *
 * Given a memory region, return whether the current memory management hardware
 * configuration would allow a user thread to read/write that region. Used by
 * system calls to validate buffers coming in from userspace.
 *
 * Notes:
 * The function is guaranteed to never return validation success, if the entire
 * buffer area is not user accessible.
 *
 * The function is guaranteed to correctly validate the permissions of the
 * supplied buffer, if the user access permissions of the entire buffer are
 * enforced by a single, enabled memory management region.
 *
 * In some architectures the validation will always return failure
 * if the supplied memory buffer spans multiple enabled memory management
 * regions (even if all such regions permit user access).
 *
 * @warning 0 size buffer has undefined behavior.
 *
 * @param addr start address of the buffer
 * @param size the size of the buffer
 * @param write If nonzero, additionally check if the area is writable.
 *	  Otherwise, just check if the memory can be read.
 *
 * @return nonzero if the permissions don't match.
 */
int arch_buffer_validate(void *addr, size_t size, int write);

/**
 * Perform a one-way transition from supervisor to kernel mode.
 *
 * Implementations of this function must do the following:
 *
 * - Reset the thread's stack pointer to a suitable initial value. We do not
 *   need any prior context since this is a one-way operation.
 * - Set up any kernel stack region for the CPU to use during privilege
 *   elevation
 * - Put the CPU in whatever its equivalent of user mode is
 * - Transfer execution to arch_new_thread() passing along all the supplied
 *   arguments, in user mode.
 *
 * @param user_entry Entry point to start executing as a user thread
 * @param p1 1st parameter to user thread
 * @param p2 2nd parameter to user thread
 * @param p3 3rd parameter to user thread
 */
FUNC_NORETURN void arch_user_mode_enter(k_thread_entry_t user_entry,
					void *p1, void *p2, void *p3);

/**
 * @brief Induce a kernel oops that appears to come from a specific location
 *
 * Normally, k_oops() generates an exception that appears to come from the
 * call site of the k_oops() itself.
 *
 * However, when validating arguments to a system call, if there are problems
 * we want the oops to appear to come from where the system call was invoked
 * and not inside the validation function.
 *
 * @param ssf System call stack frame pointer. This gets passed as an argument
 *            to _k_syscall_handler_t functions and its contents are completely
 *            architecture specific.
 */
FUNC_NORETURN void arch_syscall_oops(void *ssf);

/**
 * @brief Safely take the length of a potentially bad string
 *
 * This must not fault, instead the err parameter must have -1 written to it.
 * This function otherwise should work exactly like libc strnlen(). On success
 * *err should be set to 0.
 *
 * @param s String to measure
 * @param maxsize Max length of the string
 * @param err Error value to write
 * @return Length of the string, not counting NULL byte, up to maxsize
 */
size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err);
#endif /* CONFIG_USERSPACE */

/**
 * @brief Detect memory coherence type
 *
 * Required when ARCH_HAS_COHERENCE is true.  This function returns
 * true if the byte pointed to lies within an architecture-defined
 * "coherence region" (typically implemented with uncached memory) and
 * can safely be used in multiprocessor code without explicit flush or
 * invalidate operations.
 *
 * @note The result is for only the single byte at the specified
 * address, this API is not required to check region boundaries or to
 * expect aligned pointers.  The expectation is that the code above
 * will have queried the appropriate address(es).
 */
#ifndef CONFIG_ARCH_HAS_COHERENCE
static inline bool arch_mem_coherent(void *ptr)
{
	ARG_UNUSED(ptr);
	return true;
}
#endif

/**
 * @brief Ensure cache coherence prior to context switch
 *
 * Required when ARCH_HAS_COHERENCE is true.  On cache-incoherent
 * multiprocessor architectures, thread stacks are cached by default
 * for performance reasons.  They must therefore be flushed
 * appropriately on context switch.  The rules are:
 *
 * 1. The region containing live data in the old stack (generally the
 *    bytes between the current stack pointer and the top of the stack
 *    memory) must be flushed to underlying storage so a new CPU that
 *    runs the same thread sees the correct data.  This must happen
 *    before the assignment of the switch_handle field in the thread
 *    struct which signals the completion of context switch.
 *
 * 2. Any data areas to be read from the new stack (generally the same
 *    as the live region when it was saved) should be invalidated (and
 *    NOT flushed!) in the data cache.  This is because another CPU
 *    may have run or re-initialized the thread since this CPU
 *    suspended it, and any data present in cache will be stale.
 *
 * @note The kernel will call this function during interrupt exit when
 * a new thread has been chosen to run, and also immediately before
 * entering arch_switch() to effect a code-driven context switch.  In
 * the latter case, it is very likely that more data will be written
 * to the old_thread stack region after this function returns but
 * before the completion of the switch.  Simply flushing naively here
 * is not sufficient on many architectures and coordination with the
 * arch_switch() implementation is likely required.
 *
 * @arg old_thread The old thread to be flushed before being allowed
 *                 to run on other CPUs.
 * @arg old_switch_handle The switch handle to be stored into
 *                        old_thread (it will not be valid until the
 *                        cache is flushed so is not present yet).
 *                        This will be NULL if inside z_swap()
 *                        (because the arch_switch() has not saved it
 *                        yet).
 * @arg new_thread The new thread to be invalidated before it runs locally.
 */
#ifndef CONFIG_KERNEL_COHERENCE
static inline void arch_cohere_stacks(struct k_thread *old_thread,
				      void *old_switch_handle,
				      struct k_thread *new_thread)
{
	ARG_UNUSED(old_thread);
	ARG_UNUSED(old_switch_handle);
	ARG_UNUSED(new_thread);
}
#endif

/** @} */

/**
 * @defgroup arch-gdbstub Architecture-specific gdbstub APIs
 * @ingroup arch-interface
 * @{
 */

/**
 * @def ARCH_GDB_NUM_REGISTERS
 *
 * ARCH_GDB_NUM_REGISTERS is architecure specific and
 * this symbol must be defined in architecure specific header
 */

#ifdef CONFIG_GDBSTUB
/**
 * @brief Architecture layer debug start
 *
 * This function is called by @c gdb_init()
 */
void arch_gdb_init(void);

/**
 * @brief Continue running program
 *
 * Continue software execution.
 */
void arch_gdb_continue(void);

/**
 * @brief Continue with one step
 *
 * Continue software execution until reaches the next statement.
 */
void arch_gdb_step(void);

#endif
/** @} */

/**
 * @defgroup arch_cache Architecture-specific cache functions
 * @ingroup arch-interface
 * @{
 */

#ifdef CONFIG_CACHE_MANAGEMENT
/**
 *
 * @brief Enable d-cache
 *
 * @see arch_dcache_enable
 */
void arch_dcache_enable(void);

/**
 *
 * @brief Disable d-cache
 *
 * @see arch_dcache_disable
 */
void arch_dcache_disable(void);

/**
 *
 * @brief Enable i-cache
 *
 * @see arch_icache_enable
 */
void arch_icache_enable(void);

/**
 *
 * @brief Enable i-cache
 *
 * @see arch_dcache_disable
 */
void arch_dcache_disable(void);

/**
 *
 * @brief Write-back / Invalidate / Write-back + Invalidate all d-cache
 *
 * @see arch_dcache_all
 */
int arch_dcache_all(int op);

/**
 *
 * @brief Write-back / Invalidate / Write-back + Invalidate d-cache lines
 *
 * @see arch_dcache_range
 */
int arch_dcache_range(void *addr, size_t size, int op);

/**
 *
 * @brief Write-back / Invalidate / Write-back + Invalidate all i-cache
 *
 * @see arch_icache_all
 */
int arch_icache_all(int op);

/**
 *
 * @brief Write-back / Invalidate / Write-back + Invalidate i-cache lines
 *
 * @see arch_icache_range
 */
int arch_icache_range(void *addr, size_t size, int op);

#ifdef CONFIG_DCACHE_LINE_SIZE_DETECT
/**
 *
 * @brief Get d-cache line size
 *
 * @see sys_dcache_line_size_get
 */
size_t arch_dcache_line_size_get(void);
#endif /* CONFIG_DCACHE_LINE_SIZE_DETECT */

#ifdef CONFIG_ICACHE_LINE_SIZE_DETECT
/**
 *
 * @brief Get i-cache line size
 *
 * @see sys_icache_line_size_get
 */
size_t arch_icache_line_size_get(void);
#endif /* CONFIG_ICACHE_LINE_SIZE_DETECT */

#endif /* CONFIG_CACHE_MANAGEMENT */

/** @} */

#ifdef CONFIG_TIMING_FUNCTIONS
#include <timing/types.h>

/**
 * @ingroup arch-interface timing_api
 */

/**
 * @brief Initialize the timing subsystem.
 *
 * Perform the necessary steps to initialize the timing subsystem.
 *
 * @see timing_init()
 */
void arch_timing_init(void);

/**
 * @brief Signal the start of the timing information gathering.
 *
 * Signal to the timing subsystem that timing information
 * will be gathered from this point forward.
 *
 * @see timing_start()
 */
void arch_timing_start(void);

/**
 * @brief Signal the end of the timing information gathering.
 *
 * Signal to the timing subsystem that timing information
 * is no longer being gathered from this point forward.
 *
 * @see timing_stop()
 */
void arch_timing_stop(void);

/**
 * @brief Return timing counter.
 *
 * @return Timing counter.
 *
 * @see timing_counter_get()
 */
timing_t arch_timing_counter_get(void);

/**
 * @brief Get number of cycles between @p start and @p end.
 *
 * For some architectures or SoCs, the raw numbers from counter
 * need to be scaled to obtain actual number of cycles.
 *
 * @param start Pointer to counter at start of a measured execution.
 * @param end Pointer to counter at stop of a measured execution.
 * @return Number of cycles between start and end.
 *
 * @see timing_cycles_get()
 */
uint64_t arch_timing_cycles_get(volatile timing_t *const start,
				volatile timing_t *const end);

/**
 * @brief Get frequency of counter used (in Hz).
 *
 * @return Frequency of counter used for timing in Hz.
 *
 * @see timing_freq_get()
 */
uint64_t arch_timing_freq_get(void);

/**
 * @brief Convert number of @p cycles into nanoseconds.
 *
 * @param cycles Number of cycles
 * @return Converted time value
 *
 * @see timing_cycles_to_ns()
 */
uint64_t arch_timing_cycles_to_ns(uint64_t cycles);

/**
 * @brief Convert number of @p cycles into nanoseconds with averaging.
 *
 * @param cycles Number of cycles
 * @param count Times of accumulated cycles to average over
 * @return Converted time value
 *
 * @see timing_cycles_to_ns_avg()
 */
uint64_t arch_timing_cycles_to_ns_avg(uint64_t cycles, uint32_t count);

/**
 * @brief Get frequency of counter used (in MHz).
 *
 * @return Frequency of counter used for timing in MHz.
 *
 * @see timing_freq_get_mhz()
 */
uint32_t arch_timing_freq_get_mhz(void);

/* @} */

#endif /* CONFIG_TIMING_FUNCTIONS */

#ifdef CONFIG_PCIE_MSI_MULTI_VECTOR

struct msi_vector;
typedef struct msi_vector msi_vector_t;

/**
 * @brief Allocate vector(s) for the endpoint MSI message(s).
 *
 * @param priority the MSI vectors base interrupt priority
 * @param vectors an array to fill with allocated MSI vectors
 * @param n_vector the size of MSI vectors array
 *
 * @return The number of allocated MSI vectors
 */
uint8_t arch_pcie_msi_vectors_allocate(unsigned int priority,
				       msi_vector_t *vectors,
				       uint8_t n_vector);

/**
 * @brief Connect an MSI vector to the given routine
 *
 * @param vector The MSI vector to connect to
 * @param routine Interrupt service routine
 * @param parameter ISR parameter
 * @param flags Arch-specific IRQ configuration flag
 *
 * @return True on success, false otherwise
 */
bool arch_pcie_msi_vector_connect(msi_vector_t *vector,
				  void (*routine)(const void *parameter),
				  const void *parameter,
				  uint32_t flags);

#endif /* CONFIG_PCIE_MSI_MULTI_VECTOR */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include <arch/arch_inlines.h>

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_SYS_ARCH_INTERFACE_H_ */
