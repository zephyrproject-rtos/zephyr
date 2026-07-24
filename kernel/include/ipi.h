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
#ifdef CONFIG_SCHED_IPI_SUPPORTED
int z_ipi_work_submit(struct k_ipi_work *work, uint32_t cpu_bitmask,
		      k_ipi_func_t func);
#endif
#else
#define flag_ipi(ipi_mask) do { } while (false)
#define signal_pending_ipi() do { } while (false)
#endif /* CONFIG_SMP */


#endif /* ZEPHYR_KERNEL_INCLUDE_IPI_H_ */
