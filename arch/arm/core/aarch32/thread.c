/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief New thread creation for ARM Cortex-M and Cortex-R
 *
 * Core thread related primitives for the ARM Cortex-M and Cortex-R
 * processor architecture.
 */

#include <kernel.h>
#include <ksched.h>
#include <wait_q.h>

#if (MPU_GUARD_ALIGN_AND_SIZE_FLOAT > MPU_GUARD_ALIGN_AND_SIZE)
#define FP_GUARD_EXTRA_SIZE	(MPU_GUARD_ALIGN_AND_SIZE_FLOAT - \
				 MPU_GUARD_ALIGN_AND_SIZE)
#else
#define FP_GUARD_EXTRA_SIZE	0
#endif

#if !defined(CONFIG_MULTITHREADING) && defined(CONFIG_CPU_CORTEX_M)
extern K_THREAD_STACK_DEFINE(z_main_stack, CONFIG_MAIN_STACK_SIZE);
#endif

/* An initial context, to be "restored" by z_arm_pendsv(), is put at the other
 * end of the stack, and thus reusable by the stack when not needed anymore.
 *
 * The initial context is an exception stack frame (ESF) since exiting the
 * PendSV exception will want to pop an ESF. Interestingly, even if the lsb of
 * an instruction address to jump to must always be set since the CPU always
 * runs in thumb mode, the ESF expects the real address of the instruction,
 * with the lsb *not* set (instructions are always aligned on 16 bit
 * halfwords).  Since the compiler automatically sets the lsb of function
 * addresses, we have to unset it manually before storing it in the 'pc' field
 * of the ESF.
 */
void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	struct __basic_sf *iframe;

#ifdef CONFIG_MPU_STACK_GUARD
#if defined(CONFIG_USERSPACE)
	if (z_stack_is_user_capable(stack)) {
		/* Guard area is carved-out of the buffer instead of reserved
		 * for stacks that can host user threads
		 */
		thread->stack_info.start += MPU_GUARD_ALIGN_AND_SIZE;
		thread->stack_info.size -= MPU_GUARD_ALIGN_AND_SIZE;
	}
#endif /* CONFIG_USERSPACE */
#if FP_GUARD_EXTRA_SIZE > 0
	if ((thread->base.user_options & K_FP_REGS) != 0) {
		/* Larger guard needed due to lazy stacking of FP regs may
		 * overshoot the guard area without writing anything. We
		 * carve it out of the stack buffer as-needed instead of
		 * unconditionally reserving it.
		 */
		thread->stack_info.start += FP_GUARD_EXTRA_SIZE;
		thread->stack_info.size -= FP_GUARD_EXTRA_SIZE;
	}
#endif /* FP_GUARD_EXTRA_SIZE */
#endif /* CONFIG_MPU_STACK_GUARD */

	iframe = Z_STACK_PTR_TO_FRAME(struct __basic_sf, stack_ptr);
#if defined(CONFIG_USERSPACE)
	if ((thread->base.user_options & K_USER) != 0) {
		iframe->pc = (uint32_t)arch_user_mode_enter;
	} else {
		iframe->pc = (uint32_t)z_thread_entry;
	}
#else
	iframe->pc = (uint32_t)z_thread_entry;
#endif

#if defined(CONFIG_CPU_CORTEX_M)
	/* force ARM mode by clearing LSB of address */
	iframe->pc &= 0xfffffffe;
#endif
	iframe->a1 = (uint32_t)entry;
	iframe->a2 = (uint32_t)p1;
	iframe->a3 = (uint32_t)p2;
	iframe->a4 = (uint32_t)p3;

#if defined(CONFIG_CPU_CORTEX_M)
	iframe->xpsr =
		0x01000000UL; /* clear all, thumb bit is 1, even if RO */
#else
	iframe->xpsr = A_BIT | MODE_SYS;
#if defined(CONFIG_COMPILER_ISA_THUMB2)
	iframe->xpsr |= T_BIT;
