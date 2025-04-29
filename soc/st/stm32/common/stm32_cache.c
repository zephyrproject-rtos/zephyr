/*
 * Copyright (c) 2025 Henrik Lindblom <henrik.lindblom@vaisala.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "stm32_cache.h"
#include <zephyr/linker/linker-defs.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr-arm.h>
#include <zephyr/sys/math_extras.h>

bool stm32_buf_in_nocache(uintptr_t buf, size_t len_bytes)
{
	uintptr_t buf_end;

	/* Note: (len_bytes - 1) is safe because a length of 0 would overflow to 4 billion. */
	if (u32_add_overflow(buf, len_bytes - 1, (uint32_t *)&buf_end)) {
		return false;
	}

#ifdef CONFIG_EXTERNAL_CACHE
	/* STM32 DCACHE is enabled but it does not cover internal SRAM. If the buffer is in SRAM it
	 * is not cached by DCACHE.
	 *
	 * NOTE: This incorrectly returns true if Zephyr image RAM resides in external memory.
	 */
	if (((uintptr_t)_image_ram_start) <= buf && buf_end < ((uintptr_t)_image_ram_end)) {
		return true;
	}
#endif /* CONFIG_EXTERNAL_CACHE */

#ifdef CONFIG_NOCACHE_MEMORY
	/* Check if buffer is in NOCACHE region defined by the linker. */
	if ((uintptr_t)_nocache_ram_start <= buf && buf_end <= (uintptr_t)_nocache_ram_end) {
		return true;
	}
#endif /* CONFIG_NOCACHE_MEMORY */

#ifdef CONFIG_MEM_ATTR
	/* Check if buffer is in a region marked NOCACHE in DT. */
	if (mem_attr_check_buf((void *)buf, len_bytes, DT_MEM_ARM_MPU_RAM_NOCACHE) == 0) {
		return true;
	}
#endif /* CONFIG_MEM_ATTR */

	/* Check if buffer is in read-only region, which cannot be stale due to DCACHE because it is
	 * not writeable.
	 */
	if ((uintptr_t)__rodata_region_start <= buf && buf_end <= (uintptr_t)__rodata_region_end) {
		return true;
	}

	/* Not in any region known to be NOCACHE */
	return false;
}
