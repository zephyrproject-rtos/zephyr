/*
 * Copyright (c) 2021 BayLibre SAS
 * Written by: Nicolas Pitre
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_arch_interface.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/atomic.h>

/* to be found in fpu.S */
extern void z_arm64_fpu_save(struct z_arm64_fp_context *saved_fp_context);
extern void z_arm64_fpu_restore(struct z_arm64_fp_context *saved_fp_context);

#define FPU_DEBUG 0

#if FPU_DEBUG

/*
 * Debug traces have to be produced without printk() or any other functions
 * using a va_list as va_start() always copy the FPU registers that could be
 * used to pass float arguments, and that triggers an FPU access trap.
 */

#include <string.h>

static void DBG(char *msg, struct k_thread *th)
{
	char buf[80], *p;
	unsigned int v;

	strcpy(buf, "CPU# exc# ");
	buf[3] = '0' + arch_curr_cpu()->id;
	buf[8] = '0' + arch_exception_depth();
	strcat(buf, _current->name);
	strcat(buf, ": ");
	strcat(buf, msg);
	strcat(buf, " ");
	strcat(buf, th->name);


	v = *(unsigned char *)&th->arch.saved_fp_context;
	p = buf + strlen(buf);
	*p++ = ' ';
	*p++ = ((v >> 4) < 10) ? ((v >> 4) + '0') : ((v >> 4) - 10 + 'a');
	*p++ = ((v & 15) < 10) ? ((v & 15) + '0') : ((v & 15) - 10 + 'a');
	*p++ = '\n';
	*p = 0;

	k_str_out(buf, p - buf);
}

#else

static inline void DBG(char *msg, struct k_thread *t) { }

#endif /* FPU_DEBUG */

/*
 * Flush FPU content and disable access.
 * This is called locally and also from flush_fpu_ipi_handler().
 */
void arch_flush_local_fpu(void)
{
	__ASSERT(read_daif() & DAIF_IRQ_BIT, "must be called with IRQs disabled");

	struct k_thread *owner = atomic_ptr_get(&arch_curr_cpu()->arch.fpu_owner);

	if (owner != NULL) {
		uint64_t cpacr = read_cpacr_el1();

		/* turn on FPU access */
		write_cpacr_el1(cpacr | CPACR_EL1_FPEN_NOTRAP);
		barrier_isync_fence_full();

		/* save current owner's content */
		z_arm64_fpu_save(&owner->arch.saved_fp_context);
		/* make sure content made it to memory before releasing */
		barrier_dsync_fence_full();
		/* release ownership */
		atomic_ptr_clear(&arch_curr_cpu()->arch.fpu_owner);
		DBG("disable", owner);

		/* disable FPU access */
		write_cpacr_el1(cpacr & ~CPACR_EL1_FPEN_NOTRAP);
		barrier_isync_fence_full();
	}
}

#ifdef CONFIG_SMP
static void flush_owned_fpu(struct k_thread *thread)
{
	__ASSERT(read_daif() & DAIF_IRQ_BIT, "must be called with IRQs disabled");

	int i;

	/* search all CPUs for the owner we want */
	unsigned int num_cpus = arch_num_cpus();

	for (i = 0; i < num_cpus; i++) {
		if (atomic_ptr_get(&_kernel.cpus[i].arch.fpu_owner) != thread) {
			continue;
		}
		/* we found it live on CPU i */
		if (i == arch_curr_cpu()->id) {
			arch_flush_local_fpu();
		} else {
			/* the FPU context is live on another CPU */
			arch_flush_fpu_ipi(i);

			/*
			 * Wait for it only if this is about the thread
			 * currently running on this CPU. Otherwise the
			 * other CPU running some other thread could regain
			 * ownership the moment it is removed from it and
			 * we would be stuck here.
			 *
			 * Also, if this is for the thread running on this
			 * CPU, then we preemptively flush any live context
			 * on this CPU as well since we're likely to
			 * replace it, and this avoids a deadlock where
			 * two CPUs want to pull each other's FPU context.
			 */
			if (thread == _current) {
				arch_flush_local_fpu();
				while (atomic_ptr_get(&_kernel.cpus[i].arch.fpu_owner) == thread) {
					barrier_dsync_fence_full();
				}
			}
		}
		break;
	}
}
#endif

void z_arm64_fpu_enter_exc(void)
{
	__ASSERT(read_daif() & DAIF_IRQ_BIT, "must be called with IRQs disabled");

	/* always deny FPU access whenever an exception is entered */
	write_cpacr_el1(read_cpacr_el1() & ~CPACR_EL1_FPEN_NOTRAP);
	barrier_isync_fence_full();
}

/*
 * Simulate some FPU store instructions.
 *
 * In many cases, the FPU trap is triggered by va_start() that copies
 * the content of FP registers used for floating point argument passing
 * into the va_list object in case there were actual float arguments from
 * the caller. In practice this is almost never the case, especially if
 * FPU access is disabled and we're trapped while in exception context.
 * Rather than flushing the FPU context to its owner and enabling access
 * just to let the corresponding STR instructions execute, we simply
 * simulate them and leave the FPU access disabled. This also avoids the
 * need for disabling interrupts in syscalls and IRQ handlers as well.
 */
