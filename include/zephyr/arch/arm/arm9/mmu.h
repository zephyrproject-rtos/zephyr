/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv5 MMU interface
 *
 * The ARMv5 MMU only understands the "short descriptor" format: 1 MB sections
 * and, below them, coarse page tables of 4 kB pages. This port maps the whole
 * address space with sections, which is all a flat-mapped RTOS needs, and
 * keeps a single coarse table around for the high vector page.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_ARM9_MMU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_ARM9_MMU_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <zephyr/sys/util.h>

/** Normal memory: cacheable and bufferable (write-back). */
#define ARMV5_MMU_MEM_CACHED   0U
/** Normal memory, not cacheable: writes may still go through the write buffer. */
#define ARMV5_MMU_MEM_BUFFERED 1U
/** Device / peripheral memory: neither cacheable nor bufferable. */
#define ARMV5_MMU_MEM_DEVICE   2U

/** One 1 MB-granular region of the physical address space. */
struct armv5_mmu_region {
	const char *name;
	uintptr_t base;
	size_t size;
	uint32_t attrs;
};

/** SoC supplied description of the address space. */
struct armv5_mmu_config {
	const struct armv5_mmu_region *regions;
	unsigned int num_regions;
};

#define ARMV5_MMU_REGION(_name, _base, _size, _attrs)                                              \
	{                                                                                          \
		.name = (_name),                                                                   \
		.base = (_base),                                                                   \
		.size = (_size),                                                                   \
		.attrs = (_attrs),                                                                 \
	}

/**
 * @brief Address space description, provided by the SoC.
 *
 * Every 1 MB section not covered by a region is mapped identity as device
 * memory, so a SoC only has to describe the memory it wants cached.
 */
extern const struct armv5_mmu_config soc_mmu_config;

/** @brief Build the translation tables and turn the MMU on. */
void z_armv5_mmu_init(void);

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_ARM9_MMU_H_ */
