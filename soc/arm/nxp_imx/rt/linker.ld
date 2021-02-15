/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <autoconf.h>
 #include <devicetree.h>

#define IS_CHOSEN_SRAM(x) (DT_DEP_ORD(DT_NODELABEL(x)) == DT_DEP_ORD(DT_CHOSEN(zephyr_sram)))

MEMORY
     {
#if (DT_REG_SIZE(DT_NODELABEL(ocram)) > 0) && !IS_CHOSEN_SRAM(ocram)
        OCRAM  (wx) : ORIGIN = DT_REG_ADDR(DT_NODELABEL(ocram)), LENGTH = DT_REG_SIZE(DT_NODELABEL(ocram))
#endif
#if (DT_REG_SIZE(DT_NODELABEL(sdram0)) > 0) && !IS_CHOSEN_SRAM(sdram0)
        SDRAM  (wx) : ORIGIN = DT_REG_ADDR(DT_NODELABEL(sdram0)), LENGTH = DT_REG_SIZE(DT_NODELABEL(sdram0))
#endif
     }

#include <arch/arm/aarch32/cortex_m/scripts/linker.ld>
