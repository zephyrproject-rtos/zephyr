/**
 * Copyright (c) 2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kswap.h>
#include <ksched.h>
#include <ipi.h>

#ifdef CONFIG_SCHED_IPI_SUPPORTED
static struct k_spinlock ipi_lock;
#endif

#ifdef CONFIG_TRACE_SCHED_IPI
extern void z_trace_sched_ipi(void);
#endif

/**
 * @brief Flag CPUs as needing an inter-processor reschedule interrupt.
 *
 * Records the given CPU set in the kernel's pending-IPI mask; the
 * interrupts are dispatched by signal_pending_ipi().
 *
 * @param ipi_mask Bitmask of CPUs to signal.
 *
 * @satisfies ZEP-SRS-34-11
 */
void flag_ipi(uint32_t ipi_mask)
{
#if defined(CONFIG_SCHED_IPI_SUPPORTED)
	if (arch_num_cpus() > 1) {
		atomic_or(&_kernel.pending_ipi, (atomic_val_t)ipi_mask);
	}
#endif /* CONFIG_SCHED_IPI_SUPPORTED */
}

/**
 * @brief Create a bitmask of CPUs that need an IPI.
 *
 * Without CONFIG_IPI_OPTIMIZE all other CPUs are signalled. With it,
 * only CPUs that actually need to reschedule in response to @a thread
 * becoming ready are selected -- skipping CPUs the thread cannot run on
 * (its CPU affinity mask) and CPUs whose current thread outranks it.
 *
 * Note: sched_spinlock is held.
 *
 * @param thread Thread that became ready.
 *
 * @return Bitmask of CPUs to signal.
 *
 * @satisfies ZEP-SRS-34-16
 * @satisfies ZEP-SRS-34-18
 */
atomic_val_t ipi_mask_create(struct k_thread *thread)
{
	if (!IS_ENABLED(CONFIG_IPI_OPTIMIZE)) {
		return (CONFIG_MP_MAX_NUM_CPUS > 1) ? IPI_ALL_CPUS_MASK : 0;
	}

	uint32_t  ipi_mask = 0;
	uint32_t  num_cpus = (uint32_t)arch_num_cpus();
	uint32_t  id = _current_cpu->id;
	struct k_thread *cpu_thread;
	bool   executable_on_cpu = true;

	for (uint32_t i = 0; i < num_cpus; i++) {
		if (id == i) {
			continue;
		}

		/*
		 * An IPI absolutely does not need to be sent if ...
		 * 1. the CPU is not active, or
		 * 2. <thread> can not execute on the target CPU
		 * ... and might not need to be sent if ...
		 * 3. the target CPU's active thread is not preemptible, or
		 * 4. the target CPU's active thread has a higher priority
		 *    (Items 3 & 4 may be overridden by a metaIRQ thread)
		 */

#if defined(CONFIG_SCHED_CPU_MASK)
		executable_on_cpu = ((thread->base.cpu_mask & BIT(i)) != 0);
#endif

		cpu_thread = _kernel.cpus[i].current;
		if ((cpu_thread != NULL) &&
		    (((z_sched_prio_cmp(cpu_thread, thread) < 0) &&
		      (thread_is_preemptible(cpu_thread))) ||
		     thread_is_metairq(thread)) && executable_on_cpu) {
			ipi_mask |= BIT(i);
		}
	}

	return (atomic_val_t)ipi_mask;
}

/**
 * @brief Dispatch any pending inter-processor reschedule interrupts.
 *
 * Sends the reschedule signal to the CPUs recorded by flag_ipi(): a
 * directed IPI to just those CPUs when the architecture supports it
 * (CONFIG_ARCH_HAS_DIRECTED_IPIS), otherwise a broadcast to all other
 * CPUs.
 *
 * @satisfies ZEP-SRS-34-11
 * @satisfies ZEP-SRS-34-17
 */