#endif /* CONFIG_COMPILER_ISA_THUMB2 */
#endif /* CONFIG_CPU_CORTEX_M */

	thread->callee_saved.psp = (uint32_t)iframe;
	thread->arch.basepri = 0;

#if defined(CONFIG_USERSPACE) || defined(CONFIG_FPU_SHARING)
	thread->arch.mode = 0;
#if FP_GUARD_EXTRA_SIZE > 0
	if ((thread->base.user_options & K_FP_REGS) != 0) {
		thread->arch.mode |= Z_ARM_MODE_MPU_GUARD_FLOAT_Msk;
	}
#endif
#if defined(CONFIG_USERSPACE)
	thread->arch.priv_stack_start = 0;
#endif
#endif
	/*
	 * initial values in all other registers/thread entries are
	 * irrelevant.
	 */
}

#if defined(CONFIG_MPU_STACK_GUARD) && defined(CONFIG_FPU) \
	&& defined(CONFIG_FPU_SHARING)

static inline void z_arm_thread_stack_info_adjust(struct k_thread *thread,
	bool use_large_guard)
{
	if (use_large_guard) {
		/* Switch to use a large MPU guard if not already. */
		if ((thread->arch.mode &
			Z_ARM_MODE_MPU_GUARD_FLOAT_Msk) == 0) {
			/* Default guard size is used. Update required. */
			thread->arch.mode |= Z_ARM_MODE_MPU_GUARD_FLOAT_Msk;
#if defined(CONFIG_USERSPACE)
			if (thread->arch.priv_stack_start) {
				/* User thread */
				thread->arch.priv_stack_start +=
					FP_GUARD_EXTRA_SIZE;
			} else
#endif /* CONFIG_USERSPACE */
			{
				/* Privileged thread */
				thread->stack_info.start +=
					FP_GUARD_EXTRA_SIZE;
				thread->stack_info.size -=
					FP_GUARD_EXTRA_SIZE;
			}
		}
	} else {
		/* Switch to use the default MPU guard size if not already. */
		if ((thread->arch.mode &
			Z_ARM_MODE_MPU_GUARD_FLOAT_Msk) != 0) {
			/* Large guard size is used. Update required. */
			thread->arch.mode &= ~Z_ARM_MODE_MPU_GUARD_FLOAT_Msk;
#if defined(CONFIG_USERSPACE)
			if (thread->arch.priv_stack_start) {
				/* User thread */
				thread->arch.priv_stack_start -=
					FP_GUARD_EXTRA_SIZE;
			} else
#endif /* CONFIG_USERSPACE */
			{
				/* Privileged thread */
				thread->stack_info.start -=
					FP_GUARD_EXTRA_SIZE;
				thread->stack_info.size +=
					FP_GUARD_EXTRA_SIZE;
			}
		}
	}
}

/*
 * Adjust the MPU stack guard size together with the FPU
 * policy and the stack_info values for the thread that is
 * being switched in.
 */
uint32_t z_arm_mpu_stack_guard_and_fpu_adjust(struct k_thread *thread)
{
	if (((thread->base.user_options & K_FP_REGS) != 0) ||
		((thread->arch.mode & CONTROL_FPCA_Msk) != 0)) {
		/* The thread has been pre-tagged (at creation or later) with
		 * K_FP_REGS, i.e. it is expected to be using the FPU registers
		 * (if not already). Activate lazy stacking and program a large
		 * MPU guard to safely detect privilege thread stack overflows.
		 *
		 * OR
		 * The thread is not pre-tagged with K_FP_REGS, but it has
		 * generated an FP context. Activate lazy stacking and
		 * program a large MPU guard to detect privilege thread
		 * stack overflows.
		 */
		FPU->FPCCR |= FPU_FPCCR_LSPEN_Msk;

		z_arm_thread_stack_info_adjust(thread, true);

		/* Tag the thread with K_FP_REGS */
		thread->base.user_options |= K_FP_REGS;

		return MPU_GUARD_ALIGN_AND_SIZE_FLOAT;
	}

	/* Thread is not pre-tagged with K_FP_REGS, and it has
	 * not been using the FPU. Since there is no active FPU
	 * context, de-activate lazy stacking and program the
	 * default MPU guard size.
	 */
	FPU->FPCCR &= (~FPU_FPCCR_LSPEN_Msk);

	z_arm_thread_stack_info_adjust(thread, false);

	return MPU_GUARD_ALIGN_AND_SIZE;
}
#endif

