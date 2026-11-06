/*
 * Copyright (c) 2026 Conclusive Engineering Sp. z o.o.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

SECTION_DATA_PROLOGUE(.internalRAM,,)
{
	. = ALIGN(4);
	*(.internalRAM)
	*(".internalRAM.*")
	. = ALIGN(4);
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
