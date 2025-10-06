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
void z_riscv_pmp_stackguard_disable(void);
void z_riscv_pmp_usermode_init(struct k_thread *thread);
void z_riscv_pmp_usermode_prepare(struct k_thread *thread);
void z_riscv_pmp_usermode_enable(struct k_thread *thread);

/**
 * @brief Resets all unlocked PMP entries to OFF mode (Null Region).
 *
 * This function is used to securely clear the PMP configuration. It first
 * ensures the execution context is M-mode by setting MSTATUS_MPRV=0 and
 * MSTATUS_MPP=M-mode. It then reads all pmpcfgX CSRs, iterates through
 * the configuration bytes, and clears the Address Matching Mode bits (PMP_A)
 * for any entry that is not locked (PMP_L is clear), effectively disabling the region.
 */
void riscv_pmp_clear_all(void);

#endif /* PMP_H_ */