#ifdef CONFIG_USERSPACE
FUNC_NORETURN void arch_user_mode_enter(k_thread_entry_t user_entry,
					void *p1, void *p2, void *p3)
{

	/* Set up privileged stack before entering user mode */
	_current->arch.priv_stack_start =
		(uint32_t)z_priv_stack_find(_current->stack_obj);
#if defined(CONFIG_MPU_STACK_GUARD)
#if defined(CONFIG_THREAD_STACK_INFO)
	/* We're dropping to user mode which means the guard area is no
	 * longer used here, it instead is moved to the privilege stack
	 * to catch stack overflows there. Un-do the calculations done
	 * which accounted for memory borrowed from the thread stack.
	 */
#if FP_GUARD_EXTRA_SIZE > 0
	if ((_current->arch.mode & Z_ARM_MODE_MPU_GUARD_FLOAT_Msk) != 0) {
		_current->stack_info.start -= FP_GUARD_EXTRA_SIZE;
		_current->stack_info.size += FP_GUARD_EXTRA_SIZE;
	}
#endif /* FP_GUARD_EXTRA_SIZE */
	_current->stack_info.start -= MPU_GUARD_ALIGN_AND_SIZE;
	_current->stack_info.size += MPU_GUARD_ALIGN_AND_SIZE;
#endif /* CONFIG_THREAD_STACK_INFO */

	/* Stack guard area reserved at the bottom of the thread's
	 * privileged stack. Adjust the available (writable) stack
	 * buffer area accordingly.
	 */
#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	_current->arch.priv_stack_start +=
		((_current->arch.mode & Z_ARM_MODE_MPU_GUARD_FLOAT_Msk) != 0) ?
		MPU_GUARD_ALIGN_AND_SIZE_FLOAT : MPU_GUARD_ALIGN_AND_SIZE;
#else
	_current->arch.priv_stack_start += MPU_GUARD_ALIGN_AND_SIZE;
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */
#endif /* CONFIG_MPU_STACK_GUARD */

	z_arm_userspace_enter(user_entry, p1, p2, p3,
			     (uint32_t)_current->stack_info.start,
			     _current->stack_info.size -
			     _current->stack_info.delta);
	CODE_UNREACHABLE;
}

#endif

#if defined(CONFIG_BUILTIN_STACK_GUARD)
/*
 * @brief Configure ARM built-in stack guard
 *
 * This function configures per thread stack guards by reprogramming
 * the built-in Process Stack Pointer Limit Register (PSPLIM).
 * The functionality is meant to be used during context switch.
 *
 * @param thread thread info data structure.
 */
