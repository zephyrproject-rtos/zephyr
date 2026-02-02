/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/iterable_sections.h>

#if defined(CONFIG_NRF_PERIPHCONF_SECTION_STRIP)

SECTION_PROLOGUE(periphconf_entry,(COPY),SUBALIGN(Z_LINK_ITERABLE_SUBALIGN))
{
	Z_LINK_ITERABLE(periphconf_entry);
} GROUP_ROM_LINK_IN(DEVNULL_REGION, DEVNULL_REGION)

#else

ITERABLE_SECTION_ROM(periphconf_entry, Z_LINK_ITERABLE_SUBALIGN)

#endif
