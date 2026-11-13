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
			} else if (IS_ENABLED(CONFIG_XTENSA_MPU)) {
				region_alloc = ROUND_UP(region_alloc, LLEXT_PAGE_SIZE);
				region_align = MAX(region_align, LLEXT_PAGE_SIZE);
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

#ifdef CONFIG_LLEXT_SHAREDLIB_IMAGE

/* Regions holding segment contents, which point into the image */
static bool llext_region_in_image(enum llext_mem mem_idx)
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
#ifdef CONFIG_LLEXT_RODATA_NO_RELOC
	case LLEXT_MEM_RODATA_NO_RELOC:
#endif
		return true;
	default:
		return false;
	}
}

static int llext_read_phdr(struct llext_loader *ldr, unsigned int idx, elf_phdr_t *phdr)
{
	int ret = llext_seek(ldr, ldr->hdr.e_phoff + idx * ldr->hdr.e_phentsize);

	if (ret != 0) {
		return ret;
	}

	return llext_read(ldr, phdr, sizeof(*phdr));
}

static void llext_free_image(struct llext *ext)
{
#ifdef CONFIG_MMU
	if (ext->mmu_permissions_set) {
		/* restore default RAM permissions */
		k_mem_update_flags(ext->image, ext->image_size, K_MEM_PERM_RW);
	}
#endif

	for (enum llext_mem mem_idx = 0; mem_idx < LLEXT_MEM_COUNT; mem_idx++) {
		if (llext_region_in_image(mem_idx)) {
			ext->mem[mem_idx] = NULL;
			ext->mem_size[mem_idx] = 0;
		}
	}

	llext_free_instr(ext, ext->image);
	ext->image = NULL;
}

/*
 * Load all PT_LOAD segments of a shared library into one allocation, each at
 * its linked offset from the others, and point the program regions into it.
 */
static int llext_copy_image(struct llext_loader *ldr, struct llext *ext)
{
	uintptr_t lo = UINTPTR_MAX;
	uintptr_t hi = 0;
	uintptr_t base;
	elf_phdr_t phdr;
	size_t size;
	int ret;

	if (ldr->hdr.e_phentsize != sizeof(elf_phdr_t)) {
		LOG_ERR("Invalid program header size %u", ldr->hdr.e_phentsize);
		return -ENOEXEC;
	}

	for (unsigned int i = 0; i < ldr->hdr.e_phnum; i++) {
		ret = llext_read_phdr(ldr, i, &phdr);
		if (ret != 0) {
			return ret;
		}
		if (phdr.p_type != PT_LOAD || phdr.p_memsz == 0) {
			continue;
		}
		if (phdr.p_filesz > phdr.p_memsz) {
			LOG_ERR("Segment %u file size exceeds memory size", i);
			return -ENOEXEC;
		}
		if (phdr.p_vaddr + phdr.p_memsz < phdr.p_vaddr) {
			LOG_ERR("Segment %u wraps around the address space", i);
			return -ENOEXEC;
		}
		lo = MIN(lo, phdr.p_vaddr);
		hi = MAX(hi, phdr.p_vaddr + phdr.p_memsz);
	}

	if (lo >= hi) {
		LOG_ERR("No loadable segment");
		return -ENOEXEC;
	}

	base = ROUND_DOWN(lo, LLEXT_PAGE_SIZE);
	size = ROUND_UP(hi - base, LLEXT_PAGE_SIZE);
#ifdef CONFIG_LLEXT_HEAP_MEMBLK
	size = ROUND_UP(size, CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE);
#endif

	ext->image = llext_aligned_alloc_instr(ext, LLEXT_PAGE_SIZE, size);
	if (ext->image == NULL) {
		LOG_ERR("Failed allocating %zu bytes for the image", size);
		return -ENOMEM;
	}
	ext->image_size = size;
	ext->load_bias = (uintptr_t)ext->image - base;

	/* Gaps between segments and the part of each segment past its file data are zero */
	memset(ext->image, 0, size);

	for (unsigned int i = 0; i < ldr->hdr.e_phnum; i++) {
		ret = llext_read_phdr(ldr, i, &phdr);
		if (ret != 0) {
			goto err;
		}
		if (phdr.p_type != PT_LOAD || phdr.p_filesz == 0) {
			continue;
		}
		ret = llext_seek(ldr, phdr.p_offset);
		if (ret == 0) {
			ret = llext_read(ldr, (void *)(ext->load_bias + phdr.p_vaddr),
					 phdr.p_filesz);
		}
		if (ret != 0) {
			goto err;
		}
	}

	for (enum llext_mem mem_idx = 0; mem_idx < LLEXT_MEM_COUNT; mem_idx++) {
		elf_shdr_t *region = ldr->sects + mem_idx;

		if (!llext_region_in_image(mem_idx) || region->sh_type == SHT_NULL ||
		    region->sh_size == 0) {
			continue;
		}
		if (region->sh_addr < lo || region->sh_addr + region->sh_size > hi) {
			LOG_ERR("Region %d is outside the loadable segments", mem_idx);
			ret = -ENOEXEC;
			goto err;
		}
		ext->mem[mem_idx] = (void *)(ext->load_bias + region->sh_addr);
		ext->mem_size[mem_idx] = region->sh_size;
		ext->mem_on_heap[mem_idx] = false;
	}

	ext->alloc_size += size;

	LOG_DBG("image: %p, size %zu, load bias %#lx", ext->image, size, ext->load_bias);

	return 0;

err:
	llext_free_image(ext);
	return ret;
}

