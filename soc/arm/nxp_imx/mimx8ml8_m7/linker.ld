/*
 * Copyright (c) 2021, Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <autoconf.h>
#include <devicetree.h>

MEMORY
     {
#if defined(CONFIG_CODE_DDR)
        DDR  (wx) : ORIGIN = 0x80400000, LENGTH = 0x00C00000
#else
        DDR  (wx) : ORIGIN = 0x80000000, LENGTH = 0x01000000
#endif
     }

#include <arch/arm/aarch32/cortex_m/scripts/linker.ld>
