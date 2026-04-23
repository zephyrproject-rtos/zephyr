/*
 * Copyright (c) 2025 Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
SECTION_PROLOGUE (ambiq_dma_buff, CONFIG_SRAM_BASE_ADDRESS + CONFIG_SOC_AMBIQ_DMA_BUFF_LOCATION (NOLOAD),)
#else
SECTION_PROLOGUE (ambiq_dma_buff, DT_REG_ADDR(DT_CHOSEN(zephyr_sram)) + CONFIG_SOC_AMBIQ_DMA_BUFF_LOCATION (NOLOAD),)
#endif	
{
    __ambiq_dma_buff_start = .;
    KEEP(*(SORT_BY_NAME(".ambiq_dma_buff*")))
    . = ALIGN(CONFIG_SOC_AMBIQ_DMA_BUFF_ALIGNMENT);
    __ambiq_dma_buff_end = .;
} GROUP_LINK_IN(RAMABLE_REGION)
