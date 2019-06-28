/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Copied from linker.ld */

/* Non-cached region of RAM */
SECTION_DATA_PROLOGUE(_NOCACHE_SECTION_NAME,(NOLOAD),)
{
	MPU_ALIGN(_nocache_ram_size);
	_nocache_ram_start = .;
	*(.nocache)
	*(".nocache.*")
	MPU_ALIGN(_nocache_ram_size);
	_nocache_ram_end = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
_nocache_ram_size = _nocache_ram_end - _nocache_ram_start;
