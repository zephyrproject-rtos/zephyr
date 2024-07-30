/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2021 Lexmark International, Inc.
 * Copyright (c) 2023 Arm Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief New thread creation for ARM Cortex-M
 *
 * Core thread related primitives for the ARM Cortex-M
 * processor architecture.
 */

#include <zephyr/kernel.h>
#include <zephyr/llext/symbol.h>
#include <ksched.h>
#include <zephyr/sys/barrier.h>
#include <stdbool.h>
#include <cmsis_core.h>

#if (MPU_GUARD_ALIGN_AND_SIZE_FLOAT > MPU_GUARD_ALIGN_AND_SIZE)
#define FP_GUARD_EXTRA_SIZE	(MPU_GUARD_ALIGN_AND_SIZE_FLOAT - \
				 MPU_GUARD_ALIGN_AND_SIZE)
#else
#define FP_GUARD_EXTRA_SIZE	0
#endif

#ifndef EXC_RETURN_FTYPE
/* bit [4] allocate stack for floating-point context: 0=done 1=skipped  */
#define EXC_RETURN_FTYPE           (0x00000010UL)
#endif

/* Default last octet of EXC_RETURN, for threads that have not run yet.
 * The full EXC_RETURN value will be e.g. 0xFFFFFFBC.
 */
#if defined(CONFIG_ARM_NONSECURE_FIRMWARE)
#define DEFAULT_EXC_RETURN 0xBC;
#else
#define DEFAULT_EXC_RETURN 0xFD;
#endif

#if !defined(CONFIG_MULTITHREADING)
K_THREAD_STACK_DECLARE(z_main_stack, CONFIG_MAIN_STACK_SIZE);
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

	/* force ARM mode by clearing LSB of address */
	iframe->pc &= 0xfffffffe;
	iframe->a1 = (uint32_t)entry;
	iframe->a2 = (uint32_t)p1;
	iframe->a3 = (uint32_t)p2;
	iframe->a4 = (uint32_t)p3;

	iframe->xpsr =
		0x01000000UL; /* clear all, thumb bit is 1, even if RO */

	thread->callee_saved.psp = (uint32_t)iframe;
	thread->arch.basepri = 0;

#if defined(CONFIG_ARM_STORE_EXC_RETURN) || defined(CONFIG_USERSPACE)
	thread->arch.mode = 0;
#if defined(CONFIG_ARM_STORE_EXC_RETURN)
	thread->arch.mode_exc_return = DEFAULT_EXC_RETURN;
