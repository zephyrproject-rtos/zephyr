/* linker.ld - Linker command/script file
*
* Copyright (c) 2021 Intel Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <autoconf.h>
#include <linker/sections.h>
#include <devicetree.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

#define ICCM_ADDR	CONFIG_ICCM_MEMORY_BASE_ADDRESS
#define DCCM_ADDR	CONFIG_DCCM_MEMORY_BASE_ADDRESS
#define AONRF_ADDR	CONFIG_AONRF_MEMORY_BASE_ADDRESS

#define _TEXT_SECTION_NAME_2  text_2

#define _L2SRAM_TEXT_SECTION_NAME  .l2sram_text
#define _L2SRAM_RODATA_SECTION_NAME  .l2sram_rodata
#define _L2SRAM_DATA_SECTION_NAME  .l2sram_data
#define _L2SRAM_BSS_SECTION_NAME   .l2sram_bss

#define _ICCM_TEXT_SECTION_NAME  .iccm_text
#define _ICCM_RODATA_SECTION_NAME  .iccm_rodata

#define _DCCM_DATA_SECTION_NAME  .dccm_data
#define _DCCM_BSS_SECTION_NAME   .dccm_bss

#define AONRF_DATA_SECTION_NAME  .aonrf_data
#define AONRF_BSS_SECTION_NAME   .aonrf_bss
#define AONRF_TEXT_SECTION_NAME  .aonrf_text

MEMORY
    {
		L2SRAM_RO  (rx) : ORIGIN = (CONFIG_L2SRAM_RO_MEMORY_BASE_ADDRESS), LENGTH = CONFIG_L2SRAM_RO_MEMORY_SIZE
		L2SRAM_RW  (wx) : ORIGIN = (CONFIG_L2SRAM_RW_MEMORY_BASE_ADDRESS), LENGTH = CONFIG_L2SRAM_RW_MEMORY_SIZE
		ICCM       (rx) : ORIGIN = (CONFIG_ICCM_MEMORY_BASE_ADDRESS), LENGTH = CONFIG_ICCM_MEMORY_SIZE
		DCCM       (rw) : ORIGIN = (CONFIG_DCCM_MEMORY_BASE_ADDRESS), LENGTH = CONFIG_DCCM_MEMORY_SIZE
		AONRF      (wx) : ORIGIN = (CONFIG_AONRF_MEMORY_BASE_ADDRESS), LENGTH = CONFIG_AONRF_MEMORY_SIZE
    }

#include <arch/arm/aarch32/cortex_m/scripts/linker.ld>
