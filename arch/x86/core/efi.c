/*
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/x86/efi.h>
#include <string.h>
#include <sys/mem_manage.h>

static struct efi_system_table *efi;
static struct efi_configuration_table *ect;

static inline int efi_guid_compare(efi_guid_t *s1, efi_guid_t *s2)
{
	return memcmp(s1, s2, sizeof(efi_guid_t));
}

void efi_init(struct efi_system_table *efi_sys_table)
{
	z_phys_map((uint8_t **)&efi, (uintptr_t)efi_sys_table,
		   sizeof(struct efi_system_table), K_MEM_PERM_RW);

	/* ToDo: Verify sys_table (size, crc...) */

	/* We are interested by the configuration table, so let's map it */
	z_phys_map((uint8_t **)&ect,
		   (uintptr_t)efi->ConfigurationTable,
		   sizeof(struct efi_configuration_table) *
		   efi->NumberOfTableEntries,
		   K_MEM_PERM_RW);
}

void *efi_config_get_vendor_table_by_guid(efi_guid_t *guid)
{
	struct efi_configuration_table *ect_tmp = ect;
	int n_ct;

	if (efi == NULL) {
		return NULL;
	}

	for (n_ct = 0; n_ct < efi->NumberOfTableEntries; n_ct++) {
		if (efi_guid_compare(&ect_tmp->VendorGuid, guid) == 0) {
			return ect_tmp->VendorTable;
		}

		ect_tmp++;
	}

	return NULL;
}
