/*
 * Copyright (c) 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RISCV_PMP_H_
#define ZEPHYR_INCLUDE_RISCV_PMP_H_

#include <zephyr/arch/riscv/arch.h>
#include <zephyr/sys/iterable_sections.h>

/**
 * @brief SoC-specific PMP region descriptor.
 *
 * SoCs can define additional memory regions that need PMP protection
 * using the PMP_SOC_REGION_DEFINE macro.
 * These regions are automatically collected via iterable sections and
 * programmed into the PMP during initialization.
 *
 * Note: Uses start/end pointers instead of start/size to support regions
 * defined by linker symbols where the size is not a compile-time constant.
 */
struct pmp_soc_region {
	/** Start address of the region (must be aligned to PMP granularity) */
	const void *start;
	/** End address of the region (exclusive) */
	const void *end;
	/** PMP permission flags (PMP_R, PMP_W, PMP_X combinations) */
	uint8_t perm;
};

/**
 * @brief Define a SoC-specific PMP region.
 *
 * This macro allows SoCs to register memory regions that require PMP
 * protection. The regions are collected at link time and programmed
 * during PMP initialization.
 *
 * @param name Unique identifier for this region
 * @param _start Start address of the region (pointer or linker symbol)
 * @param _end End address of the region (pointer or linker symbol, exclusive)
 * @param _perm PMP permission flags (PMP_R | PMP_W | PMP_X)
 */
#define PMP_SOC_REGION_DEFINE(name, _start, _end, _perm)                                           \
	static const STRUCT_SECTION_ITERABLE(pmp_soc_region, name) = {                             \
		.start = (const void *)(_start),                                                   \
		.end = (const void *)(_end),                                                       \
		.perm = (_perm),                                                                   \
	}

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
