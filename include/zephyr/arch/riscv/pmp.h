/*
 * Copyright (c) 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RISCV_PMP_H_
#define ZEPHYR_INCLUDE_RISCV_PMP_H_

 #include <zephyr/arch/riscv/arch.h>

#ifdef CONFIG_RISCV_PMP

/**
 * @brief Resets all unlocked PMP entries to OFF mode (Null Region).
 *
 * This function is used to securely clear the PMP configuration. It first
 * ensures the execution context is M-mode by setting MSTATUS_MPRV=0 and
 * MSTATUS_MPP=M-mode. It then reads all pmpcfgX CSRs, iterates through
 * the configuration bytes, and clears the Address Matching Mode bits (PMP_A)
 * for any entry that is not locked (PMP_L is clear), effectively disabling the region.
 */
void z_riscv_pmp_clear_all(void);

#endif

#endif /* ZEPHYR_INCLUDE_RISCV_PMP_H_ */
