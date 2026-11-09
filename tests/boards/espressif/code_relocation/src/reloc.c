/* SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/section_tags.h>
#include <esp_private/cache_utils.h>

#include "reloc.h"

const uint32_t reloc_rodata[4] = {3U, 5U, 7U, 11U};
uint32_t reloc_data = RELOC_DATA_INIT;
uint32_t reloc_bss;
__noinit uint32_t reloc_noinit;

#if defined(CONFIG_ESP_SPIRAM)
Z_GENERIC_SECTION(.ext_ram.bss.reloc) uint8_t reloc_ext_bss[RELOC_EXT_BSS_SIZE];
#endif

uint32_t reloc_func(uint32_t x)
{
	reloc_bss += x;

	return (x * reloc_rodata[x & 3U]) + reloc_data + reloc_bss;
}

uint32_t reloc_func_no_cache(uint32_t x)
{
	uint32_t ret;

	spi_flash_disable_interrupts_caches_and_other_cpu();
	ret = reloc_func(x);
	spi_flash_enable_interrupts_caches_and_other_cpu();

	return ret;
}
