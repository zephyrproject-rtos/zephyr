/**
 * Copyright (c) 2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kswap.h>
#include <ksched.h>
#include <ipi.h>

static struct k_spinlock ipi_lock;

#ifdef CONFIG_TRACE_SCHED_IPI
extern void z_trace_sched_ipi(void);
#endif

void flag_ipi(uint32_t ipi_mask)
{
#if defined(CONFIG_SCHED_IPI_SUPPORTED)
	if (arch_num_cpus() > 1) {
		atomic_or(&_kernel.pending_ipi, (atomic_val_t)ipi_mask);
	}
#endif /* CONFIG_SCHED_IPI_SUPPORTED */
}

/* Create a bitmask of CPUs that need an IPI. Note: sched_spinlock is held. */
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

void signal_pending_ipi(void)
{
	/* Synchronization note: you might think we need to lock these
	 * two steps, but an IPI is idempotent.  It's OK if we do it
	 * twice.  All we require is that if a CPU sees the flag true,
	 * it is guaranteed to send the IPI, and if a core sets
	 * pending_ipi, the IPI will be sent the next time through
	 * this code.
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
	if (thread_is_sliceable(_current)) {
		z_time_slice();
	}
#endif /* CONFIG_TIMESLICING */

#ifdef CONFIG_ARCH_IPI_LAZY_COPROCESSORS_SAVE
	arch_ipi_lazy_coprocessors_save();
#endif

#ifdef CONFIG_SCHED_IPI_SUPPORTED
	ipi_work_process(&_kernel.cpus[_current_cpu->id].ipi_workq);
#endif
}
