/*
 * Copyright (c) Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <autoconf.h>
#include <linker/sections.h>
#include <devicetree.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

#include <linker/devicetree_sram.h>

MEMORY
{
	LINKER_DT_SRAM_REGIONS()
}

SECTIONS
{
	LINKER_DT_SRAM_SECTIONS()
}

#include <arch/arm/aarch32/cortex_m/scripts/linker.ld>