#ifdef CONFIG_MMU
static void llext_set_image_flags(uintptr_t start, uintptr_t end, uint32_t p_flags)
{
	uint32_t flags = 0;

	if ((p_flags & PF_W) != 0U) {
		flags |= K_MEM_PERM_RW;
	}
	if ((p_flags & PF_X) != 0U) {
		flags |= K_MEM_PERM_EXEC;
	}
	k_mem_update_flags((void *)start, end - start, flags);
}

/*
 * Set the permissions of each segment from its p_flags. Segments are sorted
 * by address. When two segments share a page, that page gets the
 * permissions of both. Pages outside any segment keep the default RW.
 */
static void llext_adjust_image_permissions(struct llext_loader *ldr, struct llext *ext)
{
	uintptr_t prev_end = 0;
	uint32_t prev_flags = 0;
	elf_phdr_t phdr;

	sys_cache_data_flush_range(ext->image, ext->image_size);
	sys_cache_instr_invd_range(ext->image, ext->image_size);

	for (unsigned int i = 0; i < ldr->hdr.e_phnum; i++) {
		if (llext_read_phdr(ldr, i, &phdr) != 0) {
			LOG_ERR("Failed reading program header %u", i);
			return;
		}
		if (phdr.p_type != PT_LOAD || phdr.p_memsz == 0) {
			continue;
		}

		uintptr_t start = ROUND_DOWN(ext->load_bias + phdr.p_vaddr, LLEXT_PAGE_SIZE);
		uintptr_t end =
			ROUND_UP(ext->load_bias + phdr.p_vaddr + phdr.p_memsz, LLEXT_PAGE_SIZE);

		if (start < prev_end) {
			llext_set_image_flags(start, prev_end, prev_flags | phdr.p_flags);
			start = prev_end;
		}
		if (start < end) {
			llext_set_image_flags(start, end, phdr.p_flags);
		}
		prev_end = end;
		prev_flags = phdr.p_flags;
	}
}
#endif /* CONFIG_MMU */

#endif /* CONFIG_LLEXT_SHAREDLIB_IMAGE */

int llext_copy_regions(struct llext_loader *ldr, struct llext *ext,
		       const struct llext_load_param *ldr_parm)
{
#ifdef CONFIG_LLEXT_SHAREDLIB_IMAGE
	if (ldr->hdr.e_type == ET_DYN && !ldr_parm->pre_located) {
		int ret = llext_copy_image(ldr, ext);

		if (ret < 0) {
			return ret;
		}
	}
#endif

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
				if (name == NULL) {
					LOG_WRN("-s (out of bounds section name string table "
						"index) %#zx",
						(size_t)llext_loaded_sect_ptr(ldr, ext, i));
				} else {
					LOG_DBG("-s %s %#zx", name,
						(size_t)llext_loaded_sect_ptr(ldr, ext, i));
				}
			}
		}
	}

	return 0;
}

void llext_adjust_mmu_permissions(struct llext_loader *ldr, struct llext *ext)
{
#ifdef CONFIG_MMU
	void *addr;
	size_t size;
	uint32_t flags;

#ifdef CONFIG_LLEXT_SHAREDLIB_IMAGE
	if (ext->image != NULL) {
		llext_adjust_image_permissions(ldr, ext);
		ext->mmu_permissions_set = true;
		return;
	}
#endif

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
#ifdef CONFIG_LLEXT_SHAREDLIB_IMAGE
	if (ext->image != NULL) {
		llext_free_image(ext);
	}
#endif

	for (int i = 0; i < LLEXT_MEM_COUNT; i++) {
#ifdef CONFIG_MMU
		if (ext->mmu_permissions_set && ext->mem_size[i] != 0 &&
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

	llext_heap_reset(ext);
}

int llext_add_domain(struct llext *ext, struct k_mem_domain *domain)
{
#ifdef CONFIG_USERSPACE
	int ret = 0;

	if (llext_is_image(ext)) {
		/* No memory partitions are defined for an image */
		return -ENOTSUP;
	}

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
