/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <hexagon_vm.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* Cache operations using HVM vmcache hypercall */

void arch_dcache_flush_all(void)
{
	hexagon_vm_cache(hvmc_dccleaninva, 0, 0);
}

void arch_dcache_invd_all(void)
{
	hexagon_vm_cache(hvmc_dckill, 0, 0);
}

void arch_dcache_flush_and_invd_all(void)
{
	hexagon_vm_cache(hvmc_dccleaninva, 0, 0);
}

#ifdef CONFIG_MMU

void arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	/*
	 * Identity mapping is set up at boot via vmnewmap.  Runtime
	 * map requests beyond the boot-time range are not supported.
	 */
	LOG_WRN("%s: no-op (virt=%p phys=0x%lx size=%zu)",
		__func__, virt, (unsigned long)phys, size);
	ARG_UNUSED(virt);
	ARG_UNUSED(phys);
	ARG_UNUSED(size);
	ARG_UNUSED(flags);
}

void arch_mem_unmap(void *addr, size_t size)
{
	LOG_WRN("%s: no-op (addr=%p size=%zu)", __func__, addr, size);
	ARG_UNUSED(addr);
	ARG_UNUSED(size);
}

#endif /* CONFIG_MMU */
