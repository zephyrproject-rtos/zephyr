/*
 * Copyright (c) 2022 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PMP_H_
#define PMP_H_

#include <zephyr/dt-bindings/memory-attr/memory-attr-riscv.h>

#define PMPCFG_STRIDE (__riscv_xlen / 8)

#define DT_MEM_RISCV_TO_PMP_PERM(dt_attr) (		\
	(((dt_attr) & DT_MEM_RISCV_TYPE_IO_R) ? PMP_R : 0) |	\
	(((dt_attr) & DT_MEM_RISCV_TYPE_IO_W) ? PMP_W : 0) |	\
	(((dt_attr) & DT_MEM_RISCV_TYPE_IO_X) ? PMP_X : 0))

void z_riscv_pmp_init(void);
void z_riscv_pmp_kernelmode_prepare(struct k_thread *thread);
void z_riscv_pmp_kernelmode_enable(struct k_thread *thread);
void z_riscv_pmp_kernelmode_disable(void);
void z_riscv_pmp_usermode_init(struct k_thread *thread);
void z_riscv_pmp_usermode_prepare(struct k_thread *thread);
void z_riscv_pmp_usermode_enable(struct k_thread *thread);

#endif /* PMP_H_ */
