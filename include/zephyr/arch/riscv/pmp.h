/*
 * Copyright (c) 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RISCV_PMP_H_
#define ZEPHYR_INCLUDE_RISCV_PMP_H_

#include <zephyr/arch/riscv/arch.h>

/**
 * @brief Change the memory protection (R/W/X) permissions for a defined region.
 *
 * This function locates the PMP entry that corresponds to a region defined in
 * the device tree via memory attributes (as managed by mem_attr_get_regions)
 * and updates the PMP configuration register (pmpcfg) to reflect the new
 * permissions.
 *
 * It first iterates through all configured PMP entries, decodes their address
 * and size, and attempts to find a full match for the specified region's start
 * address and size. Once the matching PMP entry is found, its R, W, and X
 * bits are updated based on the provided 'perm' mask, and the change is
 * committed to the PMP configuration registers.
 *
 * @kconfig_dep{CONFIG_MEM_ATTR}
 * @kconfig_dep{CONFIG_RISCV_PMP}
 *
 * @param region_idx Index of the memory attribute region (from
 * mem_attr_get_regions) whose permissions should be changed.
 * @param perm The new PMP permissions (a combination of PMP_R, PMP_W, PMP_X).
 *
 * @retval 0 On success.
 * @retval -EINVAL If 'perm' contains invalid bits or 'region_idx' is out of bounds.
 * @retval -ENOENT If no matching PMP entry is found for the specified memory region.
 */
int z_riscv_pmp_change_permissions(size_t region_idx, uint8_t perm);

/**
 * @brief Resets all unlocked PMP entries to OFF mode (Null Region).
 *
 * @kconfig_dep{CONFIG_RISCV_PMP}
 *
 * This function is used to securely clear the PMP configuration. It first
 * ensures the execution context is M-mode by setting MSTATUS_MPRV=0 and
 * MSTATUS_MPP=M-mode. It then reads all pmpcfgX CSRs, iterates through
 * the configuration bytes, and clears the Address Matching Mode bits (PMP_A)
 * for any entry that is not locked (PMP_L is clear), effectively disabling the region.
 */
void z_riscv_pmp_clear_all(void);

#endif /* ZEPHYR_INCLUDE_RISCV_PMP_H_ */
