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
void z_riscv_custom_pmp_entry_enable(void);


#ifdef CONFIG_CUSTOM_PMP_ENTRY

/**
 * @brief Sets the write permission for a specific PMP entry.
 *
 * Searches for the PMP entry matching CUSTOM_PMP_ENTRY_START.
 * Modifies the Write (W) bit in this entry's PMP configuration.
 *
 * @note This function currently supports up to 8 PMP slots (CONFIG_PMP_SLOTS <=
 * 8). Extending beyond 8 slots in C requires more switch cases or an assembly
 * helper.
 *
 * @param write_enable If true, enables writes to the region (sets W bit).
 *                     If false, disables writes (clears W bit).
 *
 * @return 0 on success.
 *         -ENOENT if the PMP entry for CUSTOM_PMP_ENTRY_START is not found.
 *         -ENOTSUP if CONFIG_PMP_SLOTS > 8.
 *         -EINVAL for unexpected internal errors.
 */
int riscv_pmp_set_write_permission(bool write_enable);

#endif

#endif /* PMP_H_ */
