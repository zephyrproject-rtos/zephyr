/*
 * Copyright 2026 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/arch/arm64/arm_mmu.h>

/*
 * Default, empty MMU configuration.
 *
 * The arch core always maps the Zephyr execution regions, the devicetree
 * "zephyr,memory-region" nodes and the GIC register banks (see mmu.c). A SoC
 * only needs to provide its own mmu_regions.c when it has to map *extra*
 * regions on top of those; doing so provides a strong definition of mmu_config
 * that overrides this weak default.
 *
 * This definition deliberately lives in its own translation unit, separate
 * from the code in mmu.c that reads mmu_config, so that the compiler cannot
 * constant-fold the empty initializer into those read sites and defeat a SoC's
 * strong override.
 */
const struct arm_mmu_config mmu_config __weak = {
	.num_regions = 0,
	.mmu_regions = NULL,
};
