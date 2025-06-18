/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/iterable_sections.h>

SECTION_PROLOGUE(uicr_periphconf_entry,(COPY),SUBALIGN(Z_LINK_ITERABLE_SUBALIGN))
{
	Z_LINK_ITERABLE(uicr_periphconf_entry);
} GROUP_ROM_LINK_IN(DEVNULL_REGION, DEVNULL_REGION)
