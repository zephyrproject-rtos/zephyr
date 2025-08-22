/* Macros for tagging symbols and putting them in the correct sections. */

/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_LINKER_SECTION_TAGS_H_
#define ZEPHYR_INCLUDE_LINKER_SECTION_TAGS_H_

#include <zephyr/toolchain.h>

#if !defined(_ASMLANGUAGE)

#include <zephyr/linker/sections.h>

#define __noinit		__in_section_unique(_NOINIT_SECTION_NAME)
#define __noinit_named(name)	__in_section_unique_named(_NOINIT_SECTION_NAME, name)
#define __irq_vector_table	Z_GENERIC_SECTION(_IRQ_VECTOR_TABLE_SECTION_NAME)
#define __sw_isr_table		Z_GENERIC_SECTION(_SW_ISR_TABLE_SECTION_NAME)

#ifdef CONFIG_SHARED_INTERRUPTS
#define __shared_sw_isr_table	Z_GENERIC_SECTION(_SHARED_SW_ISR_TABLE_SECTION_NAME)
#endif /* CONFIG_SHARED_INTERRUPTS */

/* Attribute macros to place code and data into IMR memory */
#define __imr __in_section_unique(imr)
#define __imrdata __in_section_unique(imrdata)

#if defined(CONFIG_ARM)
#define __kinetis_flash_config_section __in_section_unique(_KINETIS_FLASH_CONFIG_SECTION_NAME)
#define __ti_ccfg_section Z_GENERIC_SECTION(_TI_CCFG_SECTION_NAME)
#define __ccm_data_section Z_GENERIC_SECTION(_CCM_DATA_SECTION_NAME)
#define __ccm_bss_section Z_GENERIC_SECTION(_CCM_BSS_SECTION_NAME)
#define __ccm_noinit_section Z_GENERIC_SECTION(_CCM_NOINIT_SECTION_NAME)
#define __itcm_section Z_GENERIC_SECTION(_ITCM_SECTION_NAME)
#define __dtcm_data_section Z_GENERIC_SECTION(_DTCM_DATA_SECTION_NAME)
#define __dtcm_bss_section Z_GENERIC_SECTION(_DTCM_BSS_SECTION_NAME)
#define __dtcm_noinit_section Z_GENERIC_SECTION(_DTCM_NOINIT_SECTION_NAME)
#define __ocm_data_section Z_GENERIC_SECTION(_OCM_DATA_SECTION_NAME)
#define __ocm_bss_section Z_GENERIC_SECTION(_OCM_BSS_SECTION_NAME)
#define __imx_boot_conf_section Z_GENERIC_SECTION(_IMX_BOOT_CONF_SECTION_NAME)
#define __imx_boot_data_section Z_GENERIC_SECTION(_IMX_BOOT_DATA_SECTION_NAME)
#define __imx_boot_ivt_section Z_GENERIC_SECTION(_IMX_BOOT_IVT_SECTION_NAME)
#define __imx_boot_dcd_section Z_GENERIC_SECTION(_IMX_BOOT_DCD_SECTION_NAME)
#define __imx_boot_container_section Z_GENERIC_SECTION(_IMX_BOOT_CONTAINER_SECTION_NAME)
#define __stm32_sdram1_section Z_GENERIC_SECTION(_STM32_SDRAM1_SECTION_NAME)
#define __stm32_sdram2_section Z_GENERIC_SECTION(_STM32_SDRAM2_SECTION_NAME)
#define __stm32_psram_section Z_GENERIC_SECTION(_STM32_PSRAM_SECTION_NAME)
#define __stm32_backup_sram_section Z_GENERIC_SECTION(_STM32_BACKUP_SRAM_SECTION_NAME)
#endif /* CONFIG_ARM */

#if defined(CONFIG_NOCACHE_MEMORY)
#define __nocache __in_section_unique(_NOCACHE_SECTION_NAME)
#define __nocache_load __in_section_unique(_NOCACHE_LOAD_SECTION_NAME)
#define __nocache_noinit __nocache
#else
#define __nocache
#define __nocache_load
#define __nocache_noinit __noinit
#endif /* CONFIG_NOCACHE_MEMORY */

#if defined(CONFIG_KERNEL_COHERENCE)
#define __incoherent __in_section_unique(cached)
#if defined(CONFIG_USERSPACE)
#define __stackmem Z_GENERIC_SECTION(.user_stacks)
#else
#define __stackmem __incoherent
#endif /* CONFIG_USERSPACE */
#define __kstackmem __incoherent
#else
#define __incoherent
#define __stackmem Z_GENERIC_SECTION(.user_stacks)
#define __kstackmem __noinit
#endif /* CONFIG_KERNEL_COHERENCE */

#if defined(CONFIG_LINKER_USE_BOOT_SECTION)
#define __boot_func	Z_GENERIC_DOT_SECTION(BOOT_TEXT_SECTION_NAME)
#define __boot_data	Z_GENERIC_DOT_SECTION(BOOT_DATA_SECTION_NAME)
#define __boot_rodata	Z_GENERIC_DOT_SECTION(BOOT_RODATA_SECTION_NAME)
#define __boot_bss	Z_GENERIC_DOT_SECTION(BOOT_BSS_SECTION_NAME)
#define __boot_noinit	Z_GENERIC_DOT_SECTION(BOOT_NOINIT_SECTION_NAME)
#else
#define __boot_func
#define __boot_data
#define __boot_rodata
#define __boot_bss
#define __boot_noinit	__noinit
#endif /* CONFIG_LINKER_USE_BOOT_SECTION */

#if defined(CONFIG_LINKER_USE_PINNED_SECTION)
#define __pinned_func	Z_GENERIC_DOT_SECTION(PINNED_TEXT_SECTION_NAME)
#define __pinned_data	Z_GENERIC_DOT_SECTION(PINNED_DATA_SECTION_NAME)
#define __pinned_rodata	Z_GENERIC_DOT_SECTION(PINNED_RODATA_SECTION_NAME)
#define __pinned_bss	Z_GENERIC_DOT_SECTION(PINNED_BSS_SECTION_NAME)
#define __pinned_noinit	Z_GENERIC_DOT_SECTION(PINNED_NOINIT_SECTION_NAME)
#else
#define __pinned_func
#define __pinned_data
#define __pinned_rodata
#define __pinned_bss
#define __pinned_noinit	__noinit
#endif /* CONFIG_LINKER_USE_PINNED_SECTION */

#if defined(CONFIG_LINKER_USE_ONDEMAND_SECTION)
#define __ondemand_func	Z_GENERIC_DOT_SECTION(ONDEMAND_TEXT_SECTION_NAME)
#define __ondemand_rodata	Z_GENERIC_DOT_SECTION(ONDEMAND_RODATA_SECTION_NAME)
#else
#define __ondemand_func
#define __ondemand_rodata
#endif /* CONFIG_LINKER_USE_ONDEMAND_SECTION */

#if defined(CONFIG_LINKER_USE_PINNED_SECTION)
#define __isr		__pinned_func
#else
#define __isr
#endif

/* Symbol table section */
#if defined(CONFIG_SYMTAB)
#define __symtab_info		Z_GENERIC_SECTION(_SYMTAB_INFO_SECTION_NAME)
#define __symtab_entry		Z_GENERIC_SECTION(_SYMTAB_ENTRY_SECTION_NAME)
#endif /* CONFIG_SYMTAB */

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_LINKER_SECTION_TAGS_H_ */
