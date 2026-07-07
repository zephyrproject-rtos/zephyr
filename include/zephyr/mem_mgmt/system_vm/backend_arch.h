/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Architecture backend for the low-level virtual memory operations.
 *
 * This backend forwards every low-level virtual memory operation to the
 * architecture-specific arch_mem_*() implementation.
 */

#ifndef ZEPHYR_INCLUDE_MEM_MGMT_SYSTEM_VM_BACKEND_ARCH_H_
#define ZEPHYR_INCLUDE_MEM_MGMT_SYSTEM_VM_BACKEND_ARCH_H_

#include <zephyr/toolchain.h>
#include <kernel_arch_interface.h>

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

static ALWAYS_INLINE void sys_mm_vm_backend_mem_map(void *virt, uintptr_t phys, size_t size,
						    uint32_t flags)
{
	arch_mem_map(virt, phys, size, flags);
}

static ALWAYS_INLINE void sys_mm_vm_backend_mem_unmap(void *addr, size_t size)
{
	arch_mem_unmap(addr, size);
}

static ALWAYS_INLINE int sys_mm_vm_backend_page_phys_get(void *virt, uintptr_t *phys)
{
	return arch_page_phys_get(virt, phys);
}

static ALWAYS_INLINE void sys_mm_vm_backend_reserved_pages_update(void)
{
	arch_reserved_pages_update();
}

static ALWAYS_INLINE void sys_mm_vm_backend_mem_page_out(void *addr, uintptr_t location)
{
	arch_mem_page_out(addr, location);
}

static ALWAYS_INLINE void sys_mm_vm_backend_mem_page_in(void *addr, uintptr_t phys)
{
	arch_mem_page_in(addr, phys);
}

static ALWAYS_INLINE void sys_mm_vm_backend_mem_scratch(uintptr_t phys)
{
	arch_mem_scratch(phys);
}

static ALWAYS_INLINE enum arch_page_location
sys_mm_vm_backend_page_location_get(void *addr, uintptr_t *location)
{
	return arch_page_location_get(addr, location);
}

static ALWAYS_INLINE uintptr_t sys_mm_vm_backend_mem_page_info_get(void *addr, uintptr_t *location,
								   bool clear_accessed)
{
	return arch_page_info_get(addr, location, clear_accessed);
}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_MEM_MGMT_SYSTEM_VM_BACKEND_ARCH_H_ */