void signal_pending_ipi(void)
{
	/* Note: with directed IPIs, arch_sched_directed_ipi() skips the
	 * calling CPU, so if a CPU consumes its own pending bit the IPI
	 * is silently dropped. When rescheduling, callers must ensure
	 * that signal_pending_ipi() is invoked while the scheduler lock is
	 * still held. Holding the lock ensures the scheduling decision and
	 * IPI dispatch are atomic: either a concurrent flag_ipi() lands
	 * before the lock is acquired (and the CPU sees the new thread),
	 * or it lands after the lock is released (and the other CPU
	 * dispatches the IPI).
	 */

#if defined(CONFIG_SCHED_IPI_SUPPORTED)
	if (arch_num_cpus() > 1) {
		uint32_t  cpu_bitmap;

		cpu_bitmap = (uint32_t)atomic_clear(&_kernel.pending_ipi);
		if (cpu_bitmap != 0) {
#ifdef CONFIG_ARCH_HAS_DIRECTED_IPIS
			arch_sched_directed_ipi(cpu_bitmap);
#else
			arch_sched_broadcast_ipi();
#endif
		}
	}
#endif /* CONFIG_SCHED_IPI_SUPPORTED */
}

#ifdef CONFIG_SCHED_IPI_SUPPORTED
static struct k_ipi_work *first_ipi_work(sys_dlist_t *list)
{
	sys_dnode_t *work = sys_dlist_peek_head(list);
	unsigned int cpu_id = _current_cpu->id;

	return (work == NULL) ? NULL
			      : CONTAINER_OF(work, struct k_ipi_work, node[cpu_id]);
}

int k_ipi_work_add(struct k_ipi_work *work, uint32_t cpu_bitmask,
		   k_ipi_func_t func)
{
	__ASSERT(work != NULL, "");
	__ASSERT(func != NULL, "");

	k_spinlock_key_t key = k_spin_lock(&ipi_lock);

	/* Verify the IPI work item is not currently in use */

	if (k_event_wait_all(&work->event, work->bitmask,
			     false, K_NO_WAIT) != work->bitmask) {
		k_spin_unlock(&ipi_lock, key);
		return -EBUSY;
	}

	/*
	 * Add the IPI work item to the list(s)--but not for the current
	 * CPU as the architecture may not support sending an IPI to itself.
	 */

	unsigned int cpu_id = _current_cpu->id;

	cpu_bitmask &= (IPI_ALL_CPUS_MASK & ~BIT(cpu_id));

	k_event_clear(&work->event, IPI_ALL_CPUS_MASK);
	work->func = func;
	work->bitmask = cpu_bitmask;

	for (unsigned int id = 0; id < arch_num_cpus(); id++) {
		if ((cpu_bitmask & BIT(id)) != 0) {
			sys_dlist_append(&_kernel.cpus[id].ipi_workq, &work->node[id]);
		}
	}

	flag_ipi(cpu_bitmask);

	k_spin_unlock(&ipi_lock, key);

	return 0;
}

int k_ipi_work_wait(struct k_ipi_work *work, k_timeout_t timeout)
{
	uint32_t rv = k_event_wait_all(&work->event, work->bitmask,
				       false, timeout);

	return (rv == 0) ? -EAGAIN : 0;
}

void k_ipi_work_signal(void)
{
	signal_pending_ipi();
}

static void ipi_work_process(sys_dlist_t *list)
{
	unsigned int cpu_id = _current_cpu->id;

	k_spinlock_key_t key = k_spin_lock(&ipi_lock);

	for (struct k_ipi_work *work = first_ipi_work(list);
	     work != NULL; work = first_ipi_work(list)) {
		sys_dlist_remove(&work->node[cpu_id]);
		k_spin_unlock(&ipi_lock, key);

		work->func(work);

		key = k_spin_lock(&ipi_lock);
		k_event_post(&work->event, BIT(cpu_id));
	}

	k_spin_unlock(&ipi_lock, key);
}
#endif /* CONFIG_SCHED_IPI_SUPPORTED */

void z_sched_ipi(void)
{
	/* NOTE: When adding code to this, make sure this is called
	 * at appropriate location when !CONFIG_SCHED_IPI_SUPPORTED.
	 */
#ifdef CONFIG_TRACE_SCHED_IPI
	z_trace_sched_ipi();
#endif /* CONFIG_TRACE_SCHED_IPI */

#ifdef CONFIG_TIMESLICING
	z_time_slice();
#endif /* CONFIG_TIMESLICING */

#ifdef CONFIG_ARCH_IPI_LAZY_COPROCESSORS_SAVE
	arch_ipi_lazy_coprocessors_save();
#endif

#ifdef CONFIG_SCHED_IPI_SUPPORTED
	ipi_work_process(&_kernel.cpus[_current_cpu->id].ipi_workq);
#endif
}
