/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/arch/x86/efi.h>
#include <zephyr/arch/x86/legacy_bios.h>

#ifndef ZEPHYR_ARCH_X86_INCLUDE_X86_ACPI_H_
#define ZEPHYR_ARCH_X86_INCLUDE_X86_ACPI_H_

#if defined(CONFIG_X86_EFI)
static inline void *acpi_rsdp_get(void)
{
	void *rsdp = efi_get_acpi_rsdp();

	if (!rsdp) {
		rsdp = bios_acpi_rsdp_get();
	}

	return rsdp;
}
#else
static inline void *acpi_rsdp_get(void)
{
	return bios_acpi_rsdp_get();
}
#endif /* CONFIG_X86_EFI */

static inline uint64_t acpi_timer_get(void)
{
	return z_tsc_read();
}
#endif /* ZEPHYR_ARCH_X86_INCLUDE_X86_ACPI_H_ */
