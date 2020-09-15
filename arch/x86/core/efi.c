/*
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/x86/efi.h>
#include <string.h>
#include <sys/mem_manage.h>

static struct efi_system_table *efi;

void efi_init(struct efi_system_table *efi_sys_table)
{
	z_phys_map((uint8_t **)&efi, (uintptr_t)efi_sys_table,
		   sizeof(struct efi_system_table), K_MEM_PERM_RW);

	/* ToDo: Verify sys_table (size, crc...) */
}
