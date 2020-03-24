/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <autoconf.h>
 #include <devicetree.h>

MEMORY
     {
#if (DT_MMIO_SRAM_20200000_SIZE > 0) && !defined(CONFIG_DATA_OCRAM)
        OCRAM  (wx) : ORIGIN = DT_MMIO_SRAM_20200000_BASE_ADDRESS, LENGTH = DT_MMIO_SRAM_20200000_SIZE
#endif
#if (DT_MMIO_SRAM_80000000_SIZE > 0) && !defined(CONFIG_DATA_SEMC)
        SDRAM  (wx) : ORIGIN = DT_MMIO_SRAM_80000000_BASE_ADDRESS, LENGTH = DT_MMIO_SRAM_80000000_SIZE
#endif
#if (DT_REG_SIZE(DT_INST(0, nxp_imx_dtcm)) > 0) && !defined(CONFIG_DATA_DTCM)
        DTCM    (wx) : ORIGIN = DT_REG_ADDR(DT_INST(0, nxp_imx_dtcm)), LENGTH = DT_REG_ADDR(DT_INST(0, nxp_imx_dtcm))
#endif
#if (DT_REG_SIZE(DT_INST(0, nxp_imx_itcm)) > 0) && !defined(CONFIG_CODE_ITCM)
        ITCM    (wx) : ORIGIN = DT_REG_ADDR(DT_INST(0, nxp_imx_itcm)), LENGTH = DT_REG_ADDR(DT_INST(0, nxp_imx_itcm))
#endif
     }

#include <arch/arm/aarch32/cortex_m/scripts/linker.ld>
