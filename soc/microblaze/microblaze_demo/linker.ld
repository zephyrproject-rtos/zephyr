/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <layout.h>

_DDR_LOC = _LAYOUT_DDR_LOC;
_DDR_SIZE = _LAYOUT_DDR_SIZE;

MEMORY
{
	app_ram (RX) : ORIGIN = _DDR_LOC, LENGTH = _DDR_SIZE

	/* Used by and documented in include/linker/intlist.ld */
	IDT_LIST  (wx)      : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
}

#include <zephyr/arch/microblaze/linker.ld>
