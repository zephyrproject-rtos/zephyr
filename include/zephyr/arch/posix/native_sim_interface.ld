/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>

SECTIONS
{
	SECTION_PROLOGUE (.native_sim_if.data,,)
	{
		KEEP(*(.native_sim_if.data));
	}
} INSERT AFTER .data;

SECTIONS
{
	SECTION_PROLOGUE (.native_sim_if,,)
	{
		KEEP(*(.native_sim_if));
		KEEP(*(.native_sim_if.*));
	}
} INSERT AFTER .text;
