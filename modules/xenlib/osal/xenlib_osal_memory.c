/*
 * SPDX-FileCopyrightText: 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xenlib Operating System Abstraction Layer (OSAL) memory runtime interface
 * implementation
 *
 */
#include <xenlib_osal_platform.h>
#include <zephyr/arch/cache.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/device_mmio.h>

void *xenlib_osal_memalloc(size_t msize)
{
	return k_malloc(msize);
}

void *xenlib_osal_memalloc_aligned(size_t align, size_t msize)
{
	return k_aligned_alloc(align, msize);
}

void xenlib_osal_memfree(void *mblock)
{
	k_free(mblock);
}

int xenlib_osal_map_phys_memory(void **vaddr, uintptr_t paddr, size_t size,
				   uint32_t flags)
{
	uint32_t cache = flags & K_MEM_CACHE_MASK;

	if (!vaddr || size == 0 ||
	    (flags & ~(K_MEM_CACHE_MASK | K_MEM_PERM_RW)) != 0 ||
	    (cache != K_MEM_CACHE_NONE && cache != K_MEM_CACHE_WB)) {
		return -EINVAL;
	}

#ifdef CONFIG_MMU
	uint8_t *mapped;

	if (IS_ENABLED(CONFIG_KERNEL_DIRECT_MAP)) {
		flags |= K_MEM_DIRECT_MAP;
	}
	k_mem_map_phys_bare(&mapped, paddr, size, flags);
	*vaddr = mapped;
#else
	/* Cache and access attributes follow the platform's static mapping. */
	mm_reg_t mapped = 0;

	device_map(&mapped, paddr, size, flags);
	*vaddr = (void *)mapped;
#endif
	return 0;
}

void xenlib_osal_unmap_phys_memory(void *vaddr, size_t size)
{
#ifdef CONFIG_MMU
	k_mem_unmap_phys_bare(vaddr, size);
#else
	device_unmap((mm_reg_t)vaddr, size);
#endif
}

int xenlib_osal_dcache_flush_and_invd_range(void *addr, size_t size)
{
	return cache_data_flush_and_invd_range(addr, size);
}

uint32_t xenlib_osal_be32_to_cpu(uint32_t value)
{
	return sys_be32_to_cpu(value);
}
