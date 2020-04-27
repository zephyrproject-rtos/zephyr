/* Macros for tagging symbols and putting them in the correct sections. */

/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_LINKER_SECTION_TAGS_H_
#define ZEPHYR_INCLUDE_LINKER_SECTION_TAGS_H_

#include <toolchain.h>

#if !defined(_ASMLANGUAGE)

#define __noinit		__in_section_unique(_NOINIT_SECTION_NAME)
#define __irq_vector_table	Z_GENERIC_SECTION(_IRQ_VECTOR_TABLE_SECTION_NAME)
#define __sw_isr_table		Z_GENERIC_SECTION(_SW_ISR_TABLE_SECTION_NAME)

#if defined(CONFIG_ARM)
#define __kinetis_flash_config_section __in_section_unique(_KINETIS_FLASH_CONFIG_SECTION_NAME)
#define __ti_ccfg_section Z_GENERIC_SECTION(_TI_CCFG_SECTION_NAME)
#define __ccm_data_section Z_GENERIC_SECTION(_CCM_DATA_SECTION_NAME)
#define __ccm_bss_section Z_GENERIC_SECTION(_CCM_BSS_SECTION_NAME)
#define __ccm_noinit_section Z_GENERIC_SECTION(_CCM_NOINIT_SECTION_NAME)
#define __dtcm_data_section Z_GENERIC_SECTION(_DTCM_DATA_SECTION_NAME)
#define __dtcm_bss_section Z_GENERIC_SECTION(_DTCM_BSS_SECTION_NAME)
#define __dtcm_noinit_section Z_GENERIC_SECTION(_DTCM_NOINIT_SECTION_NAME)
#define __imx_boot_conf_section Z_GENERIC_SECTION(_IMX_BOOT_CONF_SECTION_NAME)
#define __imx_boot_data_section Z_GENERIC_SECTION(_IMX_BOOT_DATA_SECTION_NAME)
#define __imx_boot_ivt_section Z_GENERIC_SECTION(_IMX_BOOT_IVT_SECTION_NAME)
#define __imx_boot_dcd_section Z_GENERIC_SECTION(_IMX_BOOT_DCD_SECTION_NAME)
#endif /* CONFIG_ARM */

#if defined(CONFIG_NOCACHE_MEMORY)
#define __nocache __in_section_unique(_NOCACHE_SECTION_NAME)
#else
#define __nocache
#endif /* CONFIG_NOCACHE_MEMORY */

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_LINKER_SECTION_TAGS_H_ */
