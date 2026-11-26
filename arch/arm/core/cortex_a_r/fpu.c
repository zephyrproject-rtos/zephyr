/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Lazy FPU context switching for Cortex-A/R, as done on ARM64.  The FPU
 * is disabled (FPEXC.EN clear) on exception entry, and on context
 * switches unless the incoming thread owns it and resumes in thread
 * code.  The first VFP instruction then takes an undefined instruction
 * exception, where z_arm_fpu_trap() saves the context of the thread that
 * owns the FPU, if any, and loads the context of the current thread.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <kernel_arch_interface.h>
#include <kthread.h>
#include <string.h>

static ALWAYS_INLINE void fpu_save(struct z_arm_fp_context *ctx)
{
	uint64_t *d = ctx->d;

	__asm__ volatile("vstmia %0!, {d0-d15}" : "+r"(d) : : "memory");
#ifdef CONFIG_VFP_FEATURE_REGS_S64_D32
	__asm__ volatile("vstmia %0, {d16-d31}" : : "r"(d) : "memory");
#endif
	ctx->fpscr = __get_FPSCR();
}

static ALWAYS_INLINE void fpu_restore(struct z_arm_fp_context *ctx)
{
	uint64_t *d = ctx->d;

	__asm__ volatile("vldmia %0!, {d0-d15}" : "+r"(d) : : "memory");
#ifdef CONFIG_VFP_FEATURE_REGS_S64_D32
	__asm__ volatile("vldmia %0, {d16-d31}" : : "r"(d) : "memory");
#endif
	__set_FPSCR(ctx->fpscr);
}

static ALWAYS_INLINE void fpu_access(bool enable)
{
	__set_FPEXC(enable ? FPEXC_EN : 0);
	barrier_isync_fence_full();
}

/* Save the FPU owner's context and release the FPU.  IRQs are disabled. */
void arch_flush_local_fpu(void)
{
	struct k_thread *owner = _current_cpu->arch.fpu_owner;

	if (owner != NULL) {
		fpu_access(true);
		fpu_save(&owner->arch.saved_fp_context);
		barrier_dsync_fence_full();
		_current_cpu->arch.fpu_owner = NULL;
		fpu_access(false);
	}
}

/*
 * Called by the undefined instruction handler, before anything else,
 * with IRQs disabled.  Returns false if the FPU was already enabled,
 * in which case the instruction is really undefined.  The FPU is then
 * disabled, so that VFP code in the fault handling traps and saves the
 * context of the FPU owner instead of overwriting it.  Otherwise the
 * faulting instruction is retried with the FPU enabled.
 */
bool z_arm_fpu_trap(struct arch_esf *esf)
{
	if ((__get_FPEXC() & FPEXC_EN) != 0) {
		fpu_access(false);
		return false;
	}

	struct k_thread *owner = _current_cpu->arch.fpu_owner;

	fpu_access(true);

	if (owner != NULL) {
		fpu_save(&owner->arch.saved_fp_context);
		barrier_dsync_fence_full();
		_current_cpu->arch.fpu_owner = NULL;
	}

	if (_current_cpu->arch.exc_depth > 1) {
		/*
		 * The FPU was used in an exception handler.  Its context
		 * belongs to no thread and can't be preserved across a
		 * nested exception, so keep IRQs disabled until that
		 * handler returns.  The FPU is disabled again on its exit.
		 * A handler that enables IRQs again, with irq_unlock() of
		 * a key taken before its first VFP instruction for
		 * example, may be nested by a handler that uses the FPU
		 * and overwrites its FP registers.
		 */
		esf->basic.xpsr |= I_BIT;
		return true;
	}

	/*
	 * halt_thread() releases the FPU of a dying thread with
	 * arch_coprocessors_disable(), and nothing releases it after that.
	 * The FPU of a dead _current stays without owner, enabled until the
	 * switch away from _current. IRQs are disabled until then.
	 */
	if (!z_is_thread_dead(_current)) {
		_current_cpu->arch.fpu_owner = _current;
	}

	struct z_arm_fp_context *ctx = &_current->arch.saved_fp_context;

	if (_current->arch.fp_context_empty) {
		memset(ctx, 0, sizeof(*ctx));
		_current->arch.fp_context_empty = false;
	}
	fpu_restore(ctx);

	return true;
}

/*
 * Called when switching to _current.  The FPU is enabled only if
 * _current owns it and is about to run thread code.  A thread resumed
 * in an exception gets the FPU enabled on that exception's exit.
 */
void z_arm_fpu_thread_context_switch(struct k_thread *old)
{
	if (IS_ENABLED(CONFIG_SMP) && (_current_cpu->arch.fpu_owner == old)) {
		/* old may next run on another CPU: don't leave its context here */
		arch_flush_local_fpu();
	}

	fpu_access((_current_cpu->arch.exc_depth == 0) &&
		   (_current_cpu->arch.fpu_owner == _current));
}

/* Release the FPU if thread owns it.  Also used when thread is aborted. */
static void fpu_release(struct k_thread *thread)
{
	/*
	 * Only a thread takes the FPU for itself, and on SMP a thread that is
	 * switched out owns no FPU. So thread doesn't own the FPU of this CPU
	 * if it doesn't now, and that needs no IRQ lock to find out.
	 */
	if (arch_curr_cpu()->arch.fpu_owner != thread) {
		return;
	}

	unsigned int key = arch_irq_lock();

	if (_current_cpu->arch.fpu_owner == thread) {
		arch_flush_local_fpu();
	}

	arch_irq_unlock(key);
}

int arch_float_disable(struct k_thread *thread)
{
	if ((thread != _current) || arch_is_in_isr()) {
		return -EINVAL;
	}

	thread->base.user_options &= ~K_FP_REGS;
	fpu_release(thread);

	return 0;
}

int arch_float_enable(struct k_thread *thread, unsigned int options)
{
	ARG_UNUSED(thread);
	ARG_UNUSED(options);

	/* Any thread may use the FPU, see z_arm_fpu_trap() */
	return 0;
}

int arch_coprocessors_disable(struct k_thread *thread)
{
	fpu_release(thread);

	return 0;
}

#if defined(CONFIG_USERSPACE)
/*
 * Called when _current enters user mode: its FP registers may hold values
 * from kernel code. Drop its FP context without saving it, so that its
 * first FPU use in user mode starts with zeroed registers.
 */
void z_arm_fpu_user_mode_enter(void)
{
	unsigned int key = arch_irq_lock();

	if (_current_cpu->arch.fpu_owner == _current) {
		_current_cpu->arch.fpu_owner = NULL;
		fpu_access(false);
	}
	_current->arch.fp_context_empty = true;

	arch_irq_unlock(key);
}
#endif
