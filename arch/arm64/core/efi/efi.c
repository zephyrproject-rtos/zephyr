/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/arch/arm64/efi.h>

/*
 * Filled in by efi_stub.S. In .data, so it is part of the copied image and
 * BSS init leaves it alone.
 */
struct arm64_efi_boot_info z_arm64_efi_boot_info
	__attribute__((section(".data.z_arm64_efi_boot_info")));

const struct arm64_efi_boot_info *arm64_efi_boot_info_get(void)
{
	return (z_arm64_efi_boot_info.system_table != 0U) ? &z_arm64_efi_boot_info : NULL;
}