static bool simulate_str_q_insn(struct arch_esf *esf)
{
	/*
	 * Support only the "FP in exception" cases for now.
	 * We know there is no saved FPU context to check nor any
	 * userspace stack memory to validate in that case.
	 */
	if (arch_exception_depth() <= 1) {
		return false;
	}

	uint32_t *pc = (uint32_t *)esf->elr;
	/* The original (interrupted) sp is the top of the esf structure */
	uintptr_t sp = (uintptr_t)esf + sizeof(*esf);

	for (;;) {
		uint32_t insn = *pc;

		/*
		 * We're looking for STR (immediate, SIMD&FP) of the form:
		 *
		 *  STR Q<n>, [SP, #<pimm>]
		 *
		 * where 0 <= <n> <= 7 and <pimm> is a 12-bits multiple of 16.
		 */
		if ((insn & 0xffc003f8) != 0x3d8003e0) {
			break;
		}

		uint32_t pimm = (insn >> 10) & 0xfff;

		/* Zero the location as the above STR would have done */
		*(__int128 *)(sp + pimm * 16) = 0;

		/* move to the next instruction */
		pc++;
	}

	/* did we do something? */
	if (pc != (uint32_t *)esf->elr) {
		/* resume execution past the simulated instructions */
		esf->elr = (uintptr_t)pc;
		return true;
	}

	return false;
}

/*
 * Process the FPU trap.
 *
 * This usually means that FP regs belong to another thread. Save them
 * to that thread's save area and restore the current thread's content.
 *
 * We also get here when FP regs are used while in exception as FP access
 * is always disabled by default in that case. If so we save the FPU content
 * to the owning thread and simply enable FPU access. Exceptions should be
 * short and don't have persistent register contexts when they're done so
 * there is nothing to save/restore for that context... as long as we
 * don't get interrupted that is. To ensure that we mask interrupts to
 * the triggering exception context.
 */
void z_arm64_fpu_trap(struct arch_esf *esf)
{
	__ASSERT(read_daif() & DAIF_IRQ_BIT, "must be called with IRQs disabled");

	/* check if a quick simulation can do it */
	if (simulate_str_q_insn(esf)) {
		return;
	}

	/* turn on FPU access */
	write_cpacr_el1(read_cpacr_el1() | CPACR_EL1_FPEN_NOTRAP);
	barrier_isync_fence_full();

	/* save current owner's content  if any */
	struct k_thread *owner = atomic_ptr_get(&arch_curr_cpu()->arch.fpu_owner);

	if (owner) {
		z_arm64_fpu_save(&owner->arch.saved_fp_context);
		barrier_dsync_fence_full();
		atomic_ptr_clear(&arch_curr_cpu()->arch.fpu_owner);
		DBG("save", owner);
	}

	if (arch_exception_depth() > 1) {
		/*
		 * We were already in exception when the FPU access trap.
		 * We give it access and prevent any further IRQ recursion
		 * by disabling IRQs as we wouldn't be able to preserve the
		 * interrupted exception's FPU context.
		 */
		esf->spsr |= DAIF_IRQ_BIT;
		return;
	}

#ifdef CONFIG_SMP
	/*
	 * Make sure the FPU context we need isn't live on another CPU.
	 * The current CPU's FPU context is NULL at this point.
	 */
	flush_owned_fpu(_current);
#endif

	/* become new owner */
	atomic_ptr_set(&arch_curr_cpu()->arch.fpu_owner, _current);

	/* restore our content */
	z_arm64_fpu_restore(&_current->arch.saved_fp_context);
	DBG("restore", _current);
}

/*
 * Perform lazy FPU context switching by simply granting or denying
 * access to FP regs based on FPU ownership before leaving the last
 * exception level in case of exceptions, or during a thread context
 * switch with the exception level of the new thread being 0.
 * If current thread doesn't own the FP regs then it will trap on its
 * first access and then the actual FPU context switching will occur.
 */
static void fpu_access_update(unsigned int exc_update_level)
{
	__ASSERT(read_daif() & DAIF_IRQ_BIT, "must be called with IRQs disabled");

	uint64_t cpacr = read_cpacr_el1();

	if (arch_exception_depth() == exc_update_level) {
		/* We're about to execute non-exception code */
		if (atomic_ptr_get(&arch_curr_cpu()->arch.fpu_owner) == _current) {
			/* turn on FPU access */
			write_cpacr_el1(cpacr | CPACR_EL1_FPEN_NOTRAP);
		} else {
			/* deny FPU access */
			write_cpacr_el1(cpacr & ~CPACR_EL1_FPEN_NOTRAP);
		}
	} else {
		/*
		 * Any new exception level should always trap on FPU
		 * access as we want to make sure IRQs are disabled before
		 * granting it access (see z_arm64_fpu_trap() documentation).
		 */
		write_cpacr_el1(cpacr & ~CPACR_EL1_FPEN_NOTRAP);
	}
	barrier_isync_fence_full();
}

/*
 * This is called on every exception exit except for z_arm64_fpu_trap().
 * In that case the exception level of interest is 1 (soon to be 0).
 */
void z_arm64_fpu_exit_exc(void)
{
	fpu_access_update(1);
}

/*
 * This is called from z_arm64_context_switch(). FPU access may be granted
 * only if exception level is 0. If we switch to a thread that is still in
 * some exception context then FPU access would be re-evaluated at exception
 * exit time via z_arm64_fpu_exit_exc().
 */
void z_arm64_fpu_thread_context_switch(void)
{
	fpu_access_update(0);
}

int arch_float_disable(struct k_thread *thread)
{
	if (thread != NULL) {
		unsigned int key = arch_irq_lock();

#ifdef CONFIG_SMP
		flush_owned_fpu(thread);
#else
		if (thread == atomic_ptr_get(&arch_curr_cpu()->arch.fpu_owner)) {
			arch_flush_local_fpu();
		}
#endif

		arch_irq_unlock(key);
	}

	return 0;
}

int arch_float_enable(struct k_thread *thread, unsigned int options)
{
	/* floats always gets enabled automatically at the moment */
	return 0;
}
