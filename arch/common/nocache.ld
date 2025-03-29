/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Copied from linker.ld */

/* Non-cached region of RAM */
#if defined(CONFIG_NOCACHE_MEMORY_IS_LOADABLE)
SECTION_DATA_PROLOGUE(_NOCACHE_SECTION_NAME,,)
#else
SECTION_DATA_PROLOGUE(_NOCACHE_SECTION_NAME,(NOLOAD),)
#endif
{
#if defined(CONFIG_MMU)
	MMU_ALIGN;
#else
	MPU_ALIGN(_nocache_ram_size);
#endif
	_nocache_ram_start = .;
	*(.nocache)
	*(".nocache.*")

#include <snippets-nocache-section.ld>

#if defined(CONFIG_MMU)
	MMU_ALIGN;
#else
	MPU_ALIGN(_nocache_ram_size);
#endif
	_nocache_ram_end = .;

#if defined(CONFIG_NOCACHE_MEMORY_IS_LOADABLE)
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#else
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
#endif

_nocache_ram_size = _nocache_ram_end - _nocache_ram_start;
#if defined(CONFIG_NOCACHE_MEMORY_IS_LOADABLE)
_nocache_load_start = LOADADDR(_NOCACHE_SECTION_NAME);
#endif
