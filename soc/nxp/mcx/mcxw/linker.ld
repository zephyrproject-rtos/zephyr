/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

m_sector_size                       = 0x2000;
m_flash_end                         = 0x000FFFFF;


m_fsl_prodInfo_size                 = m_sector_size;
m_fsl_prodInfo_start                = m_flash_end - m_fsl_prodInfo_size + 1;
m_fsl_prodInfo_end                  = m_flash_end;
PROD_DATA_BASE_ADDR                 = m_fsl_prodInfo_start;

/*
 * We perform all custom placement before including the generic linker file. This
 * is done because calling this linker at the beginning will place some sections
 * first, such as .noinit*, and this includes the rpmsg_sh_mem, which results
 * in placing the rpmsg section in RAM instead of shared mem.
 */

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
