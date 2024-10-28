/*
 * Copyright (c) 2023 Intel Corporation
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/llext/loader.h>
#include <zephyr/llext/llext.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(llext, CONFIG_LLEXT_LOG_LEVEL);

#include <string.h>

#include "llext_priv.h"

#ifdef CONFIG_MMU_PAGE_SIZE
#define LLEXT_PAGE_SIZE CONFIG_MMU_PAGE_SIZE
#else
/* Arm's MPU wants a 32 byte minimum mpu region */
#define LLEXT_PAGE_SIZE 32
#endif

K_HEAP_DEFINE(llext_heap, CONFIG_LLEXT_HEAP_SIZE * 1024);

/*
 * Initialize the memory partition associated with the specified memory region
 */
static void llext_init_mem_part(struct llext *ext, enum llext_mem mem_idx,
			uintptr_t start, size_t len)
{
#ifdef CONFIG_USERSPACE
	if (mem_idx < LLEXT_MEM_PARTITIONS) {
		ext->mem_parts[mem_idx].start = start;
		ext->mem_parts[mem_idx].size = len;

		switch (mem_idx) {
		case LLEXT_MEM_TEXT:
			ext->mem_parts[mem_idx].attr = K_MEM_PARTITION_P_RX_U_RX;
			break;
		case LLEXT_MEM_DATA:
		case LLEXT_MEM_BSS:
			ext->mem_parts[mem_idx].attr = K_MEM_PARTITION_P_RW_U_RW;
			break;
		case LLEXT_MEM_RODATA:
			ext->mem_parts[mem_idx].attr = K_MEM_PARTITION_P_RO_U_RO;
			break;
		default:
			break;
		}
	}
#endif

	LOG_DBG("region %d: start 0x%zx, size %zd", mem_idx, (size_t)start, len);
}

static int llext_copy_section(struct llext_loader *ldr, struct llext *ext,
			      enum llext_mem mem_idx)
{
	int ret;

	if (!ldr->sects[mem_idx].sh_size) {
		return 0;
	}
	ext->mem_size[mem_idx] = ldr->sects[mem_idx].sh_size;

	if (ldr->sects[mem_idx].sh_type != SHT_NOBITS &&
	    IS_ENABLED(CONFIG_LLEXT_STORAGE_WRITABLE)) {
		/* Directly use data from the ELF buffer if peek() is supported */
		ext->mem[mem_idx] = llext_peek(ldr, ldr->sects[mem_idx].sh_offset);
		if (ext->mem[mem_idx]) {
			llext_init_mem_part(ext, mem_idx, (uintptr_t)ext->mem[mem_idx],
				ldr->sects[mem_idx].sh_size);
			ext->mem_on_heap[mem_idx] = false;
			return 0;
		}
	}

	/* On ARM with an MPU a pow(2, N)*32 sized and aligned region is needed,
	 * otherwise its typically an mmu page (sized and aligned memory region)
	 * we are after that we can assign memory permission bits on.
	 */
#ifndef CONFIG_ARM_MPU
	const uintptr_t sect_alloc = ROUND_UP(ldr->sects[mem_idx].sh_size, LLEXT_PAGE_SIZE);
	const uintptr_t sect_align = LLEXT_PAGE_SIZE;
#else
	uintptr_t sect_alloc = LLEXT_PAGE_SIZE;

	while (sect_alloc < ldr->sects[mem_idx].sh_size) {
		sect_alloc *= 2;
	}
	uintptr_t sect_align = sect_alloc;
#endif

	ext->mem[mem_idx] = llext_aligned_alloc(sect_align, sect_alloc);
	if (!ext->mem[mem_idx]) {
		return -ENOMEM;
	}

	ext->alloc_size += sect_alloc;

	llext_init_mem_part(ext, mem_idx, (uintptr_t)ext->mem[mem_idx],
		sect_alloc);

	if (ldr->sects[mem_idx].sh_type == SHT_NOBITS) {
		memset(ext->mem[mem_idx], 0, ldr->sects[mem_idx].sh_size);
	} else {
		ret = llext_seek(ldr, ldr->sects[mem_idx].sh_offset);
		if (ret != 0) {
			goto err;
		}

		ret = llext_read(ldr, ext->mem[mem_idx], ldr->sects[mem_idx].sh_size);
		if (ret != 0) {
			goto err;
		}
	}

	ext->mem_on_heap[mem_idx] = true;

	return 0;

err:
	llext_free(ext->mem[mem_idx]);
	ext->mem[mem_idx] = NULL;
	return ret;
}

int llext_copy_strings(struct llext_loader *ldr, struct llext *ext)
{
	int ret = llext_copy_section(ldr, ext, LLEXT_MEM_SHSTRTAB);

	if (!ret) {
		ret = llext_copy_section(ldr, ext, LLEXT_MEM_STRTAB);
	}

	return ret;
}

int llext_copy_regions(struct llext_loader *ldr, struct llext *ext)
{
	for (enum llext_mem mem_idx = 0; mem_idx < LLEXT_MEM_COUNT; mem_idx++) {
		/* strings have already been copied */
		if (ext->mem[mem_idx]) {
			continue;
		}

		int ret = llext_copy_section(ldr, ext, mem_idx);

		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

void llext_adjust_mmu_permissions(struct llext *ext)
{
#ifdef CONFIG_MMU
	void *addr;
	size_t size;
	uint32_t flags;

	for (enum llext_mem mem_idx = 0; mem_idx < LLEXT_MEM_PARTITIONS; mem_idx++) {
		addr = ext->mem[mem_idx];
		size = ROUND_UP(ext->mem_size[mem_idx], LLEXT_PAGE_SIZE);
		if (size == 0) {
			continue;
		}
		switch (mem_idx) {
		case LLEXT_MEM_TEXT:
			sys_cache_instr_invd_range(addr, size);
			flags = K_MEM_PERM_EXEC;
			break;
		case LLEXT_MEM_DATA:
		case LLEXT_MEM_BSS:
			/* memory is already K_MEM_PERM_RW by default */
			continue;
		case LLEXT_MEM_RODATA:
			flags = 0;
			break;
		default:
			continue;
		}
		sys_cache_data_flush_range(addr, size);
		k_mem_update_flags(addr, size, flags);
	}
#endif
}

void llext_free_regions(struct llext *ext)
{
	for (int i = 0; i < LLEXT_MEM_COUNT; i++) {
#ifdef CONFIG_MMU
		if (ext->mem_size[i] != 0) {
			/* restore default RAM permissions */
			k_mem_update_flags(ext->mem[i],
					   ROUND_UP(ext->mem_size[i], LLEXT_PAGE_SIZE),
					   K_MEM_PERM_RW);
		}
#endif
		if (ext->mem_on_heap[i]) {
			LOG_DBG("freeing memory region %d", i);
			llext_free(ext->mem[i]);
			ext->mem[i] = NULL;
		}
	}
}

int llext_add_domain(struct llext *ext, struct k_mem_domain *domain)
{
#ifdef CONFIG_USERSPACE
	int ret = 0;

	for (int i = 0; i < LLEXT_MEM_PARTITIONS; i++) {
		if (ext->mem_size[i] == 0) {
			continue;
		}
		ret = k_mem_domain_add_partition(domain, &ext->mem_parts[i]);
		if (ret != 0) {
			LOG_ERR("Failed adding memory partition %d to domain %p",
				i, domain);
			return ret;
		}
	}

	return ret;
#else
	return -ENOSYS;
#endif
}
