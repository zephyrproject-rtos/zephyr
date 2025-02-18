/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <getopt.h>
#include <zephyr/zvm/os.h>
#include <zephyr/zvm/zvm.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

#define MB_SIZE	 (1024 * 1024)

/**
 * Template of guest os, now for linux and zephyr.
 */
static struct z_os_info z_overall_vm_infos[] = {
	{
		.os_type = OS_TYPE_ZEPHYR,
		.vcpu_num = ZEPHYR_VM_VCPU_NUM,
		.vm_mem_base = ZEPHYR_VM_MEMORY_BASE,
		.vm_mem_size = ZEPHYR_VM_MEMORY_SIZE,
		.vm_image_base = ZEPHYR_IMAGE_BASE,
		.vm_image_size = ZEPHYR_IMAGE_SIZE,
		.vm_load_base = ZEPHYR_VM_LOAD_BASE,
		.entry_point = ZEPHYR_VM_MEMORY_BASE,
	},
	{
		.os_type = OS_TYPE_LINUX,
		.vcpu_num = LINUX_VM_VCPU_NUM,
		.vm_mem_base = LINUX_VM_MEMORY_BASE,
		.vm_mem_size = LINUX_VM_MEMORY_SIZE,
		.vm_image_base = LINUX_IMAGE_BASE,
		.vm_image_size = LINUX_IMAGE_SIZE,
		.vm_load_base = LINUX_VM_LOAD_BASE,
		.entry_point = LINUX_VM_MEMORY_BASE,
	},

};

int get_os_info_by_type(struct z_os_info *vm_info)
{
	struct getopt_state *state = getopt_state_get();
	char *vm_type = state->optarg;
	int ret = 0;
	struct z_os_info tmp_vm_info;

	if (strcmp(vm_type, "zephyr") == 0) {
		tmp_vm_info = z_overall_vm_infos[OS_TYPE_ZEPHYR];
		goto out;
	}

	if (strcmp(vm_type, "linux") == 0) {
		tmp_vm_info = z_overall_vm_infos[OS_TYPE_LINUX];
		goto out;
	}

	ZVM_LOG_WARN("The VM type is not supported(Linux or zephyr).\n Please try again!\n");
	return -EINVAL;

out:
	vm_info->vcpu_num = tmp_vm_info.vcpu_num;
	vm_info->vm_image_base = tmp_vm_info.vm_image_base;
	vm_info->vm_image_size = tmp_vm_info.vm_image_size;
	vm_info->vm_mem_base = tmp_vm_info.vm_mem_base;
	vm_info->vm_mem_size = tmp_vm_info.vm_mem_size;
	vm_info->os_type = tmp_vm_info.os_type;
	vm_info->vm_load_base = tmp_vm_info.vm_load_base;
	vm_info->entry_point = tmp_vm_info.entry_point;

	return ret;
}

