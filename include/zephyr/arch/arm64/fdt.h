/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Arm64 boot FDT access.
 * @ingroup arm64_boot_fdt
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_ARCH_ARM64_FDT_H_
#define ZEPHYR_INCLUDE_ZEPHYR_ARCH_ARM64_FDT_H_

#include <stdint.h>

/**
 * @defgroup arm64_boot_fdt Arm64 boot FDT
 * @ingroup arch-interface
 * @brief Access the boot Flattened Device Tree passed to an arm64 image.
 * @{
 */

/**
 * @brief Return a pointer to the saved boot FDT copy.
 *
 * The returned pointer refers to the Zephyr-owned copy made during early
 * boot. If @p fdt_size is not NULL, it is set to the number of valid bytes in
 * the saved copy. A reported size of 0 means no valid FDT copy has been
 * recorded.
 *
 * @param fdt_size Optional output for the saved FDT size in bytes.
 *
 * @return Pointer to the saved FDT copy.
 */
uintptr_t arm64_boot_fdt_get(uint32_t *fdt_size);

/** @} */

#endif /* ZEPHYR_INCLUDE_ZEPHYR_ARCH_ARM64_FDT_H_ */
