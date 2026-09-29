/*
 * Copyright (c) 2023 Intel Corporation
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/sys/minmax.h>
#include <zephyr/llext/loader.h>
#include <zephyr/llext/llext.h>
#include <zephyr/arch/common/instr_mem.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(llext, CONFIG_LLEXT_LOG_LEVEL);

#include <string.h>

#include "llext_priv.h"
#include "llext_mem.h"

#ifdef CONFIG_LLEXT_HEAP_DYNAMIC
bool llext_heap_inited;
#endif

/* PMP granularity is only defined when RISC-V PMP is built in. */
#ifdef CONFIG_PMP_GRANULARITY
#define LLEXT_PMP_GRANULARITY CONFIG_PMP_GRANULARITY
#else
#define LLEXT_PMP_GRANULARITY 1
#endif

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
#ifdef CONFIG_LLEXT_VENEERS
		case LLEXT_MEM_VENEER:
#endif
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

static bool llext_ptr_in_dyn_image(const struct llext *ext, const void *ptr)
{
	uintptr_t addr = (uintptr_t)ptr;

	return ext->dyn_base != 0 && addr >= ext->dyn_base && addr < ext->dyn_base + ext->dyn_span;
}

static bool llext_et_dyn_program_region(enum llext_mem mem_idx)
{
	switch (mem_idx) {
	case LLEXT_MEM_TEXT:
	case LLEXT_MEM_DATA:
	case LLEXT_MEM_RODATA:
	case LLEXT_MEM_BSS:
	case LLEXT_MEM_EXPORT:
	case LLEXT_MEM_PREINIT:
	case LLEXT_MEM_INIT:
	case LLEXT_MEM_FINI:
		return true;
	default:
		return false;
	}
}

static int llext_write_region(struct llext_loader *ldr, struct llext *ext, enum llext_mem mem_idx)
{
	elf_shdr_t *region = ldr->sects + mem_idx;
	uintptr_t base = (uintptr_t)ext->mem[mem_idx];
	size_t offset = region->sh_offset;
	size_t length = region->sh_size;
	int ret;

	if (region->sh_type == SHT_NOBITS) {
		memset(ext->mem[mem_idx], 0, region->sh_size);
		return 0;
	}

	if (region->sh_flags & SHF_ALLOC) {
		size_t prepad = region->sh_info;

		memset((void *)base, 0, prepad);
		base += prepad;
		offset += prepad;
		length -= prepad;
	}

	ret = llext_seek(ldr, offset);
	if (ret != 0) {
		return ret;
	}

	return llext_read(ldr, (void *)base, length);
}

