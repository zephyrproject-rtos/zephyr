/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2025 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Copied from linker.ld */

/* Non-cached region of RAM */
SECTION_DATA_PROLOGUE(_NOCACHE_SECTION_NAME,(NOLOAD),)
{
#if defined(CONFIG_MMU)
	MMU_ALIGN;
#else
	MPU_ALIGN(_nocache_ram_size);
#endif
	_nocache_ram_start = .;
	_nocache_noload_ram_start = .;
	*(.nocache)
	*(".nocache.*")

#include <snippets-nocache-section.ld>

	_nocache_noload_ram_end = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
_nocache_noload_ram_size = _nocache_noload_ram_end - _nocache_noload_ram_start;

/* Non-cached loadable region of RAM and ROM */
SECTION_DATA_PROLOGUE(_NOCACHE_LOAD_SECTION_NAME,,)
{
	_nocache_load_ram_start = .;
	*(.nocache_load)
	*(".nocache_load.*")

#if defined(CONFIG_MMU)
	MMU_ALIGN;
#else
	MPU_ALIGN(_nocache_ram_size);
#endif
	_nocache_load_ram_end = .;
	_nocache_ram_end = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
_nocache_load_ram_size = _nocache_load_ram_end - _nocache_load_ram_start;
_nocache_load_rom_start = LOADADDR(_NOCACHE_LOAD_SECTION_NAME);
_nocache_ram_size = _nocache_ram_end - _nocache_ram_start;
