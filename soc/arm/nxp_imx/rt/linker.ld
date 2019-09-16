/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <autoconf.h>
 #include <generated_dts_board.h>

MEMORY
     {
#if (DT_MMIO_SRAM_20200000_SIZE > 0) && !defined(CONFIG_DATA_OCRAM)
        OCRAM  (wx) : ORIGIN = DT_MMIO_SRAM_20200000_BASE_ADDRESS, LENGTH = DT_MMIO_SRAM_20200000_SIZE
#endif
#if (DT_MMIO_SRAM_80000000_SIZE > 0) && !defined(CONFIG_DATA_SEMC)
        SDRAM  (wx) : ORIGIN = DT_MMIO_SRAM_80000000_BASE_ADDRESS, LENGTH = DT_MMIO_SRAM_80000000_SIZE
#endif
#if (DT_INST_0_NXP_IMX_DTCM_SIZE > 0) && !defined(CONFIG_DATA_DTCM)
        DTCM    (wx) : ORIGIN = DT_INST_0_NXP_IMX_DTCM_BASE_ADDRESS, LENGTH = DT_INST_0_NXP_IMX_DTCM_SIZE
#endif
#if (DT_INST_0_NXP_IMX_ITCM_SIZE > 0) && !defined(CONFIG_CODE_ITCM)
        ITCM    (wx) : ORIGIN = DT_INST_0_NXP_IMX_ITCM_BASE_ADDRESS, LENGTH = DT_INST_0_NXP_IMX_ITCM_SIZE
#endif
     }

#include <arch/arm/cortex_m/scripts/linker.ld>
