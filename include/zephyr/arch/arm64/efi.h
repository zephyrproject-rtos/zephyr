/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Information the arm64 EFI stub collects before ExitBootServices()
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_EFI_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_EFI_H_

#ifndef _ASMLANGUAGE

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief What UEFI handed over when Zephyr was started as an EFI application */
struct arm64_efi_boot_info {
	/**
	 * Physical address of the EFI system table. Zephyr does not map it:
	 * map it before use, with k_mem_map_phys_bare() for example. Some
	 * configuration tables may be in EfiBootServicesData memory, which
	 * stays intact only because Zephyr uses no memory outside its own RAM.
	 */
	uintptr_t system_table;
	/**
	 * Physical address of the UEFI memory map passed to ExitBootServices(),
	 * an array of EFI_MEMORY_DESCRIPTOR. Zephyr does not map it either. It
	 * is in EfiLoaderData memory, which UEFI leaves alone.
	 */
	uintptr_t memmap;
	/** Size of the memory map in bytes */
	size_t memmap_size;
	/** Size of one memory map entry in bytes, which may exceed sizeof(EFI_MEMORY_DESCRIPTOR) */
	size_t memmap_desc_size;
	/** EFI_MEMORY_DESCRIPTOR version */
	uint32_t memmap_desc_version;
};

/**
 * @brief Get what UEFI handed over at boot
 *
 * @return The boot information.
 * @retval NULL Zephyr was not started by UEFI.
 */
#if defined(CONFIG_BUILD_OUTPUT_EFI) || defined(__DOXYGEN__)
const struct arm64_efi_boot_info *arm64_efi_boot_info_get(void);
#else
static inline const struct arm64_efi_boot_info *arm64_efi_boot_info_get(void)
{
	return NULL;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_EFI_H_ */
