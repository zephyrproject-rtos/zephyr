/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/iterable_sections.h>

/* Only needed because the driver is defined in tests folder */
SECTION_PROLOGUE(custom_api_area,,SUBALIGN(Z_LINK_ITERABLE_SUBALIGN))
{
	Z_LINK_ITERABLE(fake_driver_api);
	PLACE_SYMBOL_HERE(_fake_driver_api_ext_end);
} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
