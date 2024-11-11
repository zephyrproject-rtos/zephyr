/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RISCV_CORE_IPI_IMPL_H_
#define ZEPHYR_ARCH_RISCV_CORE_IPI_IMPL_H_

void z_riscv_sched_ipi_handler(unsigned int cpu_id);

#ifdef CONFIG_RISCV_SMP_IPI_CLINT
#include "ipi_clint.h"
#else /* CONFIG_RISCV_SMP_IPI_CUSTOM */
inline void z_riscv_ipi_send(unsigned int cpu);
inline void z_riscv_ipi_clear(unsigned int cpu);
int arch_smp_init(void);
#endif

#endif /* ZEPHYR_ARCH_RISCV_CORE_IPI_IMPL_H_ */
