/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal kernel APIs with public scope
 *
 * The main set of architecture APIs is specified by
 * include/sys/arch_interface.h
 *
 * Any public kernel APIs that are implemented as inline functions and need to
 * call architecture-specific APIso will have the prototypes for the
 * architecture-specific APIs here. Architecture APIs that aren't used in this
 * way go in include/sys/arch_interface.h.
 *
 * The set of architecture-specific macros used internally by public macros
 * in public headers is also specified and documented.
 *
 * For all macros and inline function prototypes described herein, <arch/cpu.h>
 * must eventually pull in full definitions for all of them (the actual macro
 * defines and inline function bodies)
 *
 * include/kernel.h and other public headers depend on definitions in this
 * header.
 */

#ifndef ZEPHYR_INCLUDE_SYS_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_SYS_ARCH_INLINES_H_

#ifndef _ASMLANGUAGE

#include <stdbool.h>
#include <zephyr/types.h>
#include <arch/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

/* NOTE: We cannot pull in kernel.h here, need some forward declarations  */
struct k_thread;
typedef struct _k_thread_stack_element k_thread_stack_t;

/**
 * @addtogroup arch-timing
 * @{
 */

/**
 * Obtain the current cycle count, in units that are hardware-specific
 *
 * @see k_cycle_get_32()
 */
static inline u32_t z_arch_k_cycle_get_32(void);

/** @} */


/**
 * @addtogroup arch-threads
 * @{
 */

/**
 * @def Z_ARCH_THREAD_STACK_DEFINE(sym, size)
 *
 * @see K_THREAD_STACK_DEFINE()
 */

/**
 * @def Z_ARCH_THREAD_STACK_ARRAY_DEFINE(sym, size)
 *
 * @see K_THREAD_STACK_ARRAY_DEFINE()
 */

/**
 * @def Z_ARCH_THREAD_STACK_LEN(size)
 *
 * @see K_THREAD_STACK_LEN()
 */

/**
 * @def Z_ARCH_THREAD_STACK_MEMBER(sym, size)
 *
 * @see K_THREAD_STACK_MEMBER()
 */

/*
 * @def Z_ARCH_THREAD_STACK_SIZEOF(sym)
 *
 * @see K_THREAD_STACK_SIZEOF()
 */

/**
 * @def Z_ARCH_THREAD_STACK_RESERVED
 *
 * @see K_THREAD_STACK_RESERVED
 */

/**
 * @def Z_ARCH_THREAD_STACK_BUFFER(sym)
 *
 * @see K_THREAD_STACK_RESERVED
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
 * an implementation of z_sys_power_save_idle in the kernel when the
 * '_sys_power_save_flag' variable is non-zero.
 *
 * Architectures that do not implement power management instructions may
 * immediately return, otherwise a power-saving instruction should be
 * issued to wait for an interrupt.
 *
 * @see k_cpu_idle()
 */
void z_arch_cpu_idle(void);

/**
 * @brief Atomically re-enable interrupts and enter low power mode
 *
 * The requirements for z_arch_cpu_atomic_idle() are as follows:
 *
 * 1) Enabling interrupts and entering a low-power mode needs to be
 *    atomic, i.e. there should be no period of time where interrupts are
 *    enabled before the processor enters a low-power mode.  See the comments
 *    in k_lifo_get(), for example, of the race condition that
 *    occurs if this requirement is not met.
 *
 * 2) After waking up from the low-power mode, the interrupt lockout state
 *    must be restored as indicated in the 'key' input parameter.
 *
 * @see k_cpu_atomic_idle()
 *
 * @param key Lockout key returned by previous invocation of z_arch_irq_lock()
 */
void z_arch_cpu_atomic_idle(unsigned int key);

/** @} */


/**
 * @addtogroup arch-smp
 * @{
 */

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
 * @param fn Function to begin running on the CPU.  First argument is
 *        an irq_unlock() key.
 * @param arg Untyped argument to be passed to "fn"
 */
void z_arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		      void (*fn)(int key, void *data), void *arg);
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
static inline unsigned int z_arch_irq_lock(void);

/**
 * Unlock interrupts on the current CPU
 *
 * @see irq_unlock()
 */