void configure_builtin_stack_guard(struct k_thread *thread)
{
#if defined(CONFIG_USERSPACE)
	if ((thread->arch.mode & CONTROL_nPRIV_Msk) != 0) {
		/* Only configure stack limit for threads in privileged mode
		 * (i.e supervisor threads or user threads doing system call).
		 * User threads executing in user mode do not require a stack
		 * limit protection.
		 */
		__set_PSPLIM(0);
		return;
	}
	/* Only configure PSPLIM to guard the privileged stack area, if
	 * the thread is currently using it, otherwise guard the default
	 * thread stack. Note that the conditional check relies on the
	 * thread privileged stack being allocated in higher memory area
	 * than the default thread stack (ensured by design).
	 */
	uint32_t guard_start =
		((thread->arch.priv_stack_start) &&
			(__get_PSP() >= thread->arch.priv_stack_start)) ?
		(uint32_t)thread->arch.priv_stack_start :
		(uint32_t)thread->stack_obj;

	__ASSERT(thread->stack_info.start == ((uint32_t)thread->stack_obj),
		"stack_info.start does not point to the start of the"
		"thread allocated area.");
#else
	uint32_t guard_start = thread->stack_info.start;
#endif
#if defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
	__set_PSPLIM(guard_start);
#else
#error "Built-in PSP limit checks not supported by HW"
#endif
}
#endif /* CONFIG_BUILTIN_STACK_GUARD */

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)

#define IS_MPU_GUARD_VIOLATION(guard_start, guard_len, fault_addr, stack_ptr) \
	((fault_addr != -EINVAL) ? \
	((fault_addr >= guard_start) && \
	(fault_addr < (guard_start + guard_len)) && \
	(stack_ptr < (guard_start + guard_len))) \
	: \
	(stack_ptr < (guard_start + guard_len)))

/**
 * @brief Assess occurrence of current thread's stack corruption
 *
 * This function performs an assessment whether a memory fault (on a
 * given memory address) is the result of stack memory corruption of
 * the current thread.
 *
 * Thread stack corruption for supervisor threads or user threads in
 * privilege mode (when User Space is supported) is reported upon an
 * attempt to access the stack guard area (if MPU Stack Guard feature
 * is supported). Additionally the current PSP (process stack pointer)
 * must be pointing inside or below the guard area.
 *
 * Thread stack corruption for user threads in user mode is reported,
 * if the current PSP is pointing below the start of the current
 * thread's stack.
 *
 * Notes:
 * - we assume a fully descending stack,
 * - we assume a stacking error has occurred,
 * - the function shall be called when handling MemManage and Bus fault,
 *   and only if a Stacking error has been reported.
 *
 * If stack corruption is detected, the function returns the lowest
 * allowed address where the Stack Pointer can safely point to, to
 * prevent from errors when un-stacking the corrupted stack frame
 * upon exception return.
 *
 * @param fault_addr memory address on which memory access violation
 *                   has been reported. It can be invalid (-EINVAL),
 *                   if only Stacking error has been reported.
 * @param psp        current address the PSP points to
 *
 * @return The lowest allowed stack frame pointer, if error is a
 *         thread stack corruption, otherwise return 0.
 */
