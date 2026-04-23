/*
 * Copyright (c) 2024 Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_DT_HAS_AMBIQ_MSPI_CONTROLLER_ENABLED
#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
    SECTION_PROLOGUE (mspi_buff, CONFIG_SRAM_BASE_ADDRESS + CONFIG_MSPI_AMBIQ_BUFF_RAM_LOCATION (NOLOAD),)
#else
    SECTION_PROLOGUE (mspi_buff, DT_REG_ADDR(DT_CHOSEN(zephyr_sram)) + CONFIG_MSPI_AMBIQ_BUFF_RAM_LOCATION (NOLOAD),)
#endif
    {
        __mspi_buff_start = .;
        KEEP(*(SORT_BY_NAME(".mspi_buff*")))
        . = ALIGN(CONFIG_MSPI_AMBIQ_BUFF_ALIGNMENT);
        __mspi_buff_end = .;
    } GROUP_LINK_IN(RAMABLE_REGION)
#endif
