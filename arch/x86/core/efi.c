/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/x86/efi.h>
#include <sys/mem_manage.h>

struct efi_boot_arg *efi;

void *efi_get_acpi_rsdp(void)
{
	if (efi == NULL) {
		return NULL;
	}

	return efi->acpi_rsdp;
}

void efi_init(struct efi_boot_arg *efi_arg)
{
	if (efi_arg == NULL) {
		return;
	}

	z_phys_map((uint8_t **)&efi, (uintptr_t)efi_arg,
		   sizeof(struct efi_boot_arg), 0);
}
