/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RISCV_PM_S2RAM_STRUCT_H_
#define ZEPHYR_ARCH_RISCV_PM_S2RAM_STRUCT_H_

#ifdef CONFIG_PM_S2RAM

struct __cpu_context {
	unsigned long mstatus;
	unsigned long mtvec;
	unsigned long mscratch;
#ifdef CONFIG_RISCV_HAS_CLIC
	unsigned long mtvt;
#else
	unsigned long mie;
#endif
	unsigned long sp;
};

typedef struct __cpu_context _cpu_context_t;

#endif /* CONFIG_PM_S2RAM */

#endif /* ZEPHYR_ARCH_RISCV_PM_S2RAM_STRUCT_H_ */