uint32_t z_check_thread_stack_fail(const uint32_t fault_addr, const uint32_t psp)
{
#if defined(CONFIG_MULTITHREADING)
	const struct k_thread *thread = _current;

	if (!thread) {
		return 0;
	}
#endif

#if (defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)) && \
	defined(CONFIG_MPU_STACK_GUARD)
	uint32_t guard_len =
		((_current->arch.mode & Z_ARM_MODE_MPU_GUARD_FLOAT_Msk) != 0) ?
		MPU_GUARD_ALIGN_AND_SIZE_FLOAT : MPU_GUARD_ALIGN_AND_SIZE;
#else
	/* If MPU_STACK_GUARD is not enabled, the guard length is
	 * effectively zero. Stack overflows may be detected only
	 * for user threads in nPRIV mode.
	 */
	uint32_t guard_len = MPU_GUARD_ALIGN_AND_SIZE;
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

#if defined(CONFIG_USERSPACE)
	if (thread->arch.priv_stack_start) {
		/* User thread */
		if ((__get_CONTROL() & CONTROL_nPRIV_Msk) == 0U) {
			/* User thread in privilege mode */
			if (IS_MPU_GUARD_VIOLATION(
				thread->arch.priv_stack_start - guard_len,
					guard_len,
				fault_addr, psp)) {
				/* Thread's privilege stack corruption */
				return thread->arch.priv_stack_start;
			}
		} else {
			if (psp < (uint32_t)thread->stack_obj) {
				/* Thread's user stack corruption */
				return (uint32_t)thread->stack_obj;
			}
		}
	} else {
		/* Supervisor thread */
		if (IS_MPU_GUARD_VIOLATION(thread->stack_info.start -
				guard_len,
				guard_len,
				fault_addr, psp)) {
			/* Supervisor thread stack corruption */
			return thread->stack_info.start;
		}
	}
#else /* CONFIG_USERSPACE */
#if defined(CONFIG_MULTITHREADING)
	if (IS_MPU_GUARD_VIOLATION(thread->stack_info.start - guard_len,
			guard_len,
			fault_addr, psp)) {
		/* Thread stack corruption */
		return thread->stack_info.start;
	}
#else
	if (IS_MPU_GUARD_VIOLATION((uint32_t)z_main_stack,
			guard_len,
			fault_addr, psp)) {
		/* Thread stack corruption */
		return (uint32_t)Z_THREAD_STACK_BUFFER(z_main_stack);
	}
#endif
#endif /* CONFIG_USERSPACE */

	return 0;
}
#endif /* CONFIG_MPU_STACK_GUARD || CONFIG_USERSPACE */

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
int arch_float_disable(struct k_thread *thread)
{
	if (thread != _current) {
		return -EINVAL;
	}

	if (arch_is_in_isr()) {
		return -EINVAL;
	}

	/* Disable all floating point capabilities for the thread */

	/* K_FP_REG flag is used in SWAP and stack check fail. Locking
	 * interrupts here prevents a possible context-switch or MPU
	 * fault to take an outdated thread user_options flag into
	 * account.
	 */
	int key = arch_irq_lock();

	thread->base.user_options &= ~K_FP_REGS;

	__set_CONTROL(__get_CONTROL() & (~CONTROL_FPCA_Msk));

	/* No need to add an ISB barrier after setting the CONTROL
	 * register; arch_irq_unlock() already adds one.
	 */

	arch_irq_unlock(key);

	return 0;
}
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

/* Internal function for Cortex-M initialization,
 * applicable to either case of running Zephyr
 * with or without multi-threading support.
 */
static void z_arm_prepare_switch_to_main(void)
{
#if defined(CONFIG_FPU)
	/* Initialize the Floating Point Status and Control Register when in
	 * Unshared FP Registers mode (In Shared FP Registers mode, FPSCR is
	 * initialized at thread creation for threads that make use of the FP).
	 */
	__set_FPSCR(0);
#if defined(CONFIG_FPU_SHARING)
	/* In Sharing mode clearing FPSCR may set the CONTROL.FPCA flag. */
	__set_CONTROL(__get_CONTROL() & (~(CONTROL_FPCA_Msk)));
	__ISB();
#endif /* CONFIG_FPU_SHARING */
#endif /* CONFIG_FPU */

#ifdef CONFIG_ARM_MPU
	/* Configure static memory map. This will program MPU regions,
	 * to set up access permissions for fixed memory sections, such
	 * as Application Memory or No-Cacheable SRAM area.
	 *
	 * This function is invoked once, upon system initialization.
	 */
	z_arm_configure_static_mpu_regions();
#endif
}

