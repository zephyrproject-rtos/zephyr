/*
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/x86/efi.h>
#include <string.h>
#include <sys/mem_manage.h>

#define EFI_MEM_DESC_SIZE	sizeof(struct efi_memory_descriptor)
#define EFI_MAP_MIN_SLOTS	8

struct efi_mem_map {
	unsigned long size;
	unsigned long buff_size;
	unsigned long key;
	struct efi_memory_descriptor *desc;
	unsigned long desc_size;
	uint32_t desc_version;
};

static struct efi_system_table *efi;
static struct efi_configuration_table *ect;
static void *zephyr_img_handle;

static inline int efi_guid_compare(efi_guid_t *s1, efi_guid_t *s2)
{
	return memcmp(s1, s2, sizeof(efi_guid_t));
}

static uintptr_t efi_exit_boot_services(void *image_handle)
{
	struct efi_boot_services *bs = efi->BootServices;
	struct efi_mem_map mem_map = {
		.size = EFI_MEM_DESC_SIZE * 32,
	};
	uintptr_t status;

	/*
	 * Loop on:
	 * - allocate from pool some buffer
	 * - call get_memory_map
	 * - on success: stop the loop
	 * - on error/small buffer: free the buffer, increase size, loop
	 */
	while (true) {
		mem_map.buff_size = mem_map.size;

		status = bs->AllocatePool(EfiLoaderData, mem_map.size,
					  (void **)&mem_map.desc);
		if (status != EFI_SUCCESS) {
			return status;
		}

		status = bs->GetMemoryMap(&mem_map.buff_size, mem_map.desc,
					  &mem_map.key, &mem_map.desc_size,
					  &mem_map.desc_version);
		if (status == EFI_SUCCESS) {
			break;
		}

		bs->FreePool(&mem_map.desc);

		if ((status == EFI_BUFFER_TOO_SMALL) ||
		    ((mem_map.buff_size - mem_map.size) /
		     mem_map.desc_size >= EFI_MAP_MIN_SLOTS)) {
			mem_map.size += mem_map.desc_size *
				EFI_MAP_MIN_SLOTS;
			continue;
		}

		return status;
	}

	/*
	 * Then call ExitFromServices()
	 * If it fails, call again GetMemoryMap() with same previous params
	 * and retry ExitFromServices() once again.
	 */
	status = bs->ExitBootServices(image_handle, mem_map.key);
	if (status == EFI_INVALID_PARAMETER) {
		status = bs->GetMemoryMap(&mem_map.buff_size, mem_map.desc,
					  &mem_map.key, &mem_map.desc_size,
					  &mem_map.desc_version);
		if (status == EFI_SUCCESS) {
			status = bs->ExitBootServices(image_handle,
						      mem_map.key);
		}
	}

	return status;
}

void efi_init(struct efi_system_table *efi_sys_table, void *image_handle)
{
	z_phys_map((uint8_t **)&efi, (uintptr_t)efi_sys_table,
		   sizeof(struct efi_system_table), K_MEM_PERM_RW);

	zephyr_img_handle = image_handle;

	/* ToDo: Verify sys_table (size, crc...) */

	/* We are interested by the configuration table, so let's map it */
	z_phys_map((uint8_t **)&ect,
		   (uintptr_t)efi->ConfigurationTable,
		   sizeof(struct efi_configuration_table) *
		   efi->NumberOfTableEntries,
		   K_MEM_PERM_RW);
}

int efi_exit(void)
{
	uintptr_t status;

	status = efi_exit_boot_services(zephyr_img_handle);
	if (status != EFI_SUCCESS) {
		return -1;
	}

	efi->ConsoleInHandle = NULL;
	efi->ConIn = NULL;
	efi->ConsoleOutHandle = NULL;
	efi->ConOut = NULL;
	efi->StandardErrorHandle = NULL;
	efi->StdErr = NULL;
	efi->BootServices = NULL;

	return 0;
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