static inline void z_arch_irq_unlock(unsigned int key);

/**
 * Test if calling z_arch_irq_unlock() with this key would unlock irqs
 *
 * @param key value returned by z_arch_irq_lock()
 * @return true if interrupts were unlocked prior to the z_arch_irq_lock()
 * call that produced the key argument.
 */
static inline bool z_arch_irq_unlocked(unsigned int key);

/**
 * Disable the specified interrupt line
 *
 * @see irq_disable()
 */
void z_arch_irq_disable(unsigned int irq);

/**
 * Enable the specified interrupt line
 *
 * @see irq_enable()
 */
void z_arch_irq_enable(unsigned int irq);

/**
 * Test if an interrupt line is enabled
 *
 * @see irq_is_enabled()
 */
int z_arch_irq_is_enabled(unsigned int irq);

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
int z_arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			       void (*routine)(void *parameter),
			       void *parameter, u32_t flags);

/**
 * @def Z_ARCH_IRQ_CONNECT(irq, pri, isr, arg, flags)
 *
 * @see IRQ_CONNECT()
 */

/**
 * @def Z_ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p)
 *
 * @see IRQ_DIRECT_CONNECT()
 */

/**
 * @def Z_ARCH_ISR_DIRECT_PM()
 *
 * @see ISR_DIRECT_PM()
 */

/**
 * @def Z_ARCH_ISR_DIRECT_HEADER()
 *
 * @see ISR_DIRECT_HEADER()
 */

/**
 * @def Z_ARCH_ISR_DIRECT_FOOTER(swap)
 *
 * @see ISR_DIRECT_FOOTER()
 */

/**
 * @def Z_ARCH_ISR_DIRECT_DECLARE(name)
 *
 * @see ISR_DIRECT_DECLARE()
 */

/**
 * @def Z_ARCH_EXCEPT(reason_p)
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
typedef void (*irq_offload_routine_t)(void *parameter);

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
void z_arch_irq_offload(irq_offload_routine_t routine, void *parameter);
#endif /* CONFIG_IRQ_OFFLOAD */

/** @} */


/**
 * @addtogroup arch-userspace
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
static inline u32_t z_arch_syscall_invoke0(u32_t call_id);

/**
 * Invoke a system call with 1 argument.
 *
 * @see z_arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static inline u32_t z_arch_syscall_invoke1(u32_t arg1, u32_t call_id);

/**
 * Invoke a system call with 2 arguments.
 *
 * @see z_arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param arg2 Second argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static inline u32_t z_arch_syscall_invoke2(u32_t arg1, u32_t arg2,
					   u32_t call_id);

/**
 * Invoke a system call with 3 arguments.
 *
 * @see z_arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param arg2 Second argument to the system call.
 * @param arg3 Third argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static inline u32_t z_arch_syscall_invoke3(u32_t arg1, u32_t arg2, u32_t arg3,
					   u32_t call_id);

/**
 * Invoke a system call with 4 arguments.
 *
 * @see z_arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param arg2 Second argument to the system call.
 * @param arg3 Third argument to the system call.
 * @param arg4 Fourth argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static inline u32_t z_arch_syscall_invoke4(u32_t arg1, u32_t arg2, u32_t arg3,
					   u32_t arg4, u32_t call_id);

/**
 * Invoke a system call with 5 arguments.
 *
 * @see z_arch_syscall_invoke0()
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
static inline u32_t z_arch_syscall_invoke5(u32_t arg1, u32_t arg2, u32_t arg3,
					   u32_t arg4, u32_t arg5,
					   u32_t call_id);

/**
 * Invoke a system call with 6 arguments.
 *
 * @see z_arch_syscall_invoke0()
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
static inline u32_t z_arch_syscall_invoke6(u32_t arg1, u32_t arg2, u32_t arg3,
					   u32_t arg4, u32_t arg5, u32_t arg6,
					   u32_t call_id);

/**
 * Indicate whether we are currently running in user mode
 *
 * @return true if the CPU is currently running with user permissions
 */
static inline bool z_arch_is_user_context(void);
#endif /* CONFIG_USERSPACE */

/** @} */

#ifdef __cplusplus
}
#endif
#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE?SYS_ARCH_INLINES_H_ */
