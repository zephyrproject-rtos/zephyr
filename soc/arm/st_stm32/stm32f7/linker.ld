/* linker.ld - Linker command/script file */

/*
 * Copyright (c) 2018 Yurii Hamann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <autoconf.h>
 #include <devicetree.h>

MEMORY
     {
#if (DT_MMIO_SRAM_20010000_SIZE > 0) && !defined(CONFIG_DATA_OCRAM)
        OCRAM  (wx) : ORIGIN = DT_MMIO_SRAM_20010000_BASE_ADDRESS, LENGTH = DT_MMIO_SRAM_20010000_SIZE
#endif
#if (DT_MMIO_SRAM_C0000000_SIZE > 0) && !defined(CONFIG_DATA_FMC)
        SDRAM  (wx) : ORIGIN = DT_MMIO_SRAM_C0000000_BASE_ADDRESS, LENGTH = DT_MMIO_SRAM_C0000000_SIZE
#endif
     }

#include <arch/arm/aarch32/cortex_m/scripts/linker.ld>