void arch_switch_to_main_thread(struct k_thread *main_thread, char *stack_ptr,
				k_thread_entry_t _main)
{
	z_arm_prepare_switch_to_main();

	_current = main_thread;
#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	z_thread_mark_switched_in();
#endif

	/* the ready queue cache already contains the main thread */

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
	/*
	 * If stack protection is enabled, make sure to set it
	 * before jumping to thread entry function
	 */
	z_arm_configure_dynamic_mpu_regions(main_thread);
#endif

#if defined(CONFIG_BUILTIN_STACK_GUARD)
	/* Set PSPLIM register for built-in stack guarding of main thread. */
#if defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
	__set_PSPLIM(main_thread->stack_info.start);
#else
#error "Built-in PSP limit checks not supported by HW"
#endif
#endif /* CONFIG_BUILTIN_STACK_GUARD */

	/*
	 * Set PSP to the highest address of the main stack
	 * before enabling interrupts and jumping to main.
	 */
	__asm__ volatile (
	"mov   r0,  %0\n\t"	/* Store _main in R0 */
#if defined(CONFIG_CPU_CORTEX_M)
	"msr   PSP, %1\n\t"	/* __set_PSP(stack_ptr) */
#endif

	"movs r1, #0\n\t"
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) \
			|| defined(CONFIG_ARMV7_R)
	"cpsie i\n\t"		/* __enable_irq() */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	"cpsie if\n\t"		/* __enable_irq(); __enable_fault_irq() */
	"msr   BASEPRI, r1\n\t"	/* __set_BASEPRI(0) */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
	"isb\n\t"
	"movs r2, #0\n\t"
	"movs r3, #0\n\t"
	"bl z_thread_entry\n\t"	/* z_thread_entry(_main, 0, 0, 0); */
	:
	: "r" (_main), "r" (stack_ptr)
	: "r0" /* not to be overwritten by msr PSP, %1 */
	);

	CODE_UNREACHABLE;
}

#if !defined(CONFIG_MULTITHREADING) && defined(CONFIG_CPU_CORTEX_M)

FUNC_NORETURN void z_arm_switch_to_main_no_multithreading(
	k_thread_entry_t main_entry, void *p1, void *p2, void *p3)
{
	z_arm_prepare_switch_to_main();

	/* Set PSP to the highest address of the main stack. */
	char *psp =	Z_THREAD_STACK_BUFFER(z_main_stack) +
		K_THREAD_STACK_SIZEOF(z_main_stack);

#if defined(CONFIG_BUILTIN_STACK_GUARD)
	char *psplim = (Z_THREAD_STACK_BUFFER(z_main_stack));
	/* Clear PSPLIM before setting it to guard the main stack area. */
	__set_PSPLIM(0);
#endif

	/* Store all required input in registers, to be accesible
	 * after stack pointer change. The function is not going
	 * to return, so callee-saved registers do not need to be
	 * stacked.
	 */
	register void *p1_inreg __asm__("r0") = p1;
	register void *p2_inreg __asm__("r1") = p2;
	register void *p3_inreg __asm__("r2") = p3;

	__asm__ volatile (
#ifdef CONFIG_BUILTIN_STACK_GUARD
	"msr  PSPLIM, %[_psplim]\n\t"
#endif
	"msr  PSP, %[_psp]\n\t"       /* __set_PSP(psp) */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	"cpsie i\n\t"         /* enable_irq() */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	"cpsie if\n\t"		/* __enable_irq(); __enable_fault_irq() */
	"mov r3, #0\n\t"
	"msr   BASEPRI, r3\n\t"	/* __set_BASEPRI(0) */
#endif
	"isb\n\t"
	"blx  %[_main_entry]\n\t"     /* main_entry(p1, p2, p3) */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	"cpsid i\n\t"         /* disable_irq() */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	"msr BASEPRI, %[basepri]\n\t"/* __set_BASEPRI(_EXC_IRQ_DEFAULT_PRIO) */
	"isb\n\t"
#endif
	"loop: b loop\n\t"    /* while (true); */
	:
	: "r" (p1_inreg), "r" (p2_inreg), "r" (p3_inreg),
	  [_psp]"r" (psp), [_main_entry]"r" (main_entry)
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	, [basepri] "r" (_EXC_IRQ_DEFAULT_PRIO)
#endif
#ifdef CONFIG_BUILTIN_STACK_GUARD
	, [_psplim]"r" (psplim)
#endif
	:
	);

	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}
#endif /* !CONFIG_MULTITHREADING && CONFIG_CPU_CORTEX_M */
