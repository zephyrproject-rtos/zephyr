/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_MMU_H
#define ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_MMU_H

#define Z_XTENSA_MMU_X  BIT(0)
#define Z_XTENSA_MMU_W  BIT(1)
#define Z_XTENSA_MMU_CACHED_WB BIT(2)
#define Z_XTENSA_MMU_CACHED_WT BIT(3)
#define Z_XTENSA_MMU_ILLEGAL (BIT(3) | BIT(2))

/* Struct used to map a memory region */
struct xtensa_mmu_range {
	const char *name;
	const uint32_t start;
	const uint32_t end;
	const uint32_t attrs;
};

extern const struct xtensa_mmu_range xtensa_soc_mmu_ranges[];
extern int xtensa_soc_mmu_ranges_num;

void z_xtensa_mmu_init(void);

void z_xtensa_mmu_smp_init(void);

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_MMU_H */
