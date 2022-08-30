/*
 * Copyright (c) 2022 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PMP_H_
#define PMP_H_

void z_riscv_pmp_init(void);
void z_riscv_pmp_stackguard_prepare(struct k_thread *thread);
void z_riscv_pmp_stackguard_enable(struct k_thread *thread);
void z_riscv_pmp_usermode_init(struct k_thread *thread);
void z_riscv_pmp_usermode_prepare(struct k_thread *thread);
void z_riscv_pmp_usermode_enable(struct k_thread *thread);

#endif /* PMP_H_ */
