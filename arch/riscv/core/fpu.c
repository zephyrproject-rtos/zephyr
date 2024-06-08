/*
 * Copyright (c) 2023 BayLibre SAS
 * Written by: Nicolas Pitre
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_arch_interface.h>
#include <zephyr/sys/atomic.h>

/* to be found in fpu.S */
extern void z_riscv_fpu_save(struct z_riscv_fp_context *saved_fp_context);
extern void z_riscv_fpu_restore(struct z_riscv_fp_context *saved_fp_context);

#define FPU_DEBUG 0

#if FPU_DEBUG

/*
 * Debug traces have to be produced without printk() or any other functions
 * using a va_list as va_start() may copy the FPU registers that could be
 * used to pass float arguments, and that would trigger an FPU access trap.
 * Note: Apparently gcc doesn't use float regs with variadic functions on
 * RISC-V even if -mabi is used with f or d so this precaution might be
 * unnecessary. But better be safe than sorry especially for debugging code.
 */

#include <string.h>

static void DBG(char *msg, struct k_thread *th)
{
	char buf[80], *p;
	unsigned int v;

	strcpy(buf, "CPU# exc# ");
	buf[3] = '0' + _current_cpu->id;
	buf[8] = '0' + _current->arch.exception_depth;
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

static void z_riscv_fpu_disable(void)
{
	unsigned long status = csr_read(mstatus);

	__ASSERT((status & MSTATUS_IEN) == 0, "must be called with IRQs disabled");

	if ((status & MSTATUS_FS) != 0) {
		csr_clear(mstatus, MSTATUS_FS);

		/* remember its clean/dirty state */
		_current_cpu->arch.fpu_state = (status & MSTATUS_FS);
	}
}

static void z_riscv_fpu_load(void)
{
	__ASSERT((csr_read(mstatus) & MSTATUS_IEN) == 0,
		 "must be called with IRQs disabled");
	__ASSERT((csr_read(mstatus) & MSTATUS_FS) == 0,
		 "must be called with FPU access disabled");

	/* become new owner */
	atomic_ptr_set(&_current_cpu->arch.fpu_owner, _current);

	/* restore our content */
	csr_set(mstatus, MSTATUS_FS_INIT);
	z_riscv_fpu_restore(&_current->arch.saved_fp_context);
	DBG("restore", _current);
}

/*
 * Flush FPU content and clear ownership. If the saved FPU state is "clean"
 * then we know the in-memory copy is up to date and skip the FPU content
 * transfer. The saved FPU state is updated upon disabling FPU access so
 * we require that this be called only when the FPU is disabled.
 *
 * This is called locally and also from flush_fpu_ipi_handler().
 */
void arch_flush_local_fpu(void)
{
	__ASSERT((csr_read(mstatus) & MSTATUS_IEN) == 0,
		 "must be called with IRQs disabled");
	__ASSERT((csr_read(mstatus) & MSTATUS_FS) == 0,
		 "must be called with FPU access disabled");

	struct k_thread *owner = atomic_ptr_get(&_current_cpu->arch.fpu_owner);

	if (owner != NULL) {
		bool dirty = (_current_cpu->arch.fpu_state == MSTATUS_FS_DIRTY);

		if (dirty) {
			/* turn on FPU access */
			csr_set(mstatus, MSTATUS_FS_CLEAN);
			/* save current owner's content */
			z_riscv_fpu_save(&owner->arch.saved_fp_context);
		}

		/* dirty means active use */
		owner->arch.fpu_recently_used = dirty;

		/* disable FPU access */
		csr_clear(mstatus, MSTATUS_FS);

		/* release ownership */
		atomic_ptr_clear(&_current_cpu->arch.fpu_owner);
		DBG("disable", owner);
	}
}

#ifdef CONFIG_SMP
static void flush_owned_fpu(struct k_thread *thread)
{
	__ASSERT((csr_read(mstatus) & MSTATUS_IEN) == 0,
		 "must be called with IRQs disabled");

	int i;
	atomic_ptr_val_t owner;

	/* search all CPUs for the owner we want */
	unsigned int num_cpus = arch_num_cpus();

	for (i = 0; i < num_cpus; i++) {
		owner = atomic_ptr_get(&_kernel.cpus[i].arch.fpu_owner);
		if (owner != thread) {
			continue;
		}
		/* we found it live on CPU i */
		if (i == _current_cpu->id) {
			z_riscv_fpu_disable();
			arch_flush_local_fpu();
			break;
		}
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
			z_riscv_fpu_disable();
			arch_flush_local_fpu();
			do {
				arch_nop();
				owner = atomic_ptr_get(&_kernel.cpus[i].arch.fpu_owner);
			} while (owner == thread);
		}
		break;
	}
}
#endif

