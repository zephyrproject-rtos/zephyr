/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

  #include <zephyr/devicetree.h>

#define IS_CHOSEN_SRAM(x) (DT_DEP_ORD(DT_NODELABEL(x)) == DT_DEP_ORD(DT_CHOSEN(zephyr_sram)))

MEMORY
     {
#if (DT_REG_SIZE(DT_NODELABEL(sdram0)) > 0) && !IS_CHOSEN_SRAM(sdram0)
        SDRAM  (wx) : ORIGIN = DT_REG_ADDR(DT_NODELABEL(sdram0)), LENGTH = DT_REG_SIZE(DT_NODELABEL(sdram0))
#endif
     }

#include <zephyr/arch/arm/aarch32/cortex_m/scripts/linker.ld>
