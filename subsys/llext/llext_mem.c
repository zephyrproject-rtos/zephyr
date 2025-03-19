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

	LOG_DBG("region %d: start %#zx, size %zd", mem_idx, (size_t)start, len);
}

static int llext_copy_region(struct llext_loader *ldr, struct llext *ext,
			      enum llext_mem mem_idx, const struct llext_load_param *ldr_parm)
{
	int ret;
	elf_shdr_t *region = ldr->sects + mem_idx;
	uintptr_t region_alloc = region->sh_size;
	uintptr_t region_align = region->sh_addralign;

	if (!region_alloc) {
		return 0;
	}
	ext->mem_size[mem_idx] = region_alloc;

	/*
	 * Calculate the minimum region size and alignment that can satisfy
	 * MMU/MPU requirements. This only applies to regions that contain
	 * program-accessible data (not to string tables, for example).
	 */
	if (region->sh_flags & SHF_ALLOC) {
		if (IS_ENABLED(CONFIG_ARM_MPU)) {
			/* On ARM with an MPU, regions must be sized and
			 * aligned to the same power of two (larger than 32).
			 */
			uintptr_t block_sz = MAX(MAX(region_alloc, region_align), LLEXT_PAGE_SIZE);

			block_sz = 1 << LOG2CEIL(block_sz); /* align to next power of two */
			region_alloc = block_sz;
			region_align = block_sz;
		} else if (IS_ENABLED(CONFIG_MMU)) {
			/* MMU targets map memory in page-sized chunks. Round
			 * the region to multiples of those.
			 */
			region_alloc = ROUND_UP(region_alloc, LLEXT_PAGE_SIZE);
			region_align = MAX(region_align, LLEXT_PAGE_SIZE);
		}
	}

	if (ldr->storage == LLEXT_STORAGE_WRITABLE ||           /* writable storage         */
	    (ldr->storage == LLEXT_STORAGE_PERSISTENT &&        /* || persistent storage    */
	     !(region->sh_flags & SHF_WRITE) &&                 /*    && read-only region   */
	     !(region->sh_flags & SHF_LLEXT_HAS_RELOCS))) {     /*    && no relocs to apply */
		/*
		 * Try to reuse data areas from the ELF buffer, if possible.
		 * If any of the following tests fail, a normal allocation
		 * will be attempted.
		 */
		if (region->sh_type != SHT_NOBITS) {
			/* Region has data in the file, check if peek() is supported */
			ext->mem[mem_idx] = llext_peek(ldr, region->sh_offset);
			if (ext->mem[mem_idx]) {
				if (IS_ALIGNED(ext->mem[mem_idx], region_align) ||
				    ldr_parm->pre_located) {
					/* Map this region directly to the ELF buffer */
					llext_init_mem_part(ext, mem_idx,
							    (uintptr_t)ext->mem[mem_idx],
							    region_alloc);
					ext->mem_on_heap[mem_idx] = false;
					return 0;
				}

				LOG_WRN("Cannot peek region %d: %p not aligned to %#zx",
					mem_idx, ext->mem[mem_idx], (size_t)region_align);
			}
		} else if (ldr_parm->pre_located) {
			/*
			 * In pre-located files all regions, including BSS,
			 * are placed by the user with a linker script. No
			 * additional memory allocation is needed here.
			 */
			ext->mem[mem_idx] = NULL;
			ext->mem_on_heap[mem_idx] = false;
			return 0;
		}
	}

	if (ldr_parm->pre_located) {
		/*
		 * The ELF file is supposed to be pre-located, but some
		 * regions are not accessible or not in the correct place.
		 */
		return -EFAULT;
	}

	/* Allocate a suitably aligned area for the region. */
	ext->mem[mem_idx] = llext_aligned_alloc(region_align, region_alloc);
	if (!ext->mem[mem_idx]) {
		LOG_ERR("Failed allocating %zd bytes %zd-aligned for region %d",
			(size_t)region_alloc, (size_t)region_align, mem_idx);
		return -ENOMEM;
	}

	ext->alloc_size += region_alloc;

	llext_init_mem_part(ext, mem_idx, (uintptr_t)ext->mem[mem_idx],
		region_alloc);

	if (region->sh_type == SHT_NOBITS) {
		memset(ext->mem[mem_idx], 0, region->sh_size);
	} else {
		uintptr_t base = (uintptr_t)ext->mem[mem_idx];
		size_t offset = region->sh_offset;
		size_t length = region->sh_size;

		if (region->sh_flags & SHF_ALLOC) {
			/* zero out any prepad bytes, not part of the data area */
			size_t prepad = region->sh_info;

			memset((void *)base, 0, prepad);
			base += prepad;
			offset += prepad;
			length -= prepad;
		}

		/* actual data area without prepad bytes */
		ret = llext_seek(ldr, offset);
		if (ret != 0) {
			goto err;
		}

		ret = llext_read(ldr, (void *)base, length);
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

int llext_copy_strings(struct llext_loader *ldr, struct llext *ext,
		       const struct llext_load_param *ldr_parm)
{
	int ret = llext_copy_region(ldr, ext, LLEXT_MEM_SHSTRTAB, ldr_parm);

	if (!ret) {
		ret = llext_copy_region(ldr, ext, LLEXT_MEM_STRTAB, ldr_parm);
	}

	return ret;
}

int llext_copy_regions(struct llext_loader *ldr, struct llext *ext,
		       const struct llext_load_param *ldr_parm)
{
	for (enum llext_mem mem_idx = 0; mem_idx < LLEXT_MEM_COUNT; mem_idx++) {
		/* strings have already been copied */
		if (ext->mem[mem_idx]) {
			continue;
		}

		int ret = llext_copy_region(ldr, ext, mem_idx, ldr_parm);

		if (ret < 0) {
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_LLEXT_LOG_LEVEL_DBG)) {
		LOG_DBG("gdb add-symbol-file flags:");
		for (int i = 0; i < ext->sect_cnt; ++i) {
			elf_shdr_t *shdr = ext->sect_hdrs + i;
			enum llext_mem mem_idx = ldr->sect_map[i].mem_idx;
			const char *name = llext_section_name(ldr, ext, shdr);

			/* only show sections mapped to program memory */
			if (mem_idx < LLEXT_MEM_EXPORT) {
				LOG_DBG("-s %s %#zx", name,
					(size_t)ext->mem[mem_idx] + ldr->sect_map[i].offset);
			}
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

	ext->mmu_permissions_set = true;
#endif
}

void llext_free_regions(struct llext *ext)
{
	for (int i = 0; i < LLEXT_MEM_COUNT; i++) {
#ifdef CONFIG_MMU
		if (ext->mmu_permissions_set && ext->mem_size[i] != 0 &&
		    (i == LLEXT_MEM_TEXT || i == LLEXT_MEM_RODATA)) {
			/* restore default RAM permissions of changed regions */
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
