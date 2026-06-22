/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_LINKER_UTILS_H_
#define ZEPHYR_INCLUDE_LINKER_UTILS_H_

#include <stdbool.h>
#include <stddef.h>
#include <zephyr/toolchain.h>

/**
 * @brief Check if address is in a read only section.
 *
 * Checks the default .rodata region as well as any relocated
 * rodata sections (CCM_RODATA, SMEM_RODATA) whose linker symbols
 * are exposed via weak references.  This ensures that string
 * literals placed in custom rodata sections by
 * zephyr_code_relocate(LOCATION ..._RODATA) are still recognised
 * as read-only by cbprintf, preventing the deferred-logging
 * packager from NULLing their pointers and appending copies that
 * downstream IPC log transports may not reconstruct correctly.
 *
 * @param addr Address.
 *
 * @return True if address identified within any read only section.
 */
static inline bool linker_is_in_rodata(const void *addr)
{
#if defined(CONFIG_LINKER_USE_PINNED_SECTION)
	extern char lnkr_pinned_rodata_start[];
	extern char lnkr_pinned_rodata_end[];

	if (((const char *)addr >= (const char *)lnkr_pinned_rodata_start) &&
	    ((const char *)addr < (const char *)lnkr_pinned_rodata_end)) {
		return true;
	}
#endif

#if defined(CONFIG_ARM) || defined(CONFIG_ARC) || defined(CONFIG_X86) || \
	defined(CONFIG_ARM64) || defined(CONFIG_RISCV) || defined(CONFIG_SPARC) || \
	defined(CONFIG_MIPS) || defined(CONFIG_XTENSA) || defined(CONFIG_RX) || \
	defined(CONFIG_OPENRISC)
	extern char __rodata_region_start[];
	extern char __rodata_region_end[];

	if (((const char *)addr >= (const char *)__rodata_region_start) &&
	    ((const char *)addr < (const char *)__rodata_region_end)) {
		return true;
	}

	/* Relocated rodata sections (CCM, SMEM) live outside the default
	 * __rodata_region.  zephyr_code_relocate(... CCM_RODATA/SMEM_RODATA)
	 * brackets the actual strings with __<mem>_rodata_reloc_start/end
	 * (gen_relocate_app.py).  The bare __<mem>_rodata_start/end markers
	 * are zero-sized and never span the strings, so they must NOT be used
	 * here.  Weak symbols let builds without these sections skip the check.
	 *
	 * This is kept under the same arch guard as __rodata_region: on the
	 * native/POSIX targets the symbols never exist, and the native
	 * simulator's "objcopy --localize-symbol" link step strips the weak
	 * binding off the undefined references, turning them into hard
	 * undefined references that break the final link.
	 */
	extern char __weak __ccm_rodata_reloc_start[];
	extern char __weak __ccm_rodata_reloc_end[];

	if (((const char *)__ccm_rodata_reloc_start != NULL) &&
	    ((const char *)addr >= (const char *)__ccm_rodata_reloc_start) &&
	    ((const char *)addr < (const char *)__ccm_rodata_reloc_end)) {
		return true;
	}

	extern char __weak __smem_rodata_reloc_start[];
	extern char __weak __smem_rodata_reloc_end[];

	if (((const char *)__smem_rodata_reloc_start != NULL) &&
	    ((const char *)addr >= (const char *)__smem_rodata_reloc_start) &&
	    ((const char *)addr < (const char *)__smem_rodata_reloc_end)) {
		return true;
	}
#endif

	return false;
}

#endif /* ZEPHYR_INCLUDE_LINKER_UTILS_H_ */
