/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/arch/arm/mmu/arm_mmu.h>

/*
 * Default, empty MMU configuration.
 *
 * The arch core always maps the Zephyr execution regions (and, on ARMv6,
 * the low-vectors page at 0x0; see z_arm_mmu_init() in arm_mmu.c). A SoC
 * only needs to provide its own mmu_regions.c when it has to map *extra*
 * regions on top of those; doing so provides a strong definition of
 * mmu_config that overrides this weak default.
 *
 * This definition deliberately lives in its own translation unit, separate
 * from the code in arm_mmu.c that reads mmu_config, so that the compiler
 * cannot constant-fold the empty initializer into those read sites and
 * defeat a SoC's strong override.
 */
const struct arm_mmu_config mmu_config __weak = {
	.num_regions = 0,
	.mmu_regions = NULL,
};
