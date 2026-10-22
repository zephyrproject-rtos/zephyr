/*
 * Copyright (c) 2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_KERNEL_INCLUDE_IPI_H_
#define ZEPHYR_KERNEL_INCLUDE_IPI_H_

#include <zephyr/kernel.h>
#include <stdint.h>
#include <zephyr/sys/atomic.h>

#define IPI_ALL_CPUS_MASK  (UINT32_MAX >> (32 - CONFIG_MP_MAX_NUM_CPUS))

#define IPI_CPU_MASK(cpu_id)   \
	(IS_ENABLED(CONFIG_IPI_OPTIMIZE) ? BIT(cpu_id) : IPI_ALL_CPUS_MASK)


void z_sched_ipi(void);

/* defined in ipi.c when CONFIG_SMP=y */
#ifdef CONFIG_SMP
void flag_ipi(uint32_t ipi_mask);
void signal_pending_ipi(void);
atomic_val_t ipi_mask_create(struct k_thread *thread);
#else
#define flag_ipi(ipi_mask) do { } while (false)
#define signal_pending_ipi() do { } while (false)
#endif /* CONFIG_SMP */

#ifdef CONFIG_IPI_OPTIMIZE_IDLE
/*
 * Consume and return the thread covered by the current CPU's reservation.
 * Returns NULL if no reservation exists. Caller must hold _sched_spinlock.
 */
struct k_thread *ipi_idle_reserved_take(void);

/*
 * Remove a thread's reservation before it leaves the run queue or changes
 * CPU eligibility. Not needed for a temporary dequeue/requeue with unchanged
 * eligibility.
 *
 * Caller must hold _sched_spinlock. Safe if no reservation exists; does not
 * cancel a pending IPI.
 */
void ipi_idle_thread_unreserve(struct k_thread *thread);

/*
 * Transfer coverage from old_thread to new_thread using an outstanding idle
 * CPU reservation. Returns true if new_thread is covered without another IPI.
 * Otherwise the reservation is canceled and the caller must request a new IPI.
 *
 * Caller must hold _sched_spinlock.
 */
bool ipi_idle_thread_rebind(struct k_thread *old_thread,
			    struct k_thread *new_thread);
#else
#define ipi_idle_reserved_take() NULL
#define ipi_idle_thread_unreserve(thread) do { \
	ARG_UNUSED(thread); \
} while (false)
#define ipi_idle_thread_rebind(old_thread, new_thread) false
#endif /* CONFIG_IPI_OPTIMIZE_IDLE */

#endif /* ZEPHYR_KERNEL_INCLUDE_IPI_H_ */