void z_riscv_fpu_enter_exc(void)
{
	/* always deny FPU access whenever an exception is entered */
	z_riscv_fpu_disable();
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
 *
 * Note that the exception depth count was not incremented before this call
 * as no further exceptions are expected before returning to normal mode.
 */
void z_riscv_fpu_trap(struct arch_esf *esf)
{
	__ASSERT((esf->mstatus & MSTATUS_FS) == 0 &&
		 (csr_read(mstatus) & MSTATUS_FS) == 0,
		 "called despite FPU being accessible");

	/* save current owner's content  if any */
	arch_flush_local_fpu();

	if (_current->arch.exception_depth > 0) {
		/*
		 * We were already in exception when the FPU access trapped.
		 * We give it access and prevent any further IRQ recursion
		 * by disabling IRQs as we wouldn't be able to preserve the
		 * interrupted exception's FPU context.
		 */
		esf->mstatus &= ~MSTATUS_MPIE_EN;

		/* make it accessible to the returning context */
		esf->mstatus |= MSTATUS_FS_INIT;

		return;
	}

#ifdef CONFIG_SMP
	/*
	 * Make sure the FPU context we need isn't live on another CPU.
	 * The current CPU's FPU context is NULL at this point.
	 */
	flush_owned_fpu(_current);
#endif

	/* make it accessible and clean to the returning context */
	esf->mstatus |= MSTATUS_FS_CLEAN;

	/* and load it with corresponding content */
	z_riscv_fpu_load();
}

/*
 * Perform lazy FPU context switching by simply granting or denying
 * access to FP regs based on FPU ownership before leaving the last
 * exception level in case of exceptions, or during a thread context
 * switch with the exception level of the new thread being 0.
 * If current thread doesn't own the FP regs then it will trap on its
 * first access and then the actual FPU context switching will occur.
 */
static bool fpu_access_allowed(unsigned int exc_update_level)
{
	__ASSERT((csr_read(mstatus) & MSTATUS_IEN) == 0,
		 "must be called with IRQs disabled");

	if (_current->arch.exception_depth == exc_update_level) {
		/* We're about to execute non-exception code */
		if (_current_cpu->arch.fpu_owner == _current) {
			/* everything is already in place */
			return true;
		}
		if (_current->arch.fpu_recently_used) {
			/*
			 * Before this thread was context-switched out,
			 * it made active use of the FPU, but someone else
			 * took it away in the mean time. Let's preemptively
			 * claim it back to avoid the likely exception trap
			 * to come otherwise.
			 */
			z_riscv_fpu_disable();
			arch_flush_local_fpu();
#ifdef CONFIG_SMP
			flush_owned_fpu(_current);
#endif
			z_riscv_fpu_load();
			_current_cpu->arch.fpu_state = MSTATUS_FS_CLEAN;
			return true;
		}
		return false;
	}
	/*
	 * Any new exception level should always trap on FPU
	 * access as we want to make sure IRQs are disabled before
	 * granting it access (see z_riscv_fpu_trap() documentation).
	 */
	return false;
}

/*
 * This is called on every exception exit except for z_riscv_fpu_trap().
 * In that case the exception level of interest is 1 (soon to be 0).
 */
void z_riscv_fpu_exit_exc(struct arch_esf *esf)
{
	if (fpu_access_allowed(1)) {
		esf->mstatus &= ~MSTATUS_FS;
		esf->mstatus |= _current_cpu->arch.fpu_state;
	} else {
		esf->mstatus &= ~MSTATUS_FS;
	}
}

/*
 * This is called from z_riscv_context_switch(). FPU access may be granted
 * only if exception level is 0. If we switch to a thread that is still in
 * some exception context then FPU access would be re-evaluated at exception
 * exit time via z_riscv_fpu_exit_exc().
 */
void z_riscv_fpu_thread_context_switch(void)
{
	if (fpu_access_allowed(0)) {
		csr_clear(mstatus, MSTATUS_FS);
		csr_set(mstatus, _current_cpu->arch.fpu_state);
	} else {
		z_riscv_fpu_disable();
	}
}

int arch_float_disable(struct k_thread *thread)
{
	if (thread != NULL) {
		unsigned int key = arch_irq_lock();

#ifdef CONFIG_SMP
		flush_owned_fpu(thread);
#else
		if (thread == _current_cpu->arch.fpu_owner) {
			z_riscv_fpu_disable();
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