/* One buffer, so PC-relative text-to-GOT distances stay intact. */
static int llext_prepare_et_dyn_image(struct llext_loader *ldr, struct llext *ext)
{
	uintptr_t vma_lo = UINTPTR_MAX;
	uintptr_t vma_hi = 0;
	size_t align = 4;

	if (IS_ENABLED(CONFIG_HARVARD)) {
		LOG_ERR("ET_DYN shared libraries are not supported on Harvard architectures");
		return -ENOTSUP;
	}

	for (enum llext_mem mem_idx = 0; mem_idx < LLEXT_MEM_COUNT; mem_idx++) {
		elf_shdr_t *region = ldr->sects + mem_idx;

		if (!llext_et_dyn_program_region(mem_idx) || region->sh_type == SHT_NULL ||
		    region->sh_size == 0 || !(region->sh_flags & SHF_ALLOC)) {
			continue;
		}

		if (region->sh_addr < vma_lo) {
			vma_lo = region->sh_addr;
		}
		if (region->sh_addr + region->sh_size > vma_hi) {
			vma_hi = region->sh_addr + region->sh_size;
		}
		if (region->sh_addralign > align) {
			align = region->sh_addralign;
		}
	}

	if (vma_lo == UINTPTR_MAX || vma_hi <= vma_lo) {
		LOG_ERR("ET_DYN file has no allocatable program sections");
		return -ENOEXEC;
	}

	/* ADRP and MMU updates need the link VMA's page offset. */
	if (IS_ENABLED(CONFIG_ARM64) && align < 4096U) {
		align = 4096U;
	}
#ifdef CONFIG_MMU
	if (align < LLEXT_PAGE_SIZE) {
		align = LLEXT_PAGE_SIZE;
	}
#endif
	uintptr_t map_lo = ROUND_DOWN(vma_lo, align);

	size_t span = (size_t)(vma_hi - map_lo);
#ifdef CONFIG_MMU
	/* Permission updates cover whole pages. Own that tail padding. */
	span = ROUND_UP(span, LLEXT_PAGE_SIZE);
#endif
	size_t bytes = span;

#ifdef CONFIG_LLEXT_HEAP_MEMBLK
	bytes = ROUND_UP(bytes, CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE);
#endif

	void *raw = llext_aligned_alloc_instr(ext, align, bytes);

	if (raw == NULL) {
		LOG_ERR("Failed to allocate %zu bytes for ET_DYN image", bytes);
		return -ENOMEM;
	}

	/* Aligned alloc already matches map_lo's congruence class (0 mod align). */
	uintptr_t base = (uintptr_t)raw;

	memset((void *)base, 0, span);

	ext->dyn_image = raw;
	ext->dyn_base = base;
	ext->dyn_link = map_lo;
	ext->dyn_span = span;
	ext->alloc_size += bytes;

	for (enum llext_mem mem_idx = 0; mem_idx < LLEXT_MEM_COUNT; mem_idx++) {
		elf_shdr_t *region = ldr->sects + mem_idx;

		if (!llext_et_dyn_program_region(mem_idx) || region->sh_type == SHT_NULL ||
		    region->sh_size == 0 || !(region->sh_flags & SHF_ALLOC)) {
			continue;
		}

		ext->mem[mem_idx] = (uint8_t *)base + (region->sh_addr - map_lo);
		ext->mem_size[mem_idx] = region->sh_size;
		ext->mem_on_heap[mem_idx] = false;
	}

	LOG_DBG("ET_DYN image base %p span %#zx (linked VMA %#zx)",
		(void *)base, span, (size_t)vma_lo);

	return 0;
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

	if (llext_ptr_in_dyn_image(ext, ext->mem[mem_idx])) {
		ext->mem_size[mem_idx] = region->sh_size;
		llext_init_mem_part(ext, mem_idx, (uintptr_t)ext->mem[mem_idx], region->sh_size);
		return llext_write_region(ldr, ext, mem_idx);
	}

	ext->mem_size[mem_idx] = region_alloc;

	/*
	 * Calculate the minimum region size and alignment that can satisfy
	 * MMU/MPU requirements. This only applies to regions that contain
	 * program-accessible data (not to string tables, for example).
	 */
	if (region->sh_flags & SHF_ALLOC) {
		if (IS_ENABLED(CONFIG_MMU)) {
			/* MMU targets map memory in page-sized chunks. Round
			 * the region to multiples of those.
			 */
			region_alloc = ROUND_UP(region_alloc, LLEXT_PAGE_SIZE);
			region_align = MAX(region_align, LLEXT_PAGE_SIZE);
		} else if (IS_ENABLED(CONFIG_USERSPACE)) {
			if (IS_ENABLED(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)) {
				/* Some MPU architectures (ARMv7-M, older ARC) require regions
				 * to be sized and aligned to the same power of two.
				 */
				uintptr_t block_sz =
					max3(region_alloc, region_align, LLEXT_PAGE_SIZE);

				block_sz = 1 << LOG2CEIL(block_sz); /* align to next power of two */
				region_alloc = block_sz;
				region_align = block_sz;
			} else if (IS_ENABLED(CONFIG_ARM_MPU) || IS_ENABLED(CONFIG_ARC_MPU)) {
				/* ARMv8-M and newer ARC MPUs use 32-byte alignment. */
				region_alloc = ROUND_UP(region_alloc, LLEXT_PAGE_SIZE);
				region_align = MAX(region_align, LLEXT_PAGE_SIZE);
			} else if (IS_ENABLED(CONFIG_RISCV_PMP)) {
				/*
				 * RISC-V PMP regions only need to be sized and
				 * aligned to the PMP granularity; TOR matching
				 * removes any power-of-two requirement.
				 */
				region_alloc = ROUND_UP(region_alloc, LLEXT_PMP_GRANULARITY);
				region_align = MAX(region_align, LLEXT_PMP_GRANULARITY);
			} else {
				LOG_ERR("region %d: no memory protection alignment "
					"rule for this architecture", mem_idx);
				return -ENOTSUP;
			}
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
				if ((IS_ALIGNED(ext->mem[mem_idx], region_align) ||
				     ldr_parm->pre_located) &&
				    ((mem_idx != LLEXT_MEM_TEXT) ||
				     arch_is_instr_mem(ext->mem[mem_idx], region_alloc))) {
					/* Map this region directly to the ELF buffer */
					llext_init_mem_part(ext, mem_idx,
							    (uintptr_t)ext->mem[mem_idx],
							    region_alloc);
					ext->mem_on_heap[mem_idx] = false;
					return 0;
				}

				if ((mem_idx == LLEXT_MEM_TEXT) &&
				    !arch_is_instr_mem(ext->mem[mem_idx], region_alloc)) {
					LOG_WRN("Cannot reuse ELF buffer for region %d, not "
						"instruction memory: %p-%p",
						mem_idx, ext->mem[mem_idx],
						(void *)((uintptr_t)(ext->mem[mem_idx]) +
							 region->sh_size));
				}
				if (!IS_ALIGNED(ext->mem[mem_idx], region_align)) {
					LOG_WRN("Cannot peek region %d: %p not aligned to %#zx",
						mem_idx, ext->mem[mem_idx], (size_t)region_align);
				}
			}
		} else if (ldr_parm->pre_located
#ifdef CONFIG_LLEXT_VENEERS
		   && mem_idx != LLEXT_MEM_VENEER
#endif
		   ) {
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

	if (ldr_parm->pre_located
#ifdef CONFIG_LLEXT_VENEERS
	    && mem_idx != LLEXT_MEM_VENEER
#endif
	    ) {
		/*
		 * The ELF file is supposed to be pre-located, but some
		 * regions are not accessible or not in the correct place.
		 */
		return -EFAULT;
	}

#ifdef CONFIG_LLEXT_HEAP_MEMBLK
	/* If allocating to heap, allocation must be multiple of block size */
	region_alloc = ROUND_UP(region_alloc, CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE);
#endif

	/* Allocate a suitably aligned area for the region. */
	if (region->sh_flags & SHF_EXECINSTR) {
		ext->mem[mem_idx] = llext_aligned_alloc_instr(ext, region_align, region_alloc);
	} else {
		ext->mem[mem_idx] = llext_aligned_alloc_data(ext, region_align, region_alloc);
	}

	if (!ext->mem[mem_idx]) {
		LOG_ERR("Failed allocating %zd bytes %zd-aligned for region %d",
			(size_t)region_alloc, (size_t)region_align, mem_idx);
		return -ENOMEM;
	}

	ext->alloc_size += region_alloc;

	llext_init_mem_part(ext, mem_idx, (uintptr_t)ext->mem[mem_idx],
		region_alloc);

	ret = llext_write_region(ldr, ext, mem_idx);
	if (ret != 0) {
		goto err;
	}

	ext->mem_on_heap[mem_idx] = true;

	return 0;

err:
	if (region->sh_flags & SHF_EXECINSTR) {
		llext_free_instr(ext, ext->mem[mem_idx]);
	} else {
		llext_free_data(ext, ext->mem[mem_idx]);
	}
	ext->mem[mem_idx] = NULL;
	return ret;
}

int llext_copy_strings(struct llext_loader *ldr, struct llext *ext,
		       const struct llext_load_param *ldr_parm)
{
	llext_heap_reset(ext);

	int ret = llext_copy_region(ldr, ext, LLEXT_MEM_SHSTRTAB, ldr_parm);

	if (!ret) {
		ret = llext_copy_region(ldr, ext, LLEXT_MEM_STRTAB, ldr_parm);
	}

	return ret;
}

int llext_copy_regions(struct llext_loader *ldr, struct llext *ext,
		       const struct llext_load_param *ldr_parm)
{
	/* Xtensa uses its own PLT path and may be Harvard. */
	if (!IS_ENABLED(CONFIG_XTENSA) && ldr->hdr.e_type == ET_DYN && !ldr_parm->pre_located) {
		int ret = llext_prepare_et_dyn_image(ldr, ext);

		if (ret < 0) {
			return ret;
		}
	}

	for (enum llext_mem mem_idx = 0; mem_idx < LLEXT_MEM_COUNT; mem_idx++) {
		/* strings have already been copied */
		if (ext->mem[mem_idx] && !llext_ptr_in_dyn_image(ext, ext->mem[mem_idx])) {
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
				if (name == NULL) {
					LOG_WRN("-s (out of bounds section name string table "
						"index) %#zx",
						(size_t)ext->mem[mem_idx] +
							ldr->sect_map[i].offset);
				} else {
					LOG_DBG("-s %s %#zx", name,
						(size_t)ext->mem[mem_idx] +
							ldr->sect_map[i].offset);
				}
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

	/* AArch64 will not execute a writable page. */
	if (ext->dyn_base != 0U) {
		uintptr_t image = ext->dyn_base;
		uintptr_t image_end = image + ROUND_UP(ext->dyn_span, LLEXT_PAGE_SIZE);
		uintptr_t text = (uintptr_t)ext->mem[LLEXT_MEM_TEXT];
		uintptr_t text_end = text + ext->mem_size[LLEXT_MEM_TEXT];
		uintptr_t data = (uintptr_t)ext->mem[LLEXT_MEM_DATA];
		uintptr_t data_end = data + ext->mem_size[LLEXT_MEM_DATA];
		uintptr_t bss = (uintptr_t)ext->mem[LLEXT_MEM_BSS];
		uintptr_t bss_end = bss + ext->mem_size[LLEXT_MEM_BSS];

		for (uintptr_t page = image; page < image_end; page += LLEXT_PAGE_SIZE) {
			uintptr_t page_end = page + LLEXT_PAGE_SIZE;
			bool exec = text != 0U && page < text_end && page_end > text;
			bool write = (data != 0U && page < data_end && page_end > data) ||
				     (bss != 0U && page < bss_end && page_end > bss);

			if (exec && write) {
				LOG_ERR("ET_DYN page %p is both executable and writable",
					(void *)page);
				exec = false;
			}

			flags = exec ? K_MEM_PERM_EXEC : K_MEM_PERM_RW;
			addr = (void *)page;
			sys_cache_data_flush_range(addr, LLEXT_PAGE_SIZE);
			if (exec) {
				sys_cache_instr_invd_range(addr, LLEXT_PAGE_SIZE);
			}
			k_mem_update_flags(addr, LLEXT_PAGE_SIZE, flags);
		}

		ext->mmu_permissions_set = true;
		return;
	}

	for (enum llext_mem mem_idx = 0; mem_idx < LLEXT_MEM_PARTITIONS; mem_idx++) {
		addr = ext->mem[mem_idx];
		size = ROUND_UP(ext->mem_size[mem_idx], LLEXT_PAGE_SIZE);
		if (size == 0) {
			continue;
		}
		switch (mem_idx) {
		case LLEXT_MEM_TEXT:
#ifdef CONFIG_LLEXT_VENEERS
		case LLEXT_MEM_VENEER:
#endif
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
		if ((flags & K_MEM_PERM_EXEC) != 0) {
			/* new code must reach PoU (flush above) before the
			 * stale instruction lines are dropped
			 */
			sys_cache_instr_invd_range(addr, size);
		}
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
		    !llext_ptr_in_dyn_image(ext, ext->mem[i]) &&
		    (i == LLEXT_MEM_TEXT || i == LLEXT_MEM_RODATA
#ifdef CONFIG_LLEXT_VENEERS
		     || i == LLEXT_MEM_VENEER
#endif
		     )) {
			/* restore default RAM permissions of changed regions */
			k_mem_update_flags(ext->mem[i],
					   ROUND_UP(ext->mem_size[i], LLEXT_PAGE_SIZE),
					   K_MEM_PERM_RW);
		}
#endif
		if (ext->mem_on_heap[i]) {
			LOG_DBG("freeing memory region %d", i);

			if (i == LLEXT_MEM_TEXT
#ifdef CONFIG_LLEXT_VENEERS
			    || i == LLEXT_MEM_VENEER
#endif
			    ) {
				llext_free_instr(ext, ext->mem[i]);
			} else {
				llext_free_data(ext, ext->mem[i]);
			}

			ext->mem[i] = NULL;
		}
	}

	if (ext->dyn_image != NULL) {
		LOG_DBG("freeing ET_DYN image");
#ifdef CONFIG_MMU
		if (ext->mmu_permissions_set && ext->dyn_base != 0U) {
			k_mem_update_flags((void *)ext->dyn_base,
					   ROUND_UP(ext->dyn_span, LLEXT_PAGE_SIZE),
					   K_MEM_PERM_RW);
		}
#endif
		llext_free_instr(ext, ext->dyn_image);
		ext->dyn_image = NULL;
		ext->dyn_base = 0;
		ext->dyn_link = 0;
		ext->dyn_span = 0;
	}

	llext_heap_reset(ext);
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

int llext_heap_init_harvard(void *instr_mem, size_t instr_bytes, void *data_mem, size_t data_bytes)
{
#if !defined(CONFIG_LLEXT_HEAP_DYNAMIC) || !defined(CONFIG_HARVARD)
	return -ENOSYS;
#else
	if (llext_heap_inited) {
		return -EEXIST;
	}

	k_heap_init(&llext_instr_heap, instr_mem, instr_bytes);
	k_heap_init(&llext_data_heap, data_mem, data_bytes);

	llext_heap_inited = true;
	return 0;
#endif
}

int llext_heap_init(void *mem, size_t bytes)
{
#if !defined(CONFIG_LLEXT_HEAP_DYNAMIC) || defined(CONFIG_HARVARD)
	return -ENOSYS;
#else
	if (llext_heap_inited) {
		return -EEXIST;
	}

	k_heap_init(&llext_heap, mem, bytes);

	llext_heap_inited = true;
	return 0;
#endif
}

#ifdef CONFIG_LLEXT_HEAP_DYNAMIC
static int llext_loaded(struct llext *ext, void *arg)
{
	return 1;
}
#endif

int llext_heap_uninit(void)
{
#ifdef CONFIG_LLEXT_HEAP_DYNAMIC
	if (!llext_heap_inited) {
		return -EEXIST;
	}
	if (llext_iterate(llext_loaded, NULL)) {
		return -EBUSY;
	}
	llext_heap_inited = false;
	return 0;
#else
	return -ENOSYS;
#endif
}
