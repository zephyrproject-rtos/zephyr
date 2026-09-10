/*
 * Copyright 2023, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

/*
 * We perform all custom placement before including the generic linker file. This
 * is done because calling this linker at the beginning will place some sections
 * first, such as .noinit*, and this includes the rpmsg_sh_mem, which results
 * in placing the rpmsg section in RAM instead of shared mem.
 */

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