#endif
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
		((thread->arch.mode_exc_return & EXC_RETURN_FTYPE) == 0)) {
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


bool z_arm_thread_is_in_user_mode(void)
{
	uint32_t value;

	/* return mode information */
	value = __get_CONTROL();
	return (value & CONTROL_nPRIV_Msk) != 0;
}
EXPORT_SYMBOL(z_arm_thread_is_in_user_mode);
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

	if (thread == NULL) {
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
		if (z_arm_thread_is_in_user_mode() == false) {
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
		return (uint32_t)K_THREAD_STACK_BUFFER(z_main_stack);
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

int arch_float_enable(struct k_thread *thread, unsigned int options)
{
	/* This is not supported in Cortex-M */
	return -ENOTSUP;
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
#if defined(CONFIG_ARMV8_1_M_MAINLINE)
	/*
	 * For ARMv8.1-M with FPU, the FPSCR[18:16] LTPSIZE field must be set
	 * to 0b100 for "Tail predication not applied" as it's reset value
	 */
	__set_FPSCR(4 << FPU_FPDSCR_LTPSIZE_Pos);
#else
	__set_FPSCR(0);
#endif
#if defined(CONFIG_FPU_SHARING)
	/* In Sharing mode clearing FPSCR may set the CONTROL.FPCA flag. */
	__set_CONTROL(__get_CONTROL() & (~(CONTROL_FPCA_Msk)));
	barrier_isync_fence_full();
#endif /* CONFIG_FPU_SHARING */
#endif /* CONFIG_FPU */
}

void arch_switch_to_main_thread(struct k_thread *main_thread, char *stack_ptr,
				k_thread_entry_t _main)
{
	z_arm_prepare_switch_to_main();

	_current = main_thread;

#if defined(CONFIG_THREAD_LOCAL_STORAGE)
	/* On Cortex-M, TLS uses a global variable as pointer to
	 * the thread local storage area. So this needs to point
	 * to the main thread's TLS area before switching to any
	 * thread for the first time, as the pointer is only set
	 * during context switching.
	 */
	extern uintptr_t z_arm_tls_ptr;

	z_arm_tls_ptr = main_thread->tls;
#endif

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
#error "Built-in PSP limit checks not supported by the hardware."
#endif
#endif /* CONFIG_BUILTIN_STACK_GUARD */

	/*
	 * Set PSP to the highest address of the main stack
	 * before enabling interrupts and jumping to main.
	 *
	 * The compiler may store _main on the stack, but this
	 * location is relative to `PSP`.
	 * This assembly block ensures that _main is stored in
	 * a callee saved register before switching stack and continuing
	 * with the thread entry process.
	 *
	 * When calling arch_irq_unlock_outlined, LR is lost which is fine since
	 * we do not intend to return after calling z_thread_entry.
	 */
	__asm__ volatile (
	"mov   r4,  %0\n"	/* force _main to be stored in a register */
	"msr   PSP, %1\n"	/* __set_PSP(stack_ptr) */

	"mov   r0,  #0\n"	/* arch_irq_unlock(0) */
	"ldr   r3, =arch_irq_unlock_outlined\n"
	"blx   r3\n"

	"mov   r0, r4\n"	/* z_thread_entry(_main, NULL, NULL, NULL) */
	"mov   r1, #0\n"
	"mov   r2, #0\n"
	"mov   r3, #0\n"
	"ldr   r4, =z_thread_entry\n"
	"bx    r4\n"		/* We don’t intend to return, so there is no need to link. */
	: "+r" (_main)
	: "r" (stack_ptr)
	: "r0", "r1", "r2", "r3", "r4", "ip", "lr");

	CODE_UNREACHABLE;
}

__used void arch_irq_unlock_outlined(unsigned int key)
{
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	__enable_fault_irq(); /* alters FAULTMASK */
	__enable_irq(); /* alters PRIMASK */
#endif
	arch_irq_unlock(key);
}

__used unsigned int arch_irq_lock_outlined(void)
{
	return arch_irq_lock();
}

#if !defined(CONFIG_MULTITHREADING)

FUNC_NORETURN void z_arm_switch_to_main_no_multithreading(
	k_thread_entry_t main_entry, void *p1, void *p2, void *p3)
{
	z_arm_prepare_switch_to_main();

	/* Set PSP to the highest address of the main stack. */
	char *psp =	K_THREAD_STACK_BUFFER(z_main_stack) +
		K_THREAD_STACK_SIZEOF(z_main_stack);

#if defined(CONFIG_BUILTIN_STACK_GUARD)
	char *psplim = (K_THREAD_STACK_BUFFER(z_main_stack));
	/* Clear PSPLIM before setting it to guard the main stack area. */
	__set_PSPLIM(0);
#endif

	/* Store all required input in registers, to be accessible
	 * after stack pointer change. The function is not going
	 * to return, so callee-saved registers do not need to be
	 * stacked.
	 *
	 * The compiler may store _main on the stack, but this
	 * location is relative to `PSP`.
	 * This assembly block ensures that _main is stored in
	 * a callee saved register before switching stack and continuing
	 * with the thread entry process.
	 */

	__asm__ volatile (
#ifdef CONFIG_BUILTIN_STACK_GUARD
	"msr  PSPLIM, %[_psplim]\n" /* __set_PSPLIM(_psplim) */
#endif
	"msr  PSP, %[_psp]\n"       /* __set_PSP(psp) */
	"mov r0, #0\n"
	"ldr r1, =arch_irq_unlock_outlined\n"
	"blx r1\n"

	"mov r0, %[_p1]\n"
	"mov r1, %[_p2]\n"
	"mov r2, %[_p3]\n"
	"blx  %[_main_entry]\n"     /* main_entry(p1, p2, p3) */

	"ldr r0, =arch_irq_lock_outlined\n"
	"blx r0\n"
	"loop: b loop\n\t"    /* while (true); */
	:
	: [_p1]"r" (p1), [_p2]"r" (p2), [_p3]"r" (p3),
	  [_psp]"r" (psp), [_main_entry]"r" (main_entry)
#ifdef CONFIG_BUILTIN_STACK_GUARD
	, [_psplim]"r" (psplim)
#endif
	: "r0", "r1", "r2", "ip", "lr"
	);

	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}
#endif /* !CONFIG_MULTITHREADING */