int load_vm_image(struct vm_mem_domain *vmem_domain, struct z_os *os)
{
	int ret = 0;
	uint64_t *src_hva, des_hva;
	uint64_t num_m = os->info.vm_image_size / MB_SIZE;
	uint64_t src_hpa = os->info.vm_image_base;
	uint64_t des_hpa = os->info.vm_load_base;
	uint64_t per_size = MB_SIZE;

	ZVM_LOG_INFO("OS Image Loading ...\n");
	ZVM_LOG_INFO("Image_size = %lld MB\n", num_m);
	ZVM_LOG_INFO("Image_src_hpa = 0x%llx\n", src_hpa);
	ZVM_LOG_INFO("Image_des_hpa = 0x%llx\n", des_hpa);
	while (num_m) {
		k_mem_map_phys_bare(
			(uint8_t **)&src_hva, (uintptr_t)src_hpa, per_size,
			K_MEM_CACHE_NONE | K_MEM_PERM_RW);
		k_mem_map_phys_bare(
			(uint8_t **)&des_hva, (uintptr_t)des_hpa, per_size,
			K_MEM_CACHE_NONE | K_MEM_PERM_RW);

		memcpy((void *)des_hva, src_hva, per_size);
		k_mem_unmap_phys_bare((uint8_t *)src_hva, per_size);
		k_mem_unmap_phys_bare((uint8_t *)des_hva, per_size);
		des_hpa += per_size;
		src_hpa += per_size;
		num_m--;
	}

	if (os->info.os_type != OS_TYPE_LINUX) {
		ZVM_LOG_INFO("OS Image Loaded, No need other file!\n");
		return ret;
	}

	num_m = LINUX_VMDTB_SIZE / MB_SIZE;
	src_hpa = LINUX_VMDTB_BASE;
	des_hpa = LINUX_DTB_MEM_BASE;
	ZVM_LOG_INFO("DTB Image Loading ...\n");
	ZVM_LOG_INFO("DTB_size = %lld MB\n", num_m);
	ZVM_LOG_INFO("DTB_src_hpa = 0x%llx\n", src_hpa);
	ZVM_LOG_INFO("DTB_des_hpa = 0x%llx\n", des_hpa);
	while (num_m) {
		k_mem_map_phys_bare(
			(uint8_t **)&src_hva, (uintptr_t)src_hpa, per_size,
			K_MEM_CACHE_NONE | K_MEM_PERM_RW);
		k_mem_map_phys_bare(
			(uint8_t **)&des_hva, (uintptr_t)des_hpa, per_size,
			K_MEM_CACHE_NONE | K_MEM_PERM_RW);

		memcpy((void *)des_hva, src_hva, per_size);
		k_mem_unmap_phys_bare((uint8_t *)src_hva, per_size);
		k_mem_unmap_phys_bare((uint8_t *)des_hva, per_size);
		des_hpa += per_size;
		src_hpa += per_size;
		num_m--;
	}
	ZVM_LOG_INFO("Linux DTB Image Loaded !\n");

	num_m = LINUX_VMRFS_SIZE / MB_SIZE;
	src_hpa = LINUX_VMRFS_BASE;
	des_hpa = LINUX_VMRFS_PHY_BASE;
	ZVM_LOG_INFO("FS Image Loading ...\n");
	ZVM_LOG_INFO("FS_size = %lld MB\n", num_m);
	ZVM_LOG_INFO("FS_src_hpa = 0x%llx\n", src_hpa);
	ZVM_LOG_INFO("FS_des_hpa = 0x%llx\n", des_hpa);
	while (num_m) {
		k_mem_map_phys_bare(
			(uint8_t **)&src_hva, (uintptr_t)src_hpa, per_size,
			K_MEM_CACHE_NONE | K_MEM_PERM_RW);
		k_mem_map_phys_bare(
			(uint8_t **)&des_hva, (uintptr_t)des_hpa, per_size,
			K_MEM_CACHE_NONE | K_MEM_PERM_RW);

		memcpy((void *)des_hva, src_hva, per_size);
		k_mem_unmap_phys_bare((uint8_t *)src_hva, per_size);
		k_mem_unmap_phys_bare((uint8_t *)des_hva, per_size);
		des_hpa += per_size;
		src_hpa += per_size;
		num_m--;
	}
	ZVM_LOG_INFO("Linux FS Image Loaded !\n");

	return ret;
}

int vm_os_create(struct z_os *os, struct z_os_info *vm_info)
{
	os->info.os_type = vm_info->os_type;
	os->name = (char *)k_malloc(sizeof(char)*OS_NAME_LENGTH);
	memset(os->name, '\0', OS_NAME_LENGTH);

	switch (os->info.os_type) {
	case OS_TYPE_LINUX:
		strcpy(os->name, "linux_os");
		os->is_rtos = false;
		break;
	case OS_TYPE_ZEPHYR:
		strcpy(os->name, "zephyr_os");
		os->is_rtos = true;
		break;
	default:
		return -ENXIO;
	}
	os->info.vm_mem_base = vm_info->vm_mem_base;
	os->info.vm_mem_size = vm_info->vm_mem_size;
	os->info.vm_image_base = vm_info->vm_image_base;
	os->info.vm_image_size = vm_info->vm_image_size;
	os->info.vcpu_num = vm_info->vcpu_num;
	os->info.entry_point = vm_info->entry_point;
	os->info.vm_load_base = vm_info->vm_load_base;
	return 0;
}
